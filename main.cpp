#include "p2d.hpp"

class Main : public p2d::Application
{
public:
    Main()
    {
        appName = "Main";
    }

    void onCreate() override
    {
        loadFont("assets/Roboto-Medium.ttf");
    }

    void onUpdate() override
    {
        if(input.getButton(0))
        {
            numSprites++;
            xs.push_back(screenWidth() / 2.f);
            ys.push_back(screenHeight() / 2.f);
            vxs.push_back(100);
            vys.push_back(100);
            colours.push_back(util.random());
        }

        clear();

        for(int i = 0; i < numSprites; i++)
        {
            float x = xs.at(i);
            float y = ys.at(i);
            float vx = vxs.at(i); 
            float vy = vys.at(i);

            x += (vx * time.dt);
            y += (vy * time.dt);

            if(x <= 0 || x + 100 >= screenWidth()) vx *= -1;
            if(y <= 0 || y + 100 >= screenHeight()) vy *= -1;

            drawRect(x, y, 100, 100, 1, colours.at(i));

            xs[i] = x;
            ys[i] = y;
            vxs[i] = vx;
            vys[i] = vy;
        }

        std::string msg = " Draw Calls: " + std::to_string(getDrawCalls() + 1);
        drawString(5, 5, msg, 16, colour.Red, topLeft, false);
    }

    std::vector<float> xs;
    std::vector<float> ys;
    std::vector<float> vxs;
    std::vector<float> vys;
    std::vector<uint32_t> colours;
    int numSprites = 0;
    int sprID;
};

int main()
{
    Main app;
    if(app.create(640, 480, 99999, SHOW_FPS, 100000, false))
    {
        app.start();
    }
    return 0;
}
