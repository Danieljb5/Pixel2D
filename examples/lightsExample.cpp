#include "p2d.hpp"

//Basic Cellmap system
struct edge
{
    float sx, sy;
    float ex, ey;
};

struct cell
{
    int edgeID[4];
    bool edgeExist[4];
    bool exists = false;
};

#define CELL_NORTH 0
#define CELL_SOUTH 1
#define CELL_EAST 2
#define CELL_WEST 3

class CellMap
{
public:
    CellMap(int width, int height)
    {
        worldWidth = width;
        worldHeight = height;
        world = new cell[worldWidth * worldHeight];
    }

    cell* world;
    int worldWidth;
    int worldHeight;

    std::vector<edge> edges;

    std::vector<std::tuple<float, float, float>> visibilityPolygonPoints;

    void calculateVisibilityPolygon(float ox, float oy, float radius)
    {
        visibilityPolygonPoints.clear();

        for(auto &e1 : edges)
        {
            for(int i = 0; i < 2; i++)
            {
                float rdx, rdy;
                rdx = (i == 0 ? e1.sx : e1.ex) - ox;
                rdy = (i == 0 ? e1.sy : e1.ey) - oy;

                float baseAng = atan2f(rdy, rdx);
                float ang = 0;

                for(int j = 0; j < 3; j++)
                {
                    if(j == 0) ang = baseAng - 0.0001f;
                    if(j == 1) ang = baseAng;
                    if(j == 2) ang = baseAng + 0.0001f;

                    rdx = radius * cosf(ang);
                    rdy = radius * sinf(ang);

                    float minT1 = INFINITY;
                    float minPx = 0, minPy = 0, minAng = 0;
                    bool valid = false;

                    for(auto &e2 : edges)
                    {
                        float sdx = e2.ex - e2.sx;
                        float sdy = e2.ey - e2.sy;

                        if(fabs(sdx - rdx) > 0.0f && fabs(sdy - rdy) > 0.0f)
                        {
                            float t2 = (rdx * (e2.sy - oy) + (rdy * (ox - e2.sx))) / (sdx * rdy - sdy * rdx);
                            float t1 = (e2.sx + sdx * t2 - ox) / rdx;

                            if(t1 > 0 && t2 >= 0 && t2 <= 1.0f)
                            {
                                if(t1 < minT1)
                                {
                                    minT1 = t1;
                                }
                            }
                        }
                    }
                    if(minT1 < INFINITY)
                    {
                        minPx = ox + rdx * minT1;
                        minPy = oy + rdy * minT1;
                        minAng = atan2f(minPy - oy, minPx - ox);
                        valid = true;
                    }
                    if(valid) visibilityPolygonPoints.push_back({minAng, minPx, minPy});
                }
            }
        }

        std::sort(
            visibilityPolygonPoints.begin(),
            visibilityPolygonPoints.end(),
            [&](const std::tuple<float, float, float> &t1, const std::tuple<float, float, float> &t2)
            {
                return std::get<0>(t1) < std::get<0>(t2);
            }
        );
        


    }

    void convertTileMapToVectorMap(int w, int h, float blockWidth)
    {
        edges.clear();

        for(int x = 0; x < w; x++)
        {
            for(int y = 0; y < h; y++)
            {
                for(int i = 0; i < 4; i++)
                {
                    world[y * w + x].edgeExist[i] = false;
                    world[y * w + x].edgeID[i] = 0;
                }
            }
        }


        for(int x = 1; x < w - 1; x++)
        {
            for(int y = 1; y < h - 1; y++)
            {
                int i = (y * w) + x;
                int ne = ((y - 1) * w) + x;
                int se = ((y + 1) * w) + x;
                int we = (y * w) + (x - 1);
                int ee = (y * w) + (x + 1);

                if(world[i].exists)
                {
                    if(!world[we].exists)
                    {
                        if(world[ne].edgeExist[CELL_WEST])
                        {
                            edges[world[ne].edgeID[CELL_WEST]].ey += blockWidth;
                            world[i].edgeID[CELL_WEST] = world[ne].edgeID[CELL_WEST];
                            world[i].edgeExist[CELL_WEST] = true;
                        }
                        else
                        {
                            edge e;
                            e.sx = x * blockWidth;
                            e.sy = y * blockWidth;
                            e.ex = e.sx;
                            e.ey = e.sy + blockWidth;

                            int eID = edges.size();
                            edges.push_back(e);

                            world[i].edgeID[CELL_WEST] = eID;
                            world[i].edgeExist[CELL_WEST] = true;
                        }
                    }
                    if(!world[ee].exists)
                    {
                        if(world[ne].edgeExist[CELL_EAST])
                        {
                            edges[world[ne].edgeID[CELL_EAST]].ey += blockWidth;
                            world[i].edgeID[CELL_EAST] = world[ne].edgeID[CELL_EAST];
                            world[i].edgeExist[CELL_EAST] = true;
                        }
                        else
                        {
                            edge e;
                            e.sx = x * blockWidth + blockWidth;
                            e.sy = y * blockWidth;
                            e.ex = e.sx;
                            e.ey = e.sy + blockWidth;

                            int eID = edges.size();
                            edges.push_back(e);

                            world[i].edgeID[CELL_EAST] = eID;
                            world[i].edgeExist[CELL_EAST] = true;
                        }
                    }
                    if(!world[ne].exists)
                    {
                        if(world[we].edgeExist[CELL_NORTH])
                        {
                            edges[world[we].edgeID[CELL_NORTH]].ex += blockWidth;
                            world[i].edgeID[CELL_NORTH] = world[we].edgeID[CELL_NORTH];
                            world[i].edgeExist[CELL_NORTH] = true;
                        }
                        else
                        {
                            edge e;
                            e.sx = x * blockWidth;
                            e.sy = y * blockWidth;
                            e.ex = e.sx + blockWidth;
                            e.ey = e.sy;

                            int eID = edges.size();
                            edges.push_back(e);

                            world[i].edgeID[CELL_NORTH] = eID;
                            world[i].edgeExist[CELL_NORTH] = true;
                        }
                    }
                    if(!world[se].exists)
                    {
                        if(world[we].edgeExist[CELL_SOUTH])
                        {
                            edges[world[we].edgeID[CELL_SOUTH]].ex += blockWidth;
                            world[i].edgeID[CELL_SOUTH] = world[we].edgeID[CELL_SOUTH];
                            world[i].edgeExist[CELL_SOUTH] = true;
                        }
                        else
                        {
                            edge e;
                            e.sx = x * blockWidth;
                            e.sy = y * blockWidth + blockWidth;
                            e.ex = e.sx + blockWidth;
                            e.ey = e.sy;

                            int eID = edges.size();
                            edges.push_back(e);

                            world[i].edgeID[CELL_SOUTH] = eID;
                            world[i].edgeExist[CELL_SOUTH] = true;
                        }
                    }
                }
            }
        }
    }
};



class Example : public p2d::Application
{
public:
    Example()
    {
        appName = "Lighting example";
    }

public:
    CellMap map = {40, 30};

    void onCreate() override
    {
        map.world = new cell[40 * 30];

        for(int x = 1; x < 39; x++)
        {
            map.world[40 + x].exists = true;
            map.world[28 * 40 + x].exists = true;
        }
        for(int x = 1; x < 29; x++)
        {
            map.world[x * 40 + 1].exists = true;
            map.world[x * 40 + 38].exists = true;
        }

        map.convertTileMapToVectorMap(40, 30, 16);

        loadFont("fonts/Roboto-Medium.ttf");
    }

    void onUpdate() override
    {
        float blockWidtdh = 16.f;
        float mouseX = input.getMouseX();
        float mouseY = input.getMouseY();

        if(input.getButtonUp(mouse.Left))
        {
            int i = ((int)mouseY / (int)blockWidtdh) * 40 + ((int)mouseX / (int)blockWidtdh);
            map.world[i].exists = !map.world[i].exists;
            map.convertTileMapToVectorMap(40, 30, 16);
        }

        if(input.getButton(1))
        {
            map.calculateVisibilityPolygon(mouseX, mouseY, 1000.f);
        }

        clear();

        int raysCast = map.visibilityPolygonPoints.size();

        auto it = std::unique(
            map.visibilityPolygonPoints.begin(),
            map.visibilityPolygonPoints.end(),
            [&](const std::tuple<float, float, float> &t1, const std::tuple<float, float, float> &t2)
            {
                return fabs(std::get<1>(t1) - std::get<1>(t2)) < 0.1f && fabs(std::get<2>(t1) - std::get<2>(t2)) < 0.1f;
            }
        );

        map.visibilityPolygonPoints.resize(std::distance(map.visibilityPolygonPoints.begin(), it));

        int raysCast2 = map.visibilityPolygonPoints.size();

        drawString(2, 2, "Rays Cast: " + std::to_string(raysCast) + " Rays drawn: " + std::to_string(raysCast2));


        if(input.getButton(mouse.Right) && map.visibilityPolygonPoints.size() > 1)
        {
            for(int i = 0; i < (int)map.visibilityPolygonPoints.size() - 1; i++)
            {
                fillTriangle(
                    mouseX,
                    mouseY,
                    std::get<1>(map.visibilityPolygonPoints[i]),
                    std::get<2>(map.visibilityPolygonPoints[i]),
                    std::get<1>(map.visibilityPolygonPoints[i + 1]),
                    std::get<2>(map.visibilityPolygonPoints[i + 1]));
            }

            fillTriangle(
                    mouseX,
                    mouseY,
                    std::get<1>(map.visibilityPolygonPoints[map.visibilityPolygonPoints.size() - 1]),
                    std::get<2>(map.visibilityPolygonPoints[map.visibilityPolygonPoints.size() - 1]),
                    std::get<1>(map.visibilityPolygonPoints[0]),
                    std::get<2>(map.visibilityPolygonPoints[0]));
        }

        for(int x = 0; x < 40; x++)
        {
            for(int y = 0; y < 30; y++)
            {
                if(map.world[y * 40 + x].exists)
                {
                    fillRect(x * 16, y * 16, 16, 16, 1, colour.Blue, colour.Blue);
                }
            }
        }

        // for(auto &e : map.edges)
        // {
        //     drawLine(e.sx, e.sy, e.ex, e.ey);
        //     fillCircle(e.sx, e.sy, 3, colour.Red, colour.Red, p2d::center);
        //     fillCircle(e.ex, e.ey, 3, colour.Red, colour.Red, p2d::center);
        // }
    }
};

int main()
{
    Example example;
    if(example.create(640, 480, 99999, SHOW_FPS, 1000, false))
    {
        example.start();
    }
    return 0;
}
