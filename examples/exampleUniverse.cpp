#include "p2d.hpp"
#include <random>

constexpr uint32_t starColours[8] = {
    p2d::Colour::White, p2d::Colour::MediumPurple, p2d::Colour::Purple, p2d::Colour::Cyan,
    p2d::Colour::LightBlue, p2d::Colour::OrangeRed, p2d::Colour::Orange, p2d::Colour::Red
};

constexpr uint32_t planetColours[8] = {
    p2d::Colour::Green, p2d::Colour::Grey, p2d::Colour::SkyBlue, p2d::Colour::Blue,
    p2d::Colour::BlueViolet, p2d::Colour::OrangeRed, p2d::Colour::PaleGreen, p2d::Colour::SandyBrown
};

struct Planet
{
    double distance = 0.0f;
    double diameter = 0.0f;
    double foliage = 0.0f;
    double minerals = 0.0f;
    double water = 0.0f;
    double gases = 0.0f;
    double temperature = 0.0f;
    double population = 0.0f;
    bool ring = false;
    std::vector<double> moons;
    int colour;
};

class StarSystem
{
public:
    StarSystem(uint32_t x, uint32_t y, p2d::Util &util, bool generateFullSystem = false)
    {
        seed = (x & 0xFFFF) << 16 | (y & 0xFFFF);

        util.randomSeed(seed);
        starExists = (util.randomInt(0, 20) == 1);
        if(!starExists) return;

        starDiameter = util.randomDouble(10.0, 40.0);
        starColour = starColours[util.randomInt(0, 8)];

        if(!generateFullSystem) return;

        double distanceFromStar = util.randomDouble(60.0, 200.0);
        int numPlanets = util.randomInt(0, 10);
        for(int i = 0; i < numPlanets; i++)
        {
            Planet p;
            p.distance = distanceFromStar;
            distanceFromStar += util.randomDouble(20.0, 200.0);
            p.diameter = util.randomDouble(4.0, 20.0);
            p.temperature = util.randomDouble(0.0, 1.0);
            p.foliage = util.randomDouble(0.0, 1.0);
            p.minerals = util.randomDouble(0.0, 1.0);
            p.gases = util.randomDouble(0.0, 1.0);
            p.water = util.randomDouble(0.0, 1.0);
            double sum = 1.0 / (p.foliage + p.minerals + p.gases + p.water);
            p.foliage *= sum;
            p.minerals *= sum;
            p.gases *= sum;
            p.water *= sum;
            p.population = std::max(util.randomInt(-5000000, 20000000), 0);
            p.ring = util.randomInt(0, 10) == 1;
            p.colour = planetColours[util.randomInt(0, 8)];
            int numMoons = std::max(util.randomInt(-5, 5), 0);
            for(int n = 0; n < numMoons; n++)
            {
                p.moons.push_back(util.randomDouble(1.0, 5.0));
            }
            planets.push_back(p);
        }
    }

    bool starExists = false;
    double starDiameter = 0.0f;
    int starColour = p2d::Colour::White;
    std::vector<Planet> planets;

private:
    uint32_t seed;
};

class Example : public p2d::Application
{
public:
    Example()
    {
        appName = "Example universe";
    }

    void onCreate() override
    {
        loadFont("fonts/Roboto-Medium.ttf");
    }

    void onUpdate() override
    {
        if(input.getKey(keyboard.W)) universeOffset.y -= 50.0f * time.dt;
        if(input.getKey(keyboard.S)) universeOffset.y += 50.0f * time.dt;
        if(input.getKey(keyboard.A)) universeOffset.x -= 50.0f * time.dt;
        if(input.getKey(keyboard.D)) universeOffset.x += 50.0f * time.dt;

        clear();
        
        int sectorsX = screenWidth() / 16;
        int sectorsY = screenHeight() / 16;

        sf::Vector2i mousePos = {(input.getMouseX() + 8) / 16, (input.getMouseY() + 8) / 16};
        sf::Vector2i universeMouse = mousePos + (sf::Vector2i)universeOffset;

        sf::Vector2i sector = {0, 0};

        for(sector.x = 0; sector.x < sectorsX; sector.x++)
        {
            for(sector.y = 0; sector.y < sectorsY; sector.y++)
            {
                StarSystem star(sector.x + (uint32_t)universeOffset.x, sector.y + (uint32_t)universeOffset.y, this->util);

                if(star.starExists)
                {
                    fillCircle(sector.x * 16, sector.y * 16, (int)star.starDiameter / 8, 1, star.starColour, star.starColour, center);

                    if(mousePos.x == sector.x && mousePos.y == sector.y)
                    {
                        drawCircle(sector.x * 16, sector.y * 16, 12, 2, colour.Yellow, center);
                    }
                }
            }
        }

        if(input.getButtonDown(mouse.Left))
        {
            StarSystem star(universeMouse.x, universeMouse.y, this->util);

            if(star.starExists)
            {
                isStarSelected = true;
                starSelected = universeMouse;
            }
            else
            {
                isStarSelected = false;
            }
        }

        if(isStarSelected)
        {
            StarSystem star(starSelected.x, starSelected.y, this->util, true);

            fillRect(8, 240, 496, 232, 1, colour.DarkBlue, colour.White);

            sf::Vector2i body = {14, 356};
            body.x += star.starDiameter * 1.375;
            fillCircle(body.x, body.y, (int)(star.starDiameter * 1.375), 1, star.starColour, star.starColour, center);
            body.x += (star.starDiameter * 1.375) + 8;

            for(auto& planet : star.planets)
            {
                if(body.x + planet.diameter >= 496) break;

                body.x += planet.diameter;
                fillCircle(body.x, body.y, (int)planet.diameter, 1, planet.colour, planet.colour, center);
                sf::Vector2i moon = body;
                moon.y += planet.diameter + 10;

                for(auto& pMoon : planet.moons)
                {
                    moon.y += pMoon;
                    fillCircle(moon.x, moon.y, (int)pMoon, 1, colour.Grey, colour.Grey, center);
                    moon.y += pMoon + 10;
                }

                body.x += planet.diameter + 8;
            }
            
        }

        drawString(2, 2, "X: " + std::to_string(universeOffset.x), 12);
        drawString(2, 16, "Y: " + std::to_string(universeOffset.y), 12);
    }

    sf::Vector2f universeOffset = {0, 0};
    bool isStarSelected = false;
    sf::Vector2i starSelected = {0, 0};
};

Example example;

int main()
{
    if(example.create(512, 480, 99999, SHOW_FPS, 1000, true))
    {
        example.start();
    }
    return 0;
}