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
        char* message = "Hello, World!";
        p2d::save save;
        save.putString(message);
        assets.saveData("TestSave.sav", save);
        
        loadFont("fonts/Roboto-Medium.ttf");
    }

    void onUpdate() override
    {
        clear();
        drawString(0, 0, "Saving message 'Hello, World!' to 'TestSave.sav'", 24);
    }
};

int main()
{
    Example app;
    if(app.create())
    {
        app.start();
    }
    return 0;
}
