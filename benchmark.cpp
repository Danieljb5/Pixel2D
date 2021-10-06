#include "p2d.hpp"

class ExampleApp : public Application
{
public:
    ExampleApp()
    {
        appName = "Example";
        spriteFilePath = "textures/exampleSprite.png";
    }

    void OnCreate() override
    {
        objs.push_back(new GameObject());
        objs.at(objs.size() - 1)->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        AddObject(objs.at(objs.size() - 1));
    }

    void OnUpdate() override
    {
        if(timer >= 1)
        {
            timer = 0;
            
            objs.push_back(new GameObject());
            objs.at(objs.size() - 1)->transform->position = {math.Random(-100, 100), math.Random(-100, 100)};
            objs.at(objs.size() - 1)->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
            AddObject(objs.at(objs.size() - 1));
            melons++;
            appName = (String)"melons: " + std::to_string(melons) + (String)" | " + std::to_string(time.frameRate * melons) + (String)" melons per second (m/s)";
            results.push_back(time.frameRate * melons);
            resultsFps.push_back(time.frameRate);
        }

        if(timer >= 0.5 && !b)
        {
            b = true;
            timer2 = 0;

            appName = (String)"melons: " + std::to_string(melons) + (String)" | " + std::to_string(time.frameRate * melons) + (String)" melons per second (m/s)";
            results.push_back(time.frameRate * melons);
            resultsFps.push_back(time.frameRate);
        }

        timer += time.deltaTime;
        timer2 += time.deltaTime;
    }

    void OnDestroy() override
    {
        float maxResult = 0;
        int maxIT = 0;
        for(int i = 0; i < results.size(); i++)
        {
            if(results.at(i) > maxResult)
            {
                maxResult = results.at(i);
                maxIT = i;
            }
        }

        std::cout << "best score: " << maxResult << " melons per second (m/s) - " << (int)(maxResult / resultsFps.at(maxIT)) << " melons\n";
        std::cout << " - " << resultsFps.at(maxIT) << " FPS\n";

        File file;
        file << results;
        file.Save("Results.txt");
        File file2;
        file2 << resultsFps;
        file2.Save("Results2.txt");
    }

    std::vector<GameObject*> objs;
    std::vector<float> results;
    std::vector<float> resultsFps;
    float timer = 0, timer2 = 0;
    bool b = false;
    int melons = 1;
};

int main()
{
    ExampleApp app;
    app.frameRateLimit = 99999;
    app.vsync = false;
    app.showFps = true;
    app.Create();
    return 0;
}