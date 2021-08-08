#include "p2d.hpp"

class CircleVsRect : public p2d::Application
{
public:
    CircleVsRect()
    {
        appName = "Circle Vs Rectangle";
    }

    void onCreate() override
    {
        object.pos = {3, 3};
    }

    void onUpdate() override
    {
        if(input.getKeyUp(keyboard.Space)) followObject = !followObject;

        object.vel = {0, 0};
        if(input.getKey(keyboard.W)) object.vel += {0, -1};
        if(input.getKey(keyboard.S)) object.vel += {0, 1};
        if(input.getKey(keyboard.A)) object.vel += {-1, 0};
        if(input.getKey(keyboard.D)) object.vel += {1, 0};
        sf::Vector2f nextPos;

        sf::Vector2i areaTL = {0, 0};
        sf::Vector2i areaBR = {0, 0};
        if(object.vel.x != 0 || object.vel.y != 0)
        {
            nextPos = object.pos + (mathf.norm(object.vel) * time.dt * (input.getKey(keyboard.LShift) ? 5.0f : 2.0f));
        }
        else
        {
            nextPos = object.pos;
        }
        
        sf::Vector2i currentCell = (sf::Vector2i)(object.pos - (sf::Vector2f){0.5f, 0.5f});
        sf::Vector2i targetCell = (sf::Vector2i)nextPos;
        areaTL = (sf::Vector2i)mathf.max((mathf.min((sf::Vector2f)currentCell, (sf::Vector2f)targetCell) - sf::Vector2f(1, 1)), sf::Vector2f(0, 0));
        areaBR = (sf::Vector2i)mathf.min((mathf.max((sf::Vector2f)currentCell, (sf::Vector2f)targetCell) + sf::Vector2f(2, 2)), (sf::Vector2f)worldSize);

        sf::Vector2i cell;
        for(cell.y = areaTL.y; cell.y < areaBR.y; cell.y++)
        {
            for(cell.x = areaTL.x; cell.x < areaBR.x; cell.x++)
            {
                if(worldMap[cell.y * worldSize.x + cell.x] == '#')
                {
                    sf::Vector2f nearestPoint;
                    nearestPoint.x = std::max(float(cell.x), std::min(nextPos.x, float(cell.x + 1)));
                    nearestPoint.y = std::max(float(cell.y), std::min(nextPos.y, float(cell.y + 1)));
                    sf::Vector2f rayToNearest = nearestPoint - nextPos;
                    float overlap = object.radius - mathf.mag(rayToNearest);
                    if(std::isnan(overlap)) overlap = 0;
                    overlap -= 15.5;
                    if(overlap > 0)
                    {
                        nextPos = nextPos - mathf.norm(rayToNearest) * overlap;
                        std::cout << overlap << "\n";
                    }
                }
            }
        }

        object.pos = nextPos;

        clear(colour.DarkBlue);

        if(!followObject)
        {
            if(input.getButtonDown(2)) startPan();
            if(input.getButton(2))
            {
                pan(input.getMouseX(), input.getMouseY());
            }
        }
        else
        {
            setCamPos(object.pos.x * 32, object.pos.y * 32);
        }
        setCamZoom(-input.getDeltaScroll() * time.dt * 10);

        sf::Vector2i tl = (sf::Vector2i)((camera.getCenter() - (camera.getSize() / 2.f)) / 32.f);
        sf::Vector2i br = (sf::Vector2i)((camera.getCenter() + (camera.getSize() / 2.f + (sf::Vector2f){32, 32})) / 32.f);
        tl.x = mathf.max(0, tl.x);
        tl.y = mathf.max(0, tl.y);
        br.x = mathf.min(worldSize.x, br.x);
        br.y = mathf.min(worldSize.y, br.y);
        sf::Vector2i tilePos;
        for(tilePos.y = tl.y; tilePos.y < br.y; tilePos.y++)
        {
            for(tilePos.x = tl.x; tilePos.x < br.x; tilePos.x++)
            {
                if(worldMap[tilePos.y * worldSize.x + tilePos.x] == '#')
                {
                    fillRect(tilePos.x * 32, tilePos.y * 32, 32, 32, 0);
                }
            }
        }

        fillRect(areaTL.x * 32, areaTL.y * 32, (areaBR.x - areaTL.x) * 32, (areaBR.y - areaTL.y) * 32, 0, colour.fromRGBA(0, 255, 255, 32));

        fillCircle(object.pos.x * 32, object.pos.y * 32, object.radius, 1, colour.Red, colour.Red, center);
    }

    struct worldObject
    {
        sf::Vector2f pos = {0, 0};
        sf::Vector2f vel = {0, 0};
        float radius = 16.f;
    };

    worldObject object;

    std::string worldMap = 
        	"################################"
		"#..............................#"
		"#.......#####.#.....#####......#"
		"#.......#...#.#.....#..........#"
		"#.......#...#.#.....#..........#"
		"#.......#####.#####.#####......#"
		"#..............................#"
		"#.....#####.#####.#####..##....#"
		"#.........#.#...#.....#.#.#....#"
		"#.....#####.#...#.#####...#....#"
		"#.....#.....#...#.#.......#....#"
		"#.....#####.#####.#####.#####..#"
		"#..............................#"
		"#..............................#"
		"#..#.#..........#....#.........#"
		"#..#.#..........#....#.........#"
		"#..#.#.......#####.#######.....#"
		"#..#.#..........#....#.........#"
		"#..#.#.............###.#.#.....#"
		"#..#.##########................#"
		"#..#..........#....#.#.#.#.....#"
		"#..#.####.###.#................#"
		"#..#.#......#.#................#"
		"#..#.#.####.#.#....###..###....#"
		"#..#.#......#.#....#......#....#"
		"#..#.########.#....#......#....#"
		"#..#..........#....#......#....#"
		"#..############....#......#....#"
		"#..................########....#"
		"#..............................#"
		"#..............................#"
		"################################";

    sf::Vector2i worldSize = {32, 32};
    bool followObject = false;
};

int main()
{
    CircleVsRect demo;
    if(demo.create()) demo.start();
    return 0;
}
