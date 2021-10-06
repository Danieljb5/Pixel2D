#include "p2d.hpp"

enum MsgTypes : uint32_t
{
    ServerAccept, ServerDeny, ServerPing, MessageAll, ServerMessage, ConnectionsCheck
};

class CustomClient : public client_interface<MsgTypes>
{
public:
    CustomClient() : client_interface() {}

    void pingServer(float time)
    {
        Message<MsgTypes> msg;
        msg.header.id = MsgTypes::ServerPing;
        msg << time;
        sendMessage(msg);
    }
};

class ExampleApp : public Application
{
public:
    ExampleApp()
    {
        appName = "Client";
        spriteFilePath = "textures/exampleSprite.png";
    }

    void OnCreate() override
    {
        client.connect("127.0.0.1", 60000);
    }

    void OnUpdate() override
    {
        if(input.GetKeyDown(Input::Key::P)) client.pingServer(time.time);

        if(client.isConnected())
        {
            if(!client.incoming().empty())
            {
                auto msg = client.incoming().pop_front().msg;

                switch(msg.header.id)
                {
                    case MsgTypes::ServerPing:
                    {
                        float timeNow = time.time;
                        float timeThen;
                        msg >> timeThen;
                        std::cout << "Ping: " << (timeNow - timeThen) << "\n";
                    }
                }
            }
        }
        else
        {
            std::cout << "Server Down\n";
            Exit();
        }
    }

    void OnDestroy() override
    {
        client.disconnect();
    }

    CustomClient client;
};

int main()
{
    ExampleApp app;
    app.frameRateLimit = 99999;
    app.vsync = false;
    app.showFps = true;
    app.Create();
    return 0;
}