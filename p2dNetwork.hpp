#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional>
#include <condition_variable>
#include <chrono>
#include "p2d.hpp"
#include "include/asio.hpp"

namespace p2d
{
    namespace net
    {
        template <typename T>
        class server_interface;

        template <typename T>
        struct message_header
        {
            T id{};
            uint32_t size = 0;
        };

        template <typename T>
        struct message
        {
            message_header<T> header{};
            std::vector<uint8_t> body;

            size_t size() const
            {
                return sizeof(message_header<T>) + body.size();
            }

            friend std::ostream &operator << (std::ostream &os, const message<T> &msg)
            {
                os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
                return os;
            }

            template <typename DataType>
            friend message<T> &operator << (message<T> &msg, const DataType &data)
            {
                static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
                size_t i = msg.body.size();
                msg.body.resize(msg.body.size() + sizeof(DataType));
                std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
                msg.header.size = msg.size();
                return msg;
            }

            template <typename DataType>
            friend message<T> &operator >> (message<T> &msg, DataType &data)
            {
                static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");
                size_t i = msg.body.size() - sizeof(DataType);
                std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
                msg.body.resize(i);
                msg.header.size = msg.size();
                return msg;
            }
        };

        template <typename T>
        class connection;

        template <typename T>
        struct owned_message
        {
            std::shared_ptr<connection<T>> remote = nullptr;
            message<T> msg;

            friend std::ostream &operator<<(std::ostream &os, const owned_message<T> &msg)
            {
                os << msg.msg;
                return os;
            }
        };
        

        template <typename T>
        class ts_queue
        {
        public:
            ts_queue() = default;
            ts_queue(const ts_queue<T>&) = delete;
            virtual ~ts_queue() { clear(); }

            const T& front()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.front();
            }

            const T& back()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.back();
            }

            void push_back(const T& item)
            {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_back(std::move(item));

                std::unique_lock<std::mutex> ul(muxBlocking);
                blocking.notify_one();
            }

            void push_front(const T& item)
            {
                std::scoped_lock lock(muxQueue);
                deqQueue.emplace_front(std::move(item));

                std::unique_lock<std::mutex> ul(muxBlocking);
                blocking.notify_one();
            }

            bool empty()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.empty();
            }

            size_t count()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.size();
            }

            void clear()
            {
                std::scoped_lock lock(muxQueue);
                deqQueue.clear();
            }

            T pop_front()
            {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.front());
                deqQueue.pop_front();
                return t;
            }

            T pop_back()
            {
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deqQueue.back());
                deqQueue.pop_back();
                return t;
            }

            void wait()
            {
                while(empty())
                {
                    std::unique_lock<std::mutex> ul(muxBlocking);
                    blocking.wait(ul);
                }
            }

        protected:
            std::mutex muxQueue;
            std::deque<T> deqQueue;
            std::condition_variable blocking;
            std::mutex muxBlocking;
        };

        template <typename T>
        class connection : public std::enable_shared_from_this<connection<T>>
        {
        public:
            enum class owner
            {
                server, client
            };

            connection(owner parent, asio::io_context &asioContext, asio::ip::tcp::socket socket, ts_queue<owned_message<T>> &qIn)
                : m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
            {
                m_ownerType = parent;

                if(m_ownerType == owner::server)
                {
                    m_handshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
                    m_handshakeCheck = scramble(m_handshakeOut);
                }
                else
                {
                    m_handshakeIn = 0;
                    m_handshakeOut = 0;
                }
            }

            virtual ~connection()
            {}

            uint32_t getID() const
            {
                return id;
            }

            void connectToClient(server_interface<T>* server, uint32_t uid = 0)
            {
                if(m_ownerType == owner::server)
                {
                    if(m_socket.is_open())
                    {
                        id = uid;
                        writeValidation();
                        readValidation(server);
                    }
                }
            }

            bool connectToServer(const asio::ip::tcp::resolver::results_type &endpoints)
            {
                if(m_ownerType == owner::client)
                {
                    asio::async_connect(m_socket, endpoints,
                        [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                        {
                            if(!ec)
                            {
                                readValidation();
                            }
                        }
                    );
                    return true;
                }
                return false;
            }

            bool disconnect()
            {
                if(isConnected()) asio::post(m_asioContext, [this]() { m_socket.close(); });
                return true;
            }

            bool isConnected() const
            {
                return m_socket.is_open();
            }

            bool send(const message<T> &msg)
            {
                asio::post(m_asioContext,
                    [this, msg]()
                    {
                        bool writingMessage = !m_qMessagesOut.empty();
                        m_qMessagesOut.push_back(msg);
                        if(!writingMessage) writeHeader();
                    }
                );
                return true;
            }
            
            asio::ip::tcp::socket* getSocket()
            {
            	return &m_socket;
            }

            float versionNumber = 1.0;

        private:
            void readHeader()
            {
                asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            if(m_msgTemporaryIn.header.size > 0)
                            {
                                m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size - sizeof(message_header<T>));
                                readBody();
                            }
                            else
                            {
                                addToIncomingMessageQueue();
                            }
                        }
                        else
                        {
                            std::cout << "[" << id << "] Read Header Fail.\n";
                            m_socket.close();
                        }
                    }
                );
            }

            void readBody()
            {
                asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            addToIncomingMessageQueue();
                        }
                        else
                        {
                            std::cout << "[" << id << "] Read Body Fail.\n";
                            m_socket.close();
                        }
                    }
                );
            }

            void writeHeader()
            {
                asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            if(m_qMessagesOut.front().body.size() > 0)
                            {
                                writeBody();
                            }
                            else
                            {
                                m_qMessagesOut.pop_front();
                                
                                if(!m_qMessagesOut.empty())
                                {
                                    writeHeader();
                                }
                            }
                        }
                        else
                        {
                            std::cout << "[" << id << "] Write Header Fail.\n";
                            m_socket.close();
                        }
                    }
                );
            }

            void writeBody()
            {
                asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            m_qMessagesOut.pop_front();
                            if(!m_qMessagesOut.empty())
                            {
                                writeHeader();
                            }
                        }
                        else
                        {
                            std::cout << "[" << id << "] Write Body Fail.\n";
                            m_socket.close();
                        }
                    }
                );
            }

            void addToIncomingMessageQueue()
            {
                if(m_ownerType == owner::server) m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
                else m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});

                readHeader();
            }

            uint64_t scramble(uint64_t input)
            {
                uint64_t out;
                input &= input >> 1;
                out = input ^ (input << 16) + 63512465;
                out *= out * (versionNumber << 3);
                out += input << 16;
                out /= input;
                out += versionNumber;
                out &= input ^ (input * (out ^ input));
                out ^= versionNumber;
                out ^= 0x01574185AB54;
                return out >> 16;
            }

            void writeValidation()
            {
                asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            if(m_ownerType == owner::client)
                            {
                                readHeader();
                            }
                        }
                        else
                        {
                            m_socket.close();
                        }
                    }
                );
            }

            void readValidation(server_interface<T>* server = nullptr)
            {
                asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
                    [this, server](std::error_code ec, std::size_t length)
                    {
                        if(!ec)
                        {
                            if(m_ownerType == owner::server)
                            {
                                if(m_handshakeIn == m_handshakeCheck)
                                {
                                    std::cout << "Client Validated" << std::endl;
                                    server->onClientValidated(this->shared_from_this());
                                    readHeader();
                                }
                                else
                                {
                                    std::cout << "Client Disconnected (Fail Validation)" << std::endl;
                                    m_socket.close();
                                }
                            }
                            else
                            {
                                m_handshakeOut = scramble(m_handshakeIn);
                                writeValidation();
                            }
                        }
                        else
                        {
                            std::cout << "Client Disconnected (readValidation)" << std::endl;
                            m_socket.close();
                        }
                    }
                );
            }

        protected:
            asio::ip::tcp::socket m_socket;
            asio::io_context& m_asioContext;
            ts_queue<message<T>> m_qMessagesOut;
            ts_queue<owned_message<T>>& m_qMessagesIn;
            message<T> m_msgTemporaryIn;
            owner m_ownerType = owner::server;
            uint32_t id = 0;
            uint64_t m_handshakeOut = 0;
            uint64_t m_handshakeIn = 0;
            uint64_t m_handshakeCheck = 0;
        };

        template<typename T>
        class client_interface
        {
        public:
            client_interface(float versionNumber = 1.0) : m_socket(m_context)
            {
                m_connection->versionNumber = versionNumber;
            }

            virtual ~client_interface()
            {
                disconnect();
            }

            bool connect(const std::string &host, const uint16_t port)
            {
                try
                {
                    asio::ip::tcp::resolver resolver(m_context);
                    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));
                    m_connection = std::make_unique<connection<T>>(connection<T>::owner::client, m_context, asio::ip::tcp::socket(m_context), m_qMessagesIn);
                    m_connection->connectToServer(endpoints);
                    thrContext  = std::thread([this]() { m_context.run(); });
                }
                catch (std::exception e)
                {
                    std::cerr << "Client Exception: " << e.what() << "\n";
                    return false;
                }

                return true;
            }

            void disconnect()
            {
                if(isConnected())
                {
                    m_connection->disconnect();
                }
                m_context.stop();
                if(thrContext.joinable()) thrContext.join();
                m_connection.release();
            }

            bool isConnected()
            {
                if(m_connection) return m_connection->isConnected();
                return false;
            }

            bool sendMessage(const message<T> &msg)
            {
                m_connection->send(msg);
                return true;
            }

            ts_queue<owned_message<T>>& incoming()
            {
                return m_qMessagesIn;
            }

        protected:
            asio::io_context m_context;
            std::thread thrContext;
            asio::ip::tcp::socket m_socket;
            std::unique_ptr<connection<T>> m_connection;

        private:
            ts_queue<owned_message<T>> m_qMessagesIn;
        };

        template <typename T>
        class server_interface
        {
        public:
            server_interface(uint16_t port, float versionNumber = 1.0)
                : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
            {
                this->versionNumber = versionNumber;
            }

            virtual ~server_interface()
            {
                stop();
            }

            bool start()
            {
                try
                {
                    waitForClientConnection();
                    m_threadContext = std::thread([this]() { m_asioContext.run(); });
                }
                catch (std::exception e)
                {
                    std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                    return false;
                }

                std::cout << "[SERVER] Started\n";
                return true;
            }

            void stop()
            {
                m_asioContext.stop();
                if(m_threadContext.joinable()) m_threadContext.join();
                std::cout << "[SERVER] Stopped\n";
            }

            void waitForClientConnection()
            {
                m_asioAcceptor.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket socket)
                    {
                        if(!ec)
                        {
                            std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
                            std::shared_ptr<connection<T>> newconn = std::make_shared<connection<T>>(connection<T>::owner::server, m_asioContext, std::move(socket), m_qMessagesIn);
                            if(this->onClientConnect(newconn))
                            {
                                newconn->versionNumber = this->versionNumber;
                                m_deqConnections.push_back(std::move(newconn));
                                int id = generateUniqueID();
                                m_deqConnections.back()->connectToClient(this, id);
                                ids.push_back(id);
                                std::cout << "[" << m_deqConnections.back()->getID() << "] Connection Approved\n";
                            }
                            else
                            {
                                std::cout << "[-----] Connection Denied\n";
                            }
                        }
                        else
                        {
                            std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                        }
                        waitForClientConnection();
                    }
                );
            }

            void messageClient(std::shared_ptr<connection<T>> client, const message<T> &msg)
            {
                if(client && client->isConnected())
                {
                    client->send(msg);
                }
                else
                {
                    this->onClientDisconnect(client);
                    client.reset();
                    std::cout << "Removing ID: " << std::to_string(client->getID()) << "\n";
                    ids.erase(std::remove(ids.begin(), ids.end(), client->getID()), ids.end());
                    m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
                }
            }

            void messageAllClients(const message<T> &msg, std::shared_ptr<connection<T>> ignoreClient = nullptr)
            {
                bool invalidClientExists = false;
                std::vector<int> idsToRemove;

                for(auto &client : m_deqConnections)
                {
                    if(client && client->isConnected())
                    {
                        if(client != ignoreClient) client->send(msg);
                    }
                    else
                    {
                        idsToRemove.push_back(client->getID());
                        this->onClientDisconnect(client);
                        client.reset();
                        invalidClientExists = true;
                    }
                }

                if(invalidClientExists)
                {
                    std::sort(idsToRemove.begin(), idsToRemove.end());
                    for(size_t i = 0; i < ids.size(); i++)
                    {
                        if(std::binary_search(idsToRemove.begin(), idsToRemove.end(), ids[i]))
                        {
                            std::swap(ids[i], ids[ids.size() - 1]);
                            ids.pop_back();
                        }
                    }
                    m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
                }
            }

            void update(size_t maxMessages = -1, bool wait = false)
            {
                if(wait) m_qMessagesIn.wait();

                size_t messageCount = 0;
                while(messageCount < maxMessages && !m_qMessagesIn.empty())
                {
                    auto msg = m_qMessagesIn.pop_front();
                    this->onMessage(msg.remote, msg.msg);
                    messageCount++;
                }
            }

        protected:
            virtual bool onClientConnect(std::shared_ptr<connection<T>> client)
            {
                return false;
            }

            virtual void onClientDisconnect(std::shared_ptr<connection<T>> client)
            {
                ids.erase(std::remove(ids.begin(), ids.end(), client->getID()), ids.end());
            }

            virtual void onMessage(std::shared_ptr<connection<T>> client, message<T> &msg)
            {

            }

        public:
            virtual void onClientValidated(std::shared_ptr<connection<T>> client)
            {

            }

        protected:
            ts_queue<owned_message<T>> m_qMessagesIn;
            std::deque<std::shared_ptr<connection<T>>> m_deqConnections;
            asio::io_context m_asioContext;
            std::thread m_threadContext;
            asio::ip::tcp::acceptor m_asioAcceptor;
            std::vector<int> ids;
            float versionNumber;

        private:
            int generateUniqueID()
            {
                p2d::Util u;
                int id = u.random();
                while(count(ids, id) != false)
                {
                    id = u.random();
                }
                return id;
            }

            bool count(std::vector<int> vec, int i)
            {
                return std::count(vec.begin(), vec.end(), i);
            }
        };
    }
}
