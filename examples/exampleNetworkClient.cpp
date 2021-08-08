#include "p2dNetwork.hpp"
#include "exampleNetworkSharedInfo.hpp"

class CustomClient : public p2d::net::client_interface<MsgTypes>
{
public:
    void pingServer(float time)
    {
        p2d::net::message<MsgTypes> msg;
        msg.header.id = MsgTypes::ServerPing;
        msg << time;
        sendMessage(msg);
    }
};

class App : public p2d::Application
{
public:
    App()
    {
        appName = "Example Network Client";
    }

    void onCreate() override
    {
        client.connect("127.0.0.1", 60000);
    }

    void onUpdate() override
    {
        if(input.getKeyDown(keyboard.P)) client.pingServer(time.time());

        if(client.isConnected())
        {
            if(!client.incoming().empty())
            {
                auto msg = client.incoming().pop_front().msg;

                switch (msg.header.id)
                {
                case MsgTypes::ServerPing:
                    float timeNow = time.time();
                    float timeThen;
                    msg >> timeThen;
                    std::cout << "Ping: " << (timeNow - timeThen) << "\n";
                }
            }
        }
        else
        {
            std::cout << "Server Down\n";
            exit();
        }

    }

    void onDestroy() override
    {
        client.disconnect();
    }

    CustomClient client;
    bool key[3] = {false, false, false};
};

int main()
{
    App app;
    if(app.create())
    {
        app.start();
    }
    return 0;
}