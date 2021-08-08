#include "p2d.hpp"

class Example : public p2d::Application
{
public:
    Example()
    {
        appName = "Example Sprite Atlas";
    }

    void onCreate() override
    {
        if(!loadFont("fonts/Roboto-Medium.ttf"))
        {
            console.logError("Failed loading font");
        }
        if(!spriteBuffer.loadAtlas("textures/exampleAtlas.png"))
        {
            console.logError("Failed loading sprite atlas");
        }
    }

    void onUpdate() override
    {
        if(frameRate() > 60 && canAddSprites)
        {
            numSprites += 10;
        }

        if(canAddSprites)
        {
            while(j < numSprites)
            {
                if(!canAddSprites) break;
                vxs.push_back(100);
                vys.push_back(100);
                colours.push_back(util.random());
                p2d::atlasSprite* spr = new p2d::atlasSprite();
                spr->setAtlasPos(0, 0);
                spr->setAtlasSize(218, 153);
                spr->setSize(218, 153);
                spr->setPos(util.randomFloat(0, screenWidth() - 218), util.randomFloat(0, screenHeight() - 153));
                sprites.push_back(spr);
                canAddSprites = spriteBuffer.insert(sprites[j]);
                j++;
            }
        }

        if(input.getButton(0) && canAddSprites)
        {
            numSprites++;
        }

        clear();

        // for(int i = 0; i < sprites.size(); i++)
        // {
        //     float x = sprites[i]->getX();
        //     float y = sprites[i]->getY();
        //     float vx = vxs.at(i); 
        //     float vy = vys.at(i);
        //     if(vx == 0) vx = 100;
        //     if(vy == 0) vy = 100;

        //     x += (vx * time.dt);
        //     y += (vy * time.dt);

        //     if(x <= 0 || x + 218 >= screenWidth()) vx *= -1;
        //     if(y <= 0 || y + 153 >= screenHeight()) vy *= -1;
        //     if(x < 0) x = 0;
        //     if(x >= screenWidth()) x = screenWidth() - 1;
        //     if(y < 0) y = 0;
        //     if(y >= screenHeight()) y = screenHeight() - 1;

        //     sprites[i]->setX(x);
        //     sprites[i]->setY(y);
        //     vxs[i] = vx;
        //     vys[i] = vy;
        // }

        appName = "Example Sprite Atlas | Sprites: " + std::to_string(numSprites);
    }

    std::vector<float> vxs;
    std::vector<float> vys;
    std::vector<p2d::atlasSprite*> sprites;
    std::vector<uint32_t> colours;
    int numSprites = 1000;
    bool canAddSprites = true;
    int j = 0;
    int sprID;
};

int main()
{
    Example app;
    if(app.create(640, 480, 1000, SHOW_FPS))
    {
        app.start();
    }
    return 0;
}
