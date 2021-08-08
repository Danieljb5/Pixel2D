#include "p2dNetwork.hpp"
#include "exampleNetworkSharedInfo.hpp"

class CustomServer : public p2d::net::server_interface<MsgTypes>
{
public:
    CustomServer(uint16_t port) : p2d::net::server_interface<MsgTypes>(port)
    {
        
    }

    void checkClients()
    {
        p2d::net::message<MsgTypes> msg;
        msg.header.id = MsgTypes::ConnectionsCheck;
        messageAllClients(msg);
    }
    
protected:
    bool onClientConnect(std::shared_ptr<p2d::net::connection<MsgTypes>> client) override
    {
        return true;
    }

    void onClientDisconnect(std::shared_ptr<p2d::net::connection<MsgTypes>> client) override
    {
        std::cout << "Removing client [" << client->getID() << "]\n";
    }

    void onMessage(std::shared_ptr<p2d::net::connection<MsgTypes>> client, p2d::net::message<MsgTypes> &msg) override
    {
        switch (msg.header.id)
        {
        case MsgTypes::ServerPing:
            std::cout << "[" << client->getID() << "] Server Ping\n";
            messageClient(client, msg);
        }
    }
};

int main()
{
    CustomServer server = {60000};
    server.start();
    p2d::Time time;
    bool checkClients = false;

    while(true)
    {
        server.update(-1, true);

        int t = time.time();

        if(t % 60 >= 59 && !checkClients) checkClients = true;

        if(t % 60 <= 1 && checkClients)
        {
            checkClients = false;
            server.checkClients();
        }
    }
    
    return 0;
}
