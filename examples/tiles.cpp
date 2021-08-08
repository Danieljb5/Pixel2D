#include "p2d.hpp"

class Tiles : public p2d::Application
{
public:
    Tiles()
    {
        appName = "Tiles";
    }

    void onCreate() override
    {
        const int level[] =
        {
            0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,
            0, 1, 0, 0, 2, 0, 3, 3, 3, 0, 1, 1, 1, 0, 0, 0,
            0, 1, 1, 0, 3, 3, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0,
            0, 0, 1, 0, 3, 0, 2, 2, 0, 0, 1, 1, 1, 1, 2, 0,
            2, 0, 1, 0, 3, 0, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1,
            0, 0, 1, 0, 3, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1,
        };

        if(!map.load("textures/exampleTileset.png", {32, 32}, level, 16, 8)) exit(-1);
        map.updateBounds(0, 0, 16, 8);
    }

    void onUpdate() override
    {
        clear();
        drawTileMap(map);
    }

    p2d::TileMap map;
};

int main()
{
    Tiles app;
    if(app.create())
    {
        app.start();
    }
    return 0;
}
