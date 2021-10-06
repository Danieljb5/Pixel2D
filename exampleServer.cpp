#include "p2d.hpp"

enum MsgTypes : uint32_t
{
    ServerAccept, ServerDeny, ServerPing, MessageAll, ServerMessage, ConnectionsCheck
};

class CustomServer : public server_interface<MsgTypes>
{
public:
    CustomServer(uint16_t port) : server_interface<MsgTypes>(port)
    {

    }

    void checkClients()
    {
        Message<MsgTypes> msg;
        msg.header.id = MsgTypes::ConnectionsCheck;
        messageAllClients(msg);
    }

protected:
    bool onClientConnect(std::shared_ptr<Connection<MsgTypes>> client) override
    {
        return true;
    }

    void onClientDisconnect(std::shared_ptr<Connection<MsgTypes>> client) override
    {
        std::cout << "Removing client [" << client->getID() << "]\n";
    }

    void onMessage(std::shared_ptr<Connection<MsgTypes>> client, Message<MsgTypes> &msg) override
    {
        switch(msg.header.id)
        {
            case MsgTypes::ServerPing:
            {
                std::cout << "[" << client->getID() << "] Server Ping\n";
                messageClient(client, msg);
            }
        }
    }
};

int main()
{
    CustomServer server = {60000};
    server.start();
    while(true)
    {
        server.update(-1, true);
    }
    return 0;
}