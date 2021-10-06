#ifndef P2D_HPP
#define P2D_HPP

#include <cstring>
#include <memory>
#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional>
#include <condition_variable>
#include <chrono>
#include <iterator>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <istream>
#include <ostream>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <mutex>
#include <deque>
#include <chrono>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>



struct Vector2
{
    Vector2() {}

    Vector2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }

    Vector2(sf::Vector2f vec)
    {
        x = vec.x, y = vec.y;
    }

    Vector2(sf::Vector2u vec)
    {
        x = vec.x, y = vec.y;
    }

    friend std::ostream &operator << (std::ostream &os, const Vector2 vec)
    {
        os << "X: " << vec.x << " Y: " << vec.y;
        return os;
    }

    friend Vector2 &operator * (Vector2 vec, float f)
    {
        vec.x *= f;
        vec.y *= f;
        return vec;
    }

    friend Vector2 &operator / (Vector2 vec, float f)
    {
        vec.x /= f;
        vec.y /= f;
        return vec;
    }

    friend Vector2 &operator + (Vector2 vecA, Vector2 vecB)
    {
        vecA.x += vecB.x;
        vecA.y += vecB.y;
        return vecA;
    }

    friend Vector2 &operator - (Vector2 vecA, Vector2 vecB)
    {
        vecA.x -= vecB.x;
        vecA.y -= vecB.y;
        return vecA;
    }

    float dot(Vector2 vec)
    {
        return (x * vec.x) + (y * vec.y);
    }

    float mag()
    {
        return sqrt((x * x) + (y * y));
    }

    float x = 0;
    float y = 0;
};

struct AABB
{
    AABB()
    {
        center = {0, 0};
        halfDimension = {0, 0};
    }

    AABB(Vector2 center, Vector2 halfDimension)
    {
        this->center = center;
        this->halfDimension = halfDimension;
    }

    bool ContainsPoint(Vector2 point)
    {
        return !(point.x < center.x - halfDimension.x || point.x > center.x + halfDimension.x || point.y < center.y - halfDimension.y || point.y > center.y + halfDimension.y);
    }

    bool Intersects(AABB other)
    {
        return ((other.center.x - other.halfDimension.x > center.x - halfDimension.x && other.center.x - halfDimension.x < center.x + halfDimension.x) || (other.center.x + halfDimension.x > center.x - halfDimension.x && other.center.x + halfDimension.x < center.x + halfDimension.x) || (other.center.y - halfDimension.y > center.y - halfDimension.y && other.center.y - halfDimension.y < center.y + halfDimension.y) || (other.center.y + halfDimension.y > center.y - halfDimension.y && other.center.y + halfDimension.y < center.y + halfDimension.y));
    }

    Vector2 center;
    Vector2 halfDimension;
};

struct Sprite
{
    Sprite() {}
    
    Sprite(Vector2 pos, Vector2 size)
    {
        this->pos = pos;
        this->size = size;
    }
    
    Sprite(int x, int y, int width, int height)
    {
        pos.x = x;
        pos.y = y;
        size.x = width;
        size.y = height;
    }

    Vector2 pos;
    Vector2 size;
};



template <typename T>
class server_interface;

template <typename T>
struct MessageHeader
{
    T id{};
    uint32_t size = 0;
};

template <typename T>
 struct Message { MessageHeader<T> header{}; std::vector<uint8_t> body; std::size_t size() const { return sizeof(MessageHeader<T>) + body.size(); } friend std::ostream &operator << (std::ostream &os, const Message<T> &msg) { os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size; return os; } template <typename DataType> friend Message<T> &operator << (Message<T> &msg, const DataType &data) { static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be sent"); size_t i = msg.body.size(); msg.body.resize(msg.body.size() + sizeof(DataType)); std::memcpy(msg.body.data() + i, &data, sizeof(DataType)); msg.header.size = msg.size(); return msg; } template <typename DataType> friend Message<T> &operator >> (Message<T> &msg, DataType &data) { static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be recieved"); size_t i = msg.body.size() - sizeof(DataType); std::memcpy(&data, msg.body.data() + i, sizeof(DataType)); msg.body.resize(i); msg.header.size = msg.size(); return msg; } };

template <typename T>
class Connection;

template <typename T>
struct OwnedMessage
{
    std::shared_ptr<Connection<T>> remote = nullptr;
    Message<T> msg;
    friend std::ostream &operator << (std::ostream &os, const OwnedMessage<T> &msg)
    {
        os << msg.msg;
        return os;
    }
};

template <typename T>
class TsQueue
{
public:
    TsQueue() = default;
    TsQueue(const TsQueue<T>&) = delete;

    virtual ~TsQueue()
    {
        clear();
    }

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
class Connection : public std::enable_shared_from_this<Connection<T>>
{
public:
    enum class owner { server, client };
    
    Connection(owner parent, asio::io_context &asioContext, asio::ip::tcp::socket socket, TsQueue<OwnedMessage<T>> &qIn)
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

    virtual ~Connection() {} 
    
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
            asio::async_connect(m_socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
            {
                if(!ec)
                {
                    readValidation();
                }
            });
            return true;
        }
        return false;
    }

    bool disconnect()
    {
        if(isConnected()) asio::post(m_asioContext, [this]()
        {
            m_socket.close();
        });
        return true;
    }

    bool isConnected() const
    {
        return m_socket.is_open();
    }

    bool send(const Message<T> &msg)
    {
        asio::post(m_asioContext, [this, msg]()
        {
            bool writingMessage = !m_qMessagesOut.empty();
            m_qMessagesOut.push_back(msg);
            if(!writingMessage) writeHeader();
        });
        return true;
    }

    asio::ip::tcp::socket* getSocket()
    {
        return &m_socket;
    }

private:
    void readHeader()
    {
        asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(MessageHeader<T>)), [this](std::error_code ec, std::size_t length)
        {
            if(!ec)
            {
                if(m_msgTemporaryIn.header.size > 0)
                {
                    m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size - sizeof(MessageHeader<T>));
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
        });
    }
    
    void readBody()
    {
        asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()), [this](std::error_code ec, std::size_t length)
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
        });
    }

    void writeHeader()
    {
        asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(MessageHeader<T>)), [this](std::error_code ec, std::size_t length)
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
        });
    }

    void writeBody()
    {
        asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()), [this](std::error_code ec, std::size_t length)
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
        });
    }

    void addToIncomingMessageQueue()
    {
        if(m_ownerType == owner::server)
        m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
        else m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});
        readHeader();
    }
    
    uint64_t scramble(uint64_t input)
    {
        uint64_t out; input &= input >> 1; out = input ^ (input << 16) + 63512465; out *= out * 3; out += input << 16; out /= input; out++; out &= input ^ (input * (out ^ input)); out ^= 76225362; out ^= 0x01574185AB54; return out >> 16;
    }

    void writeValidation()
    {
        asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length)
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
        });
    }

    void readValidation(server_interface<T>* server = nullptr)
    {
        asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)), [this, server](std::error_code ec, std::size_t length)
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
        });
    }
protected:
    asio::ip::tcp::socket m_socket;
    asio::io_context& m_asioContext;
    TsQueue<Message<T>> m_qMessagesOut;
    TsQueue<OwnedMessage<T>>& m_qMessagesIn;
    Message<T> m_msgTemporaryIn;
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
    client_interface() : m_socket(m_context) {}
    
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
            m_connection = std::make_unique<Connection<T>>(Connection<T>::owner::client, m_context, asio::ip::tcp::socket(m_context), m_qMessagesIn);
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
        if(m_connection)
        return m_connection->isConnected();
        return false;
    }
    
    bool sendMessage(const Message<T> &msg)
    {
        m_connection->send(msg);
        return true;
    }

    TsQueue<OwnedMessage<T>>& incoming()
    {
        return m_qMessagesIn;
    }
    
protected:
    asio::io_context m_context;
    std::thread thrContext;
    asio::ip::tcp::socket m_socket;
    std::unique_ptr<Connection<T>> m_connection;

private:
    TsQueue<OwnedMessage<T>> m_qMessagesIn;
};

template <typename T>
class server_interface
{
public:
    server_interface(uint16_t port) : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}
    
    virtual ~server_interface()
    {
        stop();
    }
    
    bool start()
    {
        try
        {
            waitForClientConnection();
            m_threadContext = std::thread([this]()
            {
                m_asioContext.run();
            });
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
        m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
        {
            if(!ec)
            {
                std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
                std::shared_ptr<Connection<T>> newconn = std::make_shared<Connection<T>>(Connection<T>::owner::server, m_asioContext, std::move(socket), m_qMessagesIn);
                if(this->onClientConnect(newconn))
                {
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
        });
    }

    void messageClient(std::shared_ptr<Connection<T>> client, const Message<T> &msg)
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

    void messageAllClients(const Message<T> &msg, std::shared_ptr<Connection<T>> ignoreClient = nullptr)
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
    virtual bool onClientConnect(std::shared_ptr<Connection<T>> client)
    {
        return false;
    }

    virtual void onClientDisconnect(std::shared_ptr<Connection<T>> client)
    {
        ids.erase(std::remove(ids.begin(), ids.end(), client->getID()), ids.end());
    }
    
    virtual void onMessage(std::shared_ptr<Connection<T>> client, Message<T> &msg) {}
    
public:
    virtual void onClientValidated(std::shared_ptr<Connection<T>> client) {}
    
protected:
    TsQueue<OwnedMessage<T>> m_qMessagesIn;
    std::deque<std::shared_ptr<Connection<T>>> m_deqConnections;
    asio::io_context m_asioContext;
    std::thread m_threadContext;
    asio::ip::tcp::acceptor m_asioAcceptor;
    std::vector<int> ids;

private:
    int generateUniqueID()
    {
        int id = std::chrono::system_clock::now().time_since_epoch().count();
        while(count(ids, id) != false)
        {
            id = id = std::chrono::system_clock::now().time_since_epoch().count();
        }
        return id;
    }
    
    bool count(std::vector<int> vec, int i)
    {
        return std::count(vec.begin(), vec.end(), i);
    }
};



typedef std::string String;



class NetworkServer
{

};

class Network
{

};

class File
{
public:
    File() {}

    File(std::vector<uint8_t> FileData)
    {
        this->data = FileData;
    }

    size_t Size() const
    {
        return data.size();
    }

    template <typename DataType>
    friend File &operator << (File &f, const DataType &d)
    {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be saved");
        size_t i = f.data.size();
        f.data.resize(f.data.size() + sizeof(DataType));
        std::memcpy(f.data.data() + i, &d, sizeof(DataType));
        return f;
    }

    template <typename DataType>
    friend File &operator >> (File &f, DataType &d)
    {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be loaded");
        size_t i = f.data.size() - sizeof(DataType);
        std::memcpy(&d, f.data.data() + i, sizeof(DataType));
        f.data.resize(i);
        return f;
    }

    std::vector<uint8_t>* Data()
    {
        return &data;
    }

    void Save(std::string FilePath)
    {
        std::ofstream out(FilePath, std::ios::out | std::ofstream::binary);
        std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>(out));
        out.close();
    }

    void Load(std::string FilePath)
    {
        std::vector<uint8_t> data;
        std::ifstream in(FilePath, std::ios::in | std::ifstream::binary);
        std::istreambuf_iterator<char> iter{in};
        std::istreambuf_iterator<char> end{};
        std::copy(iter, end, std::back_inserter(data));
        in.close();
        this->data = data;
    }

private:
    std::vector<uint8_t> data;
};

class GameObject;

class QuadTree
{
public:
    #define QTCAPACITY 4

    AABB boundary;

    GameObject* objects[QTCAPACITY];
    AABB positions[QTCAPACITY];
    int numObjects = 0;
    bool divided = false;

    QuadTree* northWest = nullptr;
    QuadTree* northEast = nullptr;
    QuadTree* southWest = nullptr;
    QuadTree* southEast = nullptr;
    
    QuadTree(AABB boundary)
    {
        this->boundary = boundary;
    }

    bool insert(GameObject* obj, AABB pos)
    {
        if(!boundary.Intersects(pos)) return false;
        if(numObjects < QTCAPACITY)
        {
            objects[numObjects] = obj;
            positions[numObjects] = pos;
            numObjects++;
            return true;
        }

        if(!divided) subdivide();

        if(northWest->insert(obj, pos)) return true;
        if(northEast->insert(obj, pos)) return true;
        if(southWest->insert(obj, pos)) return true;
        if(southWest->insert(obj, pos)) return true;
        return false;
    }

    void subdivide()
    {
        AABB nw = {{boundary.center.x - (boundary.center.x / 2.f), boundary.center.y - (boundary.center.y / 2.f)}, {boundary.halfDimension.x / 2.f, boundary.halfDimension.y / 2.f}};
        AABB ne = {{boundary.center.x - (boundary.center.x / 2.f), boundary.center.y + (boundary.center.y / 2.f)}, {boundary.halfDimension.x / 2.f, boundary.halfDimension.y / 2.f}};
        AABB sw = {{boundary.center.x + (boundary.center.x / 2.f), boundary.center.y - (boundary.center.y / 2.f)}, {boundary.halfDimension.x / 2.f, boundary.halfDimension.y / 2.f}};
        AABB se = {{boundary.center.x + (boundary.center.x / 2.f), boundary.center.y + (boundary.center.y / 2.f)}, {boundary.halfDimension.x / 2.f, boundary.halfDimension.y / 2.f}};
        northWest = new QuadTree(nw);
        northEast = new QuadTree(ne);
        southWest = new QuadTree(sw);
        southEast = new QuadTree(se);
        divided = true;
    }

    std::vector<GameObject*> queryRange(AABB range)
    {
        std::vector<GameObject*> result;

        if(!boundary.Intersects(range) && !range.Intersects(boundary))
        {
            std::cout << "does not intersect range\n";
            return result;
        }

        for(int i = 0; i < numObjects; i++)
        {
            if(positions[i].Intersects(range))
            {
                result.push_back(objects[i]);
                continue;
            }
            if(range.Intersects(positions[i])) result.push_back(objects[i]);
        }

        if(!divided) return result;

        std::vector<GameObject*> temp;
        temp = northWest->queryRange(range);
        result.insert(result.end(), temp.begin(), temp.end());
        temp = northEast->queryRange(range);
        result.insert(result.end(), temp.begin(), temp.end());
        temp = southWest->queryRange(range);
        result.insert(result.end(), temp.begin(), temp.end());
        temp = southEast->queryRange(range);
        result.insert(result.end(), temp.begin(), temp.end());

        return result;
    }

    int getCountAll()
    {
        int result = numObjects;

        if(!divided) return result;

        result += northWest->getCountAll();
        result += northEast->getCountAll();
        result += southWest->getCountAll();
        result += southEast->getCountAll();

        return result;
    }

    void clear()
    {
        numObjects = 0;

        if(divided)
        {
            northWest->clear();
            northEast->clear();
            southWest->clear();
            southEast->clear();

            delete northWest;
            delete northEast;
            delete southWest;
            delete southEast;
            divided = false;
        }
    }
};

class Time
{
public:
    Time() {}
    float deltaTime;
    float time;
    float frameRate;
};

class Math
{
public:
    Math() {}
    
    double e = (double)2.7182818284590452353602874713527;
    double pi = (double)3.141592654;

    uint32_t Random()
    {
        lehmer += 0xe120fc15;
        uint64_t tmp;
        tmp = (uint64_t)lehmer * 0x4a39b70d;
        uint32_t m1 = (tmp >> 32) ^ tmp;
        tmp = (uint64_t)m1 * 0x12fad5c9;
        uint32_t m2 = (tmp >> 32) ^ tmp;
        return m2;
    }

    int Random(int min, int max)
    {
        lehmer += 0xe120fc15;
        uint64_t tmp;
        tmp = (uint64_t)lehmer * 0x4a39b70d;
        uint32_t m1 = (tmp >> 32) ^ tmp;
        tmp = (uint64_t)m1 * 0x12fad5c9;
        uint32_t m2 = (tmp >> 32) ^ tmp;
        return (m2 % (max - min)) + min;
    }

    float Random(float min, float max)
    {
        lehmer += 0xe120fc15;
        uint64_t tmp;
        tmp = (uint64_t)lehmer * 0x4a39b70d;
        uint32_t m1 = (tmp >> 32) ^ tmp;
        tmp = (uint64_t)m1 * 0x12fad5c9;
        uint32_t m2 = (tmp >> 32) ^ tmp;
        return (Sigmoid((long double)m2 / (long double)2147483648) * (max - min)) + min;
    }

    double Random(double min, double max)
    {
        lehmer += 0xe120fc15;
        uint64_t tmp;
        tmp = (uint64_t)lehmer * 0x4a39b70d;
        uint32_t m1 = (tmp >> 32) ^ tmp;
        tmp = (uint64_t)m1 * 0x12fad5c9;
        uint32_t m2 = (tmp >> 32) ^ tmp;
        return (Sigmoid((long double)m2 / (long double)2147483648.f) * (max - min)) + min;
    }

    void Seed(uint32_t seed)
    {
        lehmer = seed;
    }

    // float DotProduct(Vector2 VecA, Vector2 VecB)
    // {
    //     return (VecA.x * VecB.x) + (VecA.y * VecB.y);
    // }

    // float Magnitude(Vector2 Vec)
    // {
    //     return sqrt((Vec.x * Vec.x) + (Vec.y * Vec.y));
    // }

    Vector2 Rotate(Vector2 Vec, Vector2 Origin, float Angle)
    {
        float s = sin(Angle);
        float c = cos(Angle);

        Vec.x -= Origin.x;
        Vec.y -= Origin.y;

        float x = Vec.x * c - Vec.y * s;
        float y = Vec.x * s + Vec.y * c;

        return {x + Origin.x, y + Origin.y};
    }

    long double Sigmoid(double x)
    {
        return (long double)1.f / ((long double)1.f + (long double)std::pow((long double)e, (long double)-x));
    }

private:
    uint32_t lehmer = 0;
};

class Input
{
public:
    Input() {}

    typedef sf::Keyboard::Key Key;
    typedef sf::Mouse::Button MouseButton;

    bool GetKey(int key)
    {
        if(keys.count(key) > 0)
        {
            return keys.at(key);
        }
    }

    bool GetKeyDown(int key)
    {
        if(keys.count(key) > 0)
        {
            return keysPressed[key];
        }
    }

    bool GetKeyUp(int key)
    {
        if(keys.count(key) > 0)
        {
            return keysReleased[key];
        }
    }

    bool GetButton(int button)
    {
        if(buttons.count(button) > 0)
        {
            return buttons.at(button);
        }
    }

    bool GetButtonDown(int button)
    {
        if(buttons.count(button) > 0)
        {
            return buttonsPressed[button];
        }
    }

    bool GetButtonUp(int button)
    {
        if(buttons.count(button) > 0)
        {
            return buttonsReleased[button];
        }
    }

    Vector2 GetMousePos()
    {
        return mousePos;
    }

    float GetMouseX()
    {
        return mousePos.x;
    }

    float GetMouseY()
    {
        return mousePos.y;
    }

    float GetMouseWheel()
    {
        return scrollWheel;
    }

    std::string GetTextTyped()
    {
        return text;
    }

    void _update()
    {
        for(int i = 0; i < Key::KeyCount; i++)
        {
            keysPressed[i] = false;
            keysReleased[i] = false;
        }

        for(int i = 0; i < MouseButton::ButtonCount; i++)
        {
            buttonsPressed[i] = false;
            buttonsReleased[i] = false;
        }
        text = "";
    }

    void _poll(sf::Event event)
    {
        switch(event.type)
        {
            case sf::Event::KeyPressed:
            {
                keys.at(event.key.code) = true;
                keysPressed.at(event.key.code) = true;
                break;
            }
            case sf::Event::KeyReleased:
            {
                keys.at(event.key.code) = false;
                keysReleased.at(event.key.code) = true;
                break;
            }
            case sf::Event::MouseButtonPressed:
            {
                buttons.at(event.key.code) = true;
                buttonsPressed.at(event.mouseButton.button) = true;
                break;
            }
            case sf::Event::MouseButtonReleased:
            {
                buttons.at(event.key.code) = false;
                buttonsReleased.at(event.mouseButton.button) = true;
                break;
            }
            case sf::Event::MouseMoved:
            {
                mousePos.x = event.mouseMove.x;
                mousePos.y = event.mouseMove.y;
                break;
            }
            case sf::Event::MouseWheelScrolled:
            {
                scrollWheel += event.mouseWheelScroll.delta;
                break;
            }
            case sf::Event::TextEntered:
            {
                text = text + static_cast<char>(event.text.unicode);
            }
        }
    }

    void _setup()
    {
        if(hasSetup) return;
        hasSetup = true;

        for(int i = 0; i < Key::KeyCount; i++)
        {
            keys.insert({i, false});
            keysPressed.insert({i, false});
            keysReleased.insert({i, false});
        }

        for(int i = 0; i < MouseButton::ButtonCount; i++)
        {
            buttons.insert({i, false});
            buttonsPressed.insert({i, false});
            buttonsPressed.insert({i, false});
        }
    }

private:
    std::map<int, bool> keys;
    std::map<int, bool> keysPressed;
    std::map<int, bool> keysReleased;
    std::map<int, bool> buttons;
    std::map<int, bool> buttonsPressed;
    std::map<int, bool> buttonsReleased;
    Vector2 mousePos;
    float scrollWheel;
    bool hasSetup = false;
    std::string text;
};

class Audio
{
public:
    Audio() {}

    bool AddSound(std::string name, std::string filepath)
    {
        if(sounds.count(name) != 0) return false;
        sf::SoundBuffer buffer;
        if(!buffer.loadFromFile(filepath)) return false;
        sounds.insert({name, buffer});
        return true;
    }

    bool RemoveSound(std::string name)
    {
        if(sounds.count(name) == 0) return false;
        sounds.erase(name);
        return true;
    }

    void RemoveAllSounds()
    {
        sounds.clear();
    }

    bool PlaySound(std::string name, float volume = 100.f, bool loop = false)
    {
        if(playing.count(name) != 0) return false;
        playing.insert({name, {}});
        playing.at(name).setBuffer(sounds.at(name));
        playing.at(name).setVolume(volume);
        playing.at(name).setLoop(loop);
        playing.at(name).play();
        return true;
    }

    bool PauseSound(std::string name)
    {
        if(playing.count(name) != 0) return false;
        playing.at(name).pause();
        return true;
    }

    bool StopSound(std::string name)
    {
        if(playing.count(name) == 0) return false;
        playing.at(name).stop();
        playing.erase(name);
        return true;
    }

    void StopAllSounds()
    {
        std::map<std::string, sf::Sound>::iterator it = playing.begin();
        while(it != playing.end())
        {
            playing.at(it->first).stop();
        }
        playing.clear();
    }

    bool AddMusic(std::string name, std::string filepath)
    {
        if(musicFiles.count(name) != 0) return false;
        musicFiles.insert({name, filepath});
        return true;
    }

    bool RemoveMusic(std::string name)
    {
        if(musicFiles.count(name) == 0) return false;
        musicFiles.erase(name);
        return true;
    }

    void RemoveAllMusic()
    {
        musicFiles.clear();
    }

    bool PlayMusic(std::string name, float volume = 100.f, bool loop = true)
    {
        music.stop();
        if(musicFiles.count(name) == 0) return false;
        if(!music.openFromFile(musicFiles.at(name))) return false;
        music.setVolume(volume);
        music.setLoop(loop);
        music.play();
    }



private:
    std::map<std::string, sf::SoundBuffer> sounds;
    std::map<std::string, sf::Sound> playing;
    std::map<std::string, std::string> musicFiles;
    sf::Music music;
    sf::SoundBuffer buffer;
};

class Transform;

class Script
{
public:
    Script() {}

    virtual void Start()
    {

    }

    virtual void Update()
    {

    }

    virtual void LateUpdate()
    {

    }

    virtual void OnCreate()
    {

    }

    virtual void OnDestroy()
    {

    }

    void _setup(sf::View* camera, Time* time, Math* math, Input* input, Audio* audio)
    {
        this->camera = camera;
        this->time = time;
        this->math = math;
        this->input = input;
        this->audio = audio;
    }

    GameObject* self;
    template <class T>
    T* GetComponent()
    {
        return self->template GetComponent<T>();
    }

    template <class T>
    bool HasComponent()
    {
        return self->template HasComponent<T>();
    }

    sf::View* camera;
    Transform* transform;
    Time* time;
    Math* math;
    Input* input;
    Audio* audio;
    float dt;
    bool simulated;
};

class Transform : public Script
{
public:
    Transform() : Script()
    {
        transform = this;

        position = {0, 0};
        scale = {1, 1};
        rotation = 0;
    }

    Vector2 position = {0.f, 0.f};
    Vector2 scale = {1.f, 1.f};
    float rotation = 0.f;
};

class Drawable : public Script
{
public:
    Drawable() : Script() {}

    virtual void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) {}

    bool lateRender = false;
};

class SpriteRenderer : public Drawable
{
public:
    SpriteRenderer() : Drawable() {}
    SpriteRenderer(Sprite sprite) : Drawable()
    {
        this->sprite = sprite;
    }
    Sprite sprite;

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) override
    {
        int prevVertices = va->getVertexCount();
        Vector2 spriteSize = sprite.size;
        Vector2 spritePos = sprite.pos;
        Vector2 sprScale = {transform->scale.x * spriteSize.x, transform->scale.y * spriteSize.y};
        Vector2 sprPos = {transform->position.x - (sprScale.x / 2.f), transform->position.y - (sprScale.y / 2.f)};
        va->resize(prevVertices + 4);
        sf::Vertex* quad = &va[0][prevVertices];
        quad[0].position = {sprPos.x, sprPos.y};
        quad[1].position = {sprPos.x + sprScale.x, sprPos.y};
        quad[2].position = {sprPos.x + sprScale.x, sprPos.y + sprScale.y};
        quad[3].position = {sprPos.x, sprPos.y + sprScale.y};
        quad[0].texCoords = {spritePos.x, spritePos.y};
        quad[1].texCoords = {spritePos.x + spriteSize.x, spritePos.y};
        quad[2].texCoords = {spritePos.x + spriteSize.x, spritePos.y + spriteSize.y};
        quad[3].texCoords = {spritePos.x, spritePos.y + spriteSize.y};
    }
};

class Collider : public Script
{
public:
    struct Collision
    {
        Collision(Collider* c1, Collider* c2, bool collision)
        {
            HasCollision = collision;
            this->c1 = c1;
            this->c2 = c2;
        }

        Collision(Collider* c1, Collider* c2, float overlap, Vector2 axis)
        {
            Overlap = overlap;
            Axis = axis;
            this->c1 = c1;
            this->c2 = c2;
        }

        Collider* c1;
        Collider* c2;
        bool HasCollision = false;
        float Overlap = 0.0f;
        Vector2 Axis = {0, 0};
    };

    enum ColliderType
    {
        Poly, Circle
    };

    Collider() : Script()
    {
        
    }

    void _resolve(Collider* other)
    {
        Collision collision = resolve(other);
        if(!collision.HasCollision)
        {
            IsColliding = false;
            return;
        }
        IsColliding = true;
        if(_isStatic && other->_isStatic) return;

        
        if(_isStatic)
        {
            other->transform->position.x += collision.Axis.x * collision.Overlap;
            other->transform->position.y += collision.Axis.y * collision.Overlap;
            return;
        }

        if(other->_isStatic)
        {
            transform->position.x += collision.Axis.x * collision.Overlap;
            transform->position.y += collision.Axis.y * collision.Overlap;
            return;
        }

        other->transform->position.x += collision.Axis.x * collision.Overlap * 0.5f;
        other->transform->position.y += collision.Axis.y * collision.Overlap * 0.5f;
        transform->position.x += collision.Axis.x * collision.Overlap * 0.5f;
        transform->position.y += collision.Axis.y * collision.Overlap * 0.5f;
        
        return;
    }

    Vector2 Centre;
    Vector2 Scale;
    float Rotation;
    // only applicable to circles
    float Radius;
    ColliderType Type;
    // only applicable to polys
    std::vector<Vector2> Vertices;
    // don't change this, you will break the collision system
    bool _isStatic = true;
    bool IsColliding = false;

private:
    Collision resolve(Collider* other)
    {
        Vector2 v1 = {Centre.x + transform->position.x, Centre.y + transform->position.y};
        Vector2 v2 = {other->Centre.x + other->transform->position.x, other->Centre.y + other->transform->position.y};

        if(Type == Circle && other->Type == Circle)
        {
            float f1 = Radius;
            float f2 = other->Radius;
            float minDst = f1 + f2;

            Vector2 delta = {v1.x - v2.x, v1.y - v2.y};
            float dst = delta.mag();

            if(dst < minDst)
            {
                Vector2 axis;
                axis.x = delta.x * (1 / dst);
                axis.y = delta.y * (1 / dst);

                return {this, other, (minDst - dst), axis};
            }

            return {this, other, false};
        }

        if(Type == Poly && other->Type == Circle)
        {
            double minDst = 0xFFFFFFFFFFFFFFFF;
            Vector2 closestDelta = {};
            Vector2 axis = {};

            for(int i = 0; i < Vertices.size(); i++)
            {
                Vector2 vec = Vertices[i];

                Vector2 worldVert = {v1.x + vec.x, v1.y + vec.y};

                Vector2 delta = {worldVert.x - v2.x, worldVert.y - v2.y};

                double dst = delta.mag();
                if(dst < minDst)
                {
                    minDst = dst;
                    closestDelta = delta;
                }
            }

            double magnitude = closestDelta.mag();
            if(magnitude != 0)
            {
                axis.x = closestDelta.x * (1 / magnitude);
                axis.y = closestDelta.y * (1 / magnitude);
                return {this, other, (magnitude - other->Radius), axis};
            }

            return {this, other, false};
        }

        if(Type == Circle && other->Type == Poly)
        {
            double minDst = 0xFFFFFFFFFFFFFFFF;
            Vector2 closestDelta = {};
            Vector2 axis = {};

            for(int i = 0; i < other->Vertices.size(); i++)
            {
                Vector2 vec = other->Vertices[i];

                Vector2 worldVert = {v2.x + vec.x, v2.y + vec.y};

                Vector2 delta = {worldVert.x - v1.x, worldVert.y - v1.y};

                double dst = delta.mag();
                if(dst < minDst)
                {
                    minDst = dst;
                    closestDelta = delta;
                }
            }

            double magnitude = closestDelta.mag();
            if(magnitude != 0)
            {
                axis.x = closestDelta.x * (1 / magnitude);
                axis.y = closestDelta.y * (1 / magnitude);
                return {this, other, (magnitude - Radius), axis};
            }

            return {this, other, false};
        }
    }
};

// This will not work if there is not already a collider
class RigidBody : public Script
{
public:
    RigidBody() : Script()
    {
        if(!HasComponent<Collider>())
        {
            std::cout << "Error: RigidBody has no collider\n";
            exit(1);
        }
        collider = GetComponent<Collider>();
    }

    void OnCreate() override
    {
        collider->_isStatic = false;
    }

    void OnDestroy() override
    {
        collider->_isStatic = true;
    }

    void _checkCollisions(std::vector<Collider*> others)
    {
        for(int i = 0; i < others.size(); i++)
        {
            collider->_resolve(others[i]);
        }
    }

    Collider* collider;
};

class BaseUIComponent : public Drawable
{
public:
    BaseUIComponent() : Drawable() {}

    // position defined by a percentage of the screen size
    Vector2 percentPosition;
    // scale defined by a percentage of the screen size
    Vector2 percentScale;
};

class UIPanel : public BaseUIComponent
{
public:
    UIPanel() : BaseUIComponent() {}

    enum PanelMode
    {
        ColourPanel, SpritePanel
    };

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) override
    {
        Vector2 wSize = {window->getSize()};

        if(mode == ColourPanel)
        {
            sf::RectangleShape rect;
            Vector2 pos = {wSize.x * percentPosition.x, wSize.y * percentPosition.y};
            Vector2 scale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
            rect.setPosition(pos.x, pos.y);
            rect.setSize({scale.x, scale.y});
            rect.setFillColor(colour);
            rect.setOutlineColor(colour);
            window->draw(rect);
            return;
        }
        int prevVertices = va->getVertexCount();
        Vector2 spriteSize = sprite.size;
        Vector2 spritePos = sprite.pos;
        Vector2 sprScale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
        Vector2 sprPos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
        va->resize(prevVertices + 4);
        sf::Vertex* quad = &va[0][prevVertices];
        quad[0].position = {sprPos.x, sprPos.y};
        quad[1].position = {sprPos.x + sprScale.x, sprPos.y};
        quad[2].position = {sprPos.x + sprScale.x, sprPos.y + sprScale.y};
        quad[3].position = {sprPos.x, sprPos.y + sprScale.y};
        quad[0].texCoords = {spritePos.x, spritePos.y};
        quad[1].texCoords = {spritePos.x + spriteSize.x, spritePos.y};
        quad[2].texCoords = {spritePos.x + spriteSize.x, spritePos.y + spriteSize.y};
        quad[3].texCoords = {spritePos.x, spritePos.y + spriteSize.y};
    }

    PanelMode mode = ColourPanel;
    sf::Color colour;
    Sprite sprite;
};

class TextRenderer : public BaseUIComponent
{
public:
    TextRenderer(String fontPath = "fonts/default.ttf") : BaseUIComponent()
    {
        if(!font.loadFromFile(fontPath))
        {
            std::cout << "Error: Invalid font" << std::endl;
            exit(-1);
        }
    }

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) override
    {
        sf::Text text1;
        text1.setCharacterSize(fontSize);
        text1.setString(this->text);
        Vector2 wSize = {window->getSize()};
        Vector2 pos = {wSize.x * percentPosition.x, wSize.y * percentPosition.y};
        text1.setPosition(pos.x, pos.y);
        text1.setFont(font);
        text1.setColor(colour);
        if(centred)
        {
            float w = text1.getLocalBounds().width;
            float h = text1.getLocalBounds().height;
            text1.setOrigin(w / 2.f, h / 2.f);
        }
        text->push_back(text1);
    }

    bool centred = true;
    String text;
    sf::Font font;
    sf::Color colour;
    unsigned int fontSize = 14;
};

class Button : public BaseUIComponent
{
public:
    // use Button(std::bind(&Class::Function, this));
    Button(std::function<void()> callback) : BaseUIComponent()
    {
        this->callback = callback;
    }

    enum ButtonMode
    {
        ColourButton, SpriteButton
    };

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) override
    {
        wSize = {window->getSize()};

        if(mode == ColourButton)
        {
            sf::RectangleShape rect;
            Vector2 pos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
            Vector2 scale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
            rect.setPosition(pos.x, pos.y);
            rect.setSize({scale.x, scale.y});
            rect.setFillColor(colour);
            rect.setOutlineColor(colour);
            window->draw(rect);
            return;
        }
        int prevVertices = va->getVertexCount();
        Vector2 spriteSize = sprite.size;
        Vector2 spritePos = sprite.pos;
        Vector2 sprScale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
        Vector2 sprPos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
        va->resize(prevVertices + 4);
        sf::Vertex* quad = &va[0][prevVertices];
        quad[0].position = {sprPos.x, sprPos.y};
        quad[1].position = {sprPos.x + sprScale.x, sprPos.y};
        quad[2].position = {sprPos.x + sprScale.x, sprPos.y + sprScale.y};
        quad[3].position = {sprPos.x, sprPos.y + sprScale.y};
        quad[0].texCoords = {spritePos.x, spritePos.y};
        quad[1].texCoords = {spritePos.x + spriteSize.x, spritePos.y};
        quad[2].texCoords = {spritePos.x + spriteSize.x, spritePos.y + spriteSize.y};
        quad[3].texCoords = {spritePos.x, spritePos.y + spriteSize.y};
    }

    void Update() override
    {
        Vector2 mousePos = input->GetMousePos();
        mousePos.x -= wSize.x / 2.f;
        mousePos.y -= wSize.y / 2.f;
        Vector2 position = Vector2(percentPosition.x * wSize.x, percentPosition.y * wSize.y);
        Vector2 scale = Vector2(percentScale.x * wSize.x / 2.f, percentScale.y * wSize.y / 2.f);
        bounds = AABB({position, scale});
        if(bounds.ContainsPoint(mousePos))
        {
            if(input->GetButtonDown(button))
            {
                (callback)();
            }
        }
    }

    ButtonMode mode = ColourButton;
    sf::Color colour = sf::Color::White;
    Sprite sprite;
    Input::MouseButton button = Input::MouseButton::Left;
    std::function<void()> callback;

private:
    Vector2 wSize;
    AABB bounds;
};

class Toggle : public BaseUIComponent
{
public:
    Toggle() : BaseUIComponent() {}

    enum ToggleMode
    {
        ColourToggle, SpriteToggle
    };

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) override
    {
        wSize = {window->getSize()};

        if(mode == ColourToggle)
        {
            sf::RectangleShape rect;
            Vector2 pos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
            Vector2 scale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
            rect.setPosition(pos.x, pos.y);
            rect.setSize({scale.x, scale.y});
            if(value)
            {
                rect.setFillColor(activeColour);
                rect.setOutlineColor(activeColour);
            }
            else
            {
                rect.setFillColor(inactiveColour);
                rect.setOutlineColor(inactiveColour);
            }
            window->draw(rect);
            return;
        }
        int prevVertices = va->getVertexCount();
        Sprite sprite;
        if(value)
        {
            sprite = activeSprite;
        }
        else
        {
            sprite = inactiveSprite;
        }
        Vector2 spriteSize = sprite.size;
        Vector2 spritePos = sprite.pos;
        Vector2 sprScale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
        Vector2 sprPos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
        va->resize(prevVertices + 4);
        sf::Vertex* quad = &va[0][prevVertices];
        quad[0].position = {sprPos.x, sprPos.y};
        quad[1].position = {sprPos.x + sprScale.x, sprPos.y};
        quad[2].position = {sprPos.x + sprScale.x, sprPos.y + sprScale.y};
        quad[3].position = {sprPos.x, sprPos.y + sprScale.y};
        quad[0].texCoords = {spritePos.x, spritePos.y};
        quad[1].texCoords = {spritePos.x + spriteSize.x, spritePos.y};
        quad[2].texCoords = {spritePos.x + spriteSize.x, spritePos.y + spriteSize.y};
        quad[3].texCoords = {spritePos.x, spritePos.y + spriteSize.y};
    }

    void Update() override
    {
        Vector2 mousePos = input->GetMousePos();
        mousePos.x -= wSize.x / 2.f;
        mousePos.y -= wSize.y / 2.f;
        Vector2 position = Vector2(percentPosition.x * wSize.x, percentPosition.y * wSize.y);
        Vector2 scale = Vector2(percentScale.x * wSize.x / 2.f, percentScale.y * wSize.y / 2.f);
        bounds = AABB({position, scale});
        if(bounds.ContainsPoint(mousePos))
        {
            if(input->GetButtonDown(button))
            {
                value = !value;
            }
        }
    }

    ToggleMode mode = ColourToggle;
    sf::Color activeColour = sf::Color::White, inactiveColour = sf::Color(0xFF888888);
    Sprite activeSprite, inactiveSprite;
    Input::MouseButton button = Input::MouseButton::Left;
    bool value;

private:
    Vector2 wSize;
    AABB bounds;
};

class TextField : public BaseUIComponent
{
public:


    TextField(String fontPath = "fonts/default.ttf") : BaseUIComponent()
    {
        if(!font.loadFromFile(fontPath))
        {
            std::cout << "Error: Invalid font" << std::endl;
            exit(-1);
        }
    }

    enum FieldMode
    {
        ColourField, SpriteField
    };

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text) override
    {
        wSize = {window->getSize()};

        if(mode == ColourField)
        {
            sf::RectangleShape rect;
            Vector2 pos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
            Vector2 scale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
            rect.setPosition(pos.x, pos.y);
            rect.setSize({scale.x, scale.y});
            if(selected)
            {
                rect.setFillColor(activeColour);
                rect.setOutlineColor(activeColour);
            }
            else
            {
                rect.setFillColor(inactiveColour);
                rect.setOutlineColor(inactiveColour);
            }
            window->draw(rect);
        }
        else
        {
            int prevVertices = va->getVertexCount();
            Sprite sprite;
            if(selected)
            {
                sprite = activeSprite;
            }
            else
            {
                sprite = inactiveSprite;
            }
            Vector2 spriteSize = sprite.size;
            Vector2 spritePos = sprite.pos;
            Vector2 sprScale = {wSize.x * percentScale.x, wSize.y * percentScale.y};
            Vector2 sprPos = {wSize.x * (percentPosition.x - percentScale.x * 0.5f), wSize.y * (percentPosition.y - percentScale.y * 0.5f)};
            va->resize(prevVertices + 4);
            sf::Vertex* quad = &va[0][prevVertices];
            quad[0].position = {sprPos.x, sprPos.y};
            quad[1].position = {sprPos.x + sprScale.x, sprPos.y};
            quad[2].position = {sprPos.x + sprScale.x, sprPos.y + sprScale.y};
            quad[3].position = {sprPos.x, sprPos.y + sprScale.y};
            quad[0].texCoords = {spritePos.x, spritePos.y};
            quad[1].texCoords = {spritePos.x + spriteSize.x, spritePos.y};
            quad[2].texCoords = {spritePos.x + spriteSize.x, spritePos.y + spriteSize.y};
            quad[3].texCoords = {spritePos.x, spritePos.y + spriteSize.y};
        }
        sf::Text text1;
        text1.setCharacterSize(fontSize);
        text1.setString(this->value);
        Vector2 wSize = {window->getSize()};
        Vector2 pos = {wSize.x * percentPosition.x, wSize.y * percentPosition.y};
        text1.setPosition(pos.x, pos.y);
        text1.setFont(font);
        text1.setColor(textColour);
        if(centred)
        {
            float w = text1.getLocalBounds().width;
            float h = text1.getLocalBounds().height;
            text1.setOrigin(w / 2.f, h / 2.f);
        }
        text->push_back(text1);
    }

    void Update() override
    {
        if(dt == dtPrev)
        {
            return;
        }
        Vector2 mousePos = input->GetMousePos();
        mousePos.x -= wSize.x / 2.f;
        mousePos.y -= wSize.y / 2.f;
        Vector2 position = Vector2(percentPosition.x * wSize.x, percentPosition.y * wSize.y);
        Vector2 scale = Vector2(percentScale.x * wSize.x / 2.f, percentScale.y * wSize.y / 2.f);
        bounds = AABB({position, scale});
        if(bounds.ContainsPoint(mousePos))
        {
            selected = true;
            std::string val = input->GetTextTyped();
            if(val.size() <= 0)
            {
                return;
                dtPrev = dt;
            }
            char c = val.at(0);
            if(c == '\b')
            {
                if(value.size() <= 0)
                {
                    dtPrev = dt;
                    return;
                }
                value.pop_back();

                dtPrev = dt;
                return;
            }
            value += c;
        }
        else selected = false;

        dtPrev = dt;
    }

    FieldMode mode = ColourField;
    sf::Color activeColour = sf::Color::White, inactiveColour = sf::Color(0x888888FF);
    Sprite activeSprite, inactiveSprite;
    Input::MouseButton button = Input::MouseButton::Left;
    bool selected = false;
    std::string value;
    bool centred = true;
    sf::Font font;
    sf::Color textColour;
    unsigned int fontSize = 14;
    float dtPrev;

private:
    Vector2 wSize;
    AABB bounds;
};

class Application;

class GameObject
{
public:
    GameObject()
    {
        components.push_back(new Transform());
        components[components.size() - 1]->_setup(camera, time, math, input, audio);
        components[components.size() - 1]->self = this;
        transform = GetComponent<Transform>();
    }

    void AddComponent(Script* component)
    {
        components.push_back(component);

        component->_setup(camera, time, math, input, audio);
        component->self = this;
        component->transform = transform;
        if(created) component->OnCreate();
        if(started) component->Start();
    }

    template <class T>
    bool HasComponent()
    {
        for(int i = 0; i < components.size(); i++)
        {
            if(dynamic_cast<T*>(components[i]) != nullptr) return true;
        }
        return false;
    }

    template <class T>
    T* GetComponent()
    {
        for(int i = 0; i < components.size(); i++)
        {
            if(dynamic_cast<T*>(components[i]) != nullptr)
            {
                return (T*)components[i];
            }
        }
        std::cout << "Error: component not found" << std::endl;
        exit(1);
    }

    void AddObject(GameObject* object)
    {
        children.push_back(object);
        object->parent = this;
        if(!setup) return;
        object->_setup(camera, time, math, input, audio);
        object->_onCreate();
        object->_start();
    }

    void RemoveObject(GameObject* object)
    {
        if(object == nullptr) return;
        auto it = children.begin();
        while(it != children.end())
        {
            if(it.base()[0] == object)
            {
                children.erase(it);
                object->_onDestroy();
            }
            it++;
        }
    }

    void Destroy()
    {
        parent->RemoveObject(this);
    }

    void _onCreate()
    {
        if(!created)
        created = true;
        for(int i = 0; i < components.size(); i++)
        {
            components[i]->OnCreate();
        }
    }

    void _onDestroy()
    {
        for(int i = 0; i < components.size(); i++)
        {
            components[i]->OnDestroy();
        }
        for(int i = 0; i < children.size(); i++)
        {
            children[i]->_onDestroy();
        }
        components.clear();
    }

    void _start()
    {
        if(!started)
        started = true;
        for(int i = 0; i < components.size(); i++)
        {
            components[i]->Start();
        }
    }

    void _update()
    {
        if(!enabled)
        {
            return;
        }
        for(int i = 0; i < components.size(); i++)
        {
            components[i]->simulated = simulated;
            components[i]->dt = dt;
            components[i]->Update();
        }
        for(int i = 0; i < children.size(); i++)
        {
            children[i]->simulated = simulated;
            children[i]->dt = dt;
            children[i]->_update();
        }
    }

    void _setup(sf::View* camera, Time* time, Math* math, Input* input, Audio* audio)
    {
        if(setup) return;
        setup = true;
        this->camera = camera;
        this->time = time;
        this->math = math;
        this->input = input;
        this->audio = audio;

        for(int i = 0; i < components.size(); i++)
        {
            components[i]->_setup(camera, time, math, input, audio);
            components[i]->OnCreate();
        }
    }

    void _qt(QuadTree* qt)
    {
        if(!enabled)
        {
            return;
        }
        Vector2 halfDim;
        if(HasComponent<SpriteRenderer>()) halfDim = GetComponent<SpriteRenderer>()->sprite.size;
        halfDim.x /= 2.f;
        halfDim.y /= 2.f;
        if(!qt->insert(this, AABB(transform->position, {halfDim.x * transform->scale.x, halfDim.y * transform->scale.y})))
        {
            std::cout << "Error: could not insert object into the quadtree" << std::endl;
            // exit(2);
        }
        for(int i = 0; i < children.size(); i++)
        {
            children[i]->_qt(qt);
        }
    }

    void _render(sf::VertexArray* va, sf::RenderWindow* window, std::vector<sf::Text>* text)
    {
        if(!enabled)
        {
            return;
        }
        if(!HasComponent<Drawable>())
        {
            return;
        }
        for(int i = 0; i < components.size(); i++)
        {
            if(dynamic_cast<Drawable*>(components[i]) != nullptr)
            {
                dynamic_cast<Drawable*>(components[i])->_render(va, window, text);
            }
        }
    }

    GameObject* parent;
    Application* app;
    sf::View* camera;
    Transform* transform;
    Time* time;
    Math* math;
    Input* input;
    Audio* audio;
    float dt;
    bool simulated;
    float _timer;
    bool enabled = true;

private:
    std::vector<GameObject*> children;
    std::vector<Script*> components;
    bool created = false, started = false, setup = false;
};

class Application
{
public:
    Application() {}

    virtual void OnCreate() {}
    virtual void OnUpdate() {}
    virtual void OnDestroy()
    {
        for(int i = 0; i < gameObjects.size(); i++)
        {
            gameObjects[i]->_onDestroy();
        }
    }

    void Create()
    {
        int style = sf::Style::Default;
        if(fullscreen)
        {
            style = sf::Style::Fullscreen;
            windowWidth = sf::VideoMode::getDesktopMode().width;
            windowHeight = sf::VideoMode::getDesktopMode().height;
        }
        if(window.isOpen()) window.close();
        window.create(sf::VideoMode(windowWidth, windowHeight), appName, style);
        window.setVerticalSyncEnabled(vsync);
        window.setFramerateLimit(frameRateLimit);
        if(!window.isOpen())
        {
            Exit(-1);
        }
        OnCreate();
        start();
    }

    // void MoveObject(GameObject* object, GameObject* newParent)
    // {
    //     if(object == nullptr) return;
    //     if(object->parent == newParent) return;
    //     if(newParent == nullptr)
    //     {
    //         if(object->parent == nullptr) return;
    //         object->parent->RemoveObject(object, false);
    //         gameObjects.push_back(object);
    //         object->parent = nullptr;
    //     }
    //     else
    //     {
    //         if(object->parent == nullptr)
    //         {
    //             auto it = gameObjects.begin();
    //             while(it != gameObjects.end())
    //             {
    //                 if(it.base()[0] == object)
    //                 {
    //                     gameObjects.erase(it);
    //                 }
    //                 it++;
    //             }
    //         }
    //         else
    //         {
    //             object->parent->RemoveObject(object, false);
    //         }
    //         newParent->children.push_back(object);
    //         object->parent = newParent;
    //     }
    // }

    void AddObject(GameObject* object)
    {
        gameObjects.push_back(object);
        object->simulated = true;
        object->parent = nullptr;
        object->app = this;
        object->_setup(camera, &time, &math, &input, &audio);
        object->_onCreate();
        object->_start();
    }

    void RemoveObject(GameObject* object, bool callOnDestroy = true)
    {
        if(object == nullptr) return;
        auto it = gameObjects.begin();
        while(it != gameObjects.end())
        {
            if(it.base()[0] == object)
            {
                gameObjects.erase(it);
                if(callOnDestroy) object->_onDestroy();
                return;
            }
            it++;
        }
        it = gameObjectsEmulated.begin();
        while(it != gameObjectsEmulated.end())
        {
            if(it.base()[0] == object)
            {
                gameObjectsEmulated.erase(it);
                if(callOnDestroy) object->_onDestroy();
                return;
            }
            it++;
        }
        
    }

    void Exit(int status = 0)
    {
        OnDestroy();
        window.close();
        std::exit(status);
    }

    std::string appName;
    std::string spriteFilePath = "default.png";
    unsigned int windowWidth = 640, windowHeight = 480, frameRateLimit = 60;
    bool fullscreen = false, vsync = true, showFps = true;
    sf::Color bgColour = sf::Color::Black;
    AABB simulationDistance = AABB({0, 0}, {windowWidth, windowHeight});
    // the time between updates for objects outside of the simulation distance
    float emulatedTargetDeltaTime = 1.f;
    // the time between updates for objects inside of the simulation distance
    float simulatedTargetDeltaTime = (1 / 60.f);

    Time time;
    Math math;
    Input input;
    Audio audio;

    sf::VertexArray tmpva;

private:
    sf::RenderWindow window;
    std::vector<GameObject*> gameObjects;
    std::vector<GameObject*> gameObjectsSimulated;
    std::vector<GameObject*> gameObjectsEmulated;
    QuadTree* qt;
    sf::View* camera;
    sf::Texture tex;
    bool texLoaded = false;
    float simulatedInterval = 0, simulatedTimer = 0; int simulatedIT = 0;
    float emulatedInterval = 0, emulatedTimer = 0; int emulatedIT = 0;
    float targetFPS = 60.0f;

    void start()
    {
        if(spriteFilePath != "default.png")
        {
            if(!tex.loadFromFile(spriteFilePath))
            {
                std::cout << "Error: failed to load sprite atlas from " << spriteFilePath << std::endl;
                Exit(-1);
            }
            texLoaded = true;
        }
        sf::Clock clock;
        sf::Time t;
        float lastTime = 0;
        time.deltaTime = 1;
        float timer = 0;
        float refreshTimer = 0;
        float frameTimer = 0;
        float fpsLast = 0;
        qt = new QuadTree({{0, 0}, {100000, 100000}});
        input._setup();

        camera = new sf::View({0, 0}, {windowWidth, windowHeight});

        while(window.isOpen())
        {
            //quadtree manager
            if(refreshTimer >= simulatedTargetDeltaTime)
            {
                qt->clear();
                for(int i = 0; i < gameObjects.size(); i++)
                {
                    if(gameObjects[i]->enabled) gameObjects[i]->_qt(qt);
                }
                for(int i = 0; i < gameObjects.size(); i++)
                {
                    gameObjects[i]->simulated = false;
                }
                gameObjectsSimulated = qt->queryRange(simulationDistance);
                for(int i = 0; i < gameObjectsSimulated.size(); i++)
                {
                    gameObjectsSimulated[i]->simulated = true;
                }
                for(int i = 0; i < gameObjects.size(); i++)
                {
                    if(!gameObjects[i]->simulated && gameObjects[i]->enabled) gameObjectsEmulated.push_back(gameObjects[i]);
                }
                refreshTimer -= simulatedTargetDeltaTime;
            }


            //event handler
            if(simulatedTimer >= simulatedTargetDeltaTime)
            {
                input._update();
                sf::Event event;
                while(window.pollEvent(event))
                {
                    if(event.type == sf::Event::Closed)
                    {
                        Exit();
                    }
                    input._poll(event);
                }
            }


            //collision handler
            std::vector<GameObject*> objsToCheck = qt->queryRange(simulationDistance);
            std::vector<GameObject*> rbs;
            for(int i = 0; i < objsToCheck.size(); i++)
            {
                if(objsToCheck[i]->HasComponent<RigidBody>())
                {
                    rbs.push_back(objsToCheck[i]);
                }
            }
            for(int i = 0; i < rbs.size(); i++)
            {
                RigidBody* rb = rbs[i]->GetComponent<RigidBody>();
                Vector2 pos = {rb->transform->position.x + rb->collider->Centre.x, rb->transform->position.y + rb->collider->Centre.y};
                Vector2 size = {rb->transform->scale.x * rb->collider->Scale.x, rb->transform->scale.y * rb->collider->Scale.y};
                std::vector<GameObject*> broadPhaseCheck = qt->queryRange({pos, size});
                std::vector<Collider*> colliders;
                for(int i = 0; i < broadPhaseCheck.size(); i++)
                {
                    if(broadPhaseCheck[i]->HasComponent<Collider>())
                    {
                        colliders.push_back(broadPhaseCheck[i]->GetComponent<Collider>());
                    }
                }
                rb->_checkCollisions(colliders);
            }


            //update system
            if(gameObjects.size() > 0) simulatedInterval = simulatedTargetDeltaTime / gameObjects.size();
            if(gameObjectsEmulated.size() > 0) emulatedInterval = emulatedTargetDeltaTime / gameObjectsEmulated.size();
            simulationDistance.center = camera->getCenter();
            OnUpdate();
            for(int i = 0; i < gameObjects.size(); i++)
            {
                if(gameObjects[i]->enabled) gameObjects[i]->_timer += time.deltaTime;
            }
            if(gameObjectsSimulated.size() > 0)
            {
                while(simulatedTimer >= simulatedInterval)
                {
                    if(simulatedIT >= gameObjects.size()) simulatedIT = 0;
                    gameObjectsSimulated[simulatedIT]->dt = gameObjects[simulatedIT]->_timer;
                    gameObjectsSimulated[simulatedIT]->_timer = 0;
                    gameObjectsSimulated[simulatedIT]->_update();
                    simulatedIT++;
                    simulatedTimer -= simulatedInterval;
                }
            }
            if(gameObjectsEmulated.size() > 0)
            {
                while(emulatedTimer > emulatedInterval)
                {
                    if(emulatedIT >= gameObjectsEmulated.size()) emulatedIT = 0;
                    gameObjectsEmulated[emulatedIT]->dt = gameObjectsEmulated[emulatedIT]->_timer;
                    gameObjectsEmulated[emulatedIT]->_timer = 0;
                    gameObjectsEmulated[emulatedIT]->_update();
                    emulatedIT++;
                    emulatedTimer -= emulatedInterval;
                }
            }


            //rendering
            if(frameTimer >= (1.f / targetFPS))
            {
                window.setView(*camera);
                window.clear(bgColour);
                sf::VertexArray* va = new sf::VertexArray(sf::Quads, 0);
                std::vector<sf::Text>* text = new std::vector<sf::Text>();
                std::vector<GameObject*> renderedObjects = qt->queryRange({camera->getCenter(), {camera->getSize()}});
                for(int i = 0; i < renderedObjects.size(); i++)
                {
                    renderedObjects[i]->_render(va, &window, text);
                }
                if(va->getVertexCount() > 0)
                {
                    sf::RenderStates state = sf::RenderStates::Default;
                    if(texLoaded)
                    {
                        state.texture = &tex;
                    }
                    window.draw(&va[0][0], va->getVertexCount(), sf::Quads, state);
                    std::cout << renderedObjects.size() << " | " << va->getVertexCount() << " | " << qt->getCountAll() << "\n";

                    tmpva = *va;
                }
                for(int i = 0; i < text->size(); i++)
                {
                    window.draw(text->at(i));
                }
                window.display();
                delete va;
                delete text;
                frameTimer -= (1.f / targetFPS);
            }


            //time manager
            t = clock.getElapsedTime();
            time.time = t.asSeconds();
            time.deltaTime = time.time - lastTime;
            lastTime = time.time;
            time.frameRate = 1.f / time.deltaTime;
            refreshTimer += time.deltaTime;
            simulatedTimer += time.deltaTime;
            emulatedTimer += time.deltaTime;
            frameTimer += time.deltaTime;

            timer += time.deltaTime;
            if(timer >= 0.5)
            {
                timer = 0;
                if(showFps)
                {
                    int fps = (int)((time.frameRate + fpsLast) / 2.f);
                    window.setTitle(appName + " | " + std::to_string(fps) + " FPS");
                }
                else
                {
                    window.setTitle(appName);
                }
                fpsLast = time.frameRate;
            }
            
        }
    }
};

#endif