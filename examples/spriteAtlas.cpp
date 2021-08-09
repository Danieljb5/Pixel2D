#include "p2d.hpp"

// this file is both an example of how to use the sprite atlas, as well as a simple benchmark to test how many sprites a machine can render with this library

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
                spr->setAtlasTexture(tex);
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

    p2d::atlasTexture tex = {0, 0, 218, 153};
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
