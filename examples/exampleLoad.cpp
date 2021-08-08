#include "p2d.hpp"

class Example : public p2d::Application
{
public:
    Example()
    {
        appName = "Example";
    }

    void onCreate() override
    {
        p2d::save save;
        save = assets.loadSave("TestSave.sav");
        message = save.getString();
        console.log(std::to_string(message.length()).c_str());
        console.log(message.c_str());

        loadFont("fonts/Roboto-Medium.ttf");
    }

    void onUpdate() override
    {
        clear();
        drawString(0, 0, std::string("Loaded message '" + message + "' from 'TestSave.sav'"), 24);
    }
    
    std::string message;
};

int main()
{
    Example app;
    if(app.create()) // leaving create() empty sets up a window with the default parameters
    {
        app.start();
    }
    return 0;
}
