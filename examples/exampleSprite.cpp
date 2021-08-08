#include "p2d.hpp"

class Example : public p2d::Application
{
public:
    Example()
    {
        appName = "Example Sprite";
    }

    void onCreate() override
    {
        int sprID = assets.load("textures/exampleSprite.png", p2d::Assets::texture);
        spr = assets.getSprite(sprID);
        spr.setX(0);
        spr.setY(0);
        spr.setWidth(218);
        spr.setHeight(153);
        x = screenWidth() / 2.f; y = screenHeight() / 2.f;
        vx = 1; vy = 1;
    }

    void onUpdate() override
    {
        clear();
        x += vx;
        y += vy;

        if(x <= 0 || x + spr.getWidth() >= screenWidth()) vx *= -1;
        if(y <= 0 || y + spr.getHeight() >= screenHeight()) vy *= -1;

        spr.setX(x);
        spr.setY(y);
        drawSprite(spr);
    }

    p2d::sprite spr;
    float x, y;
    float vx, vy;
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
