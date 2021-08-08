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
        //init
    }

    void onUpdate() override
    {
        //game logic
    }
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
