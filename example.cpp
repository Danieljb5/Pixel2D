#include "p2d.hpp"

class AnotherScript : public Script
{
public:
    AnotherScript() : Script() {}
    std::string msg = "Hello, World!\n";
};

class CustomScript : public Script
{
public:
    CustomScript() : Script() {}

    void Start() override
    {
        if(transform == nullptr)
        {
            std::cout << "No Transform Found\n";
            exit(1);
        }
        std::cout << transform->position << std::endl;
        transform->position.x += math->pi;
        std::cout << transform->position << std::endl;
        if(GetComponent<AnotherScript>() == nullptr)
        {
            std::cout << "No AnotherScript Found\n";
            exit(1);
        }
        std::cout << GetComponent<AnotherScript>()->msg;
    }
};

class CustomMovementScript : public Script
{
public:
    CustomMovementScript(AABB* simDist) : Script()
    {
        this->simDist = simDist;
    }

    void Start() override
    {
        if(transform == nullptr)
        {
            std::cout << "No Transform Found\n";
            exit(1);
        }
    }

    void Update() override
    {
        timer += dt;
        if(timer >= 0.5f)
        {
            if(simulated)
            {
                std::cout << "simulated! x: " << transform->position.x << " dt: " << dt << std::endl;
            }
            else
            {
                std::cout << "emulated! x: " << transform->position.x << " dt: " << dt << std::endl;
            }
            timer = 0;
        }
        if(b)
        {
            transform->position.x -= 50 * dt;
            if(transform->position.x < simDist->center.x - (simDist->halfDimension.x * 2))
            {
                b = false;
            }
        }
        if(!b)
        {
            transform->position.x += 50 * dt;
            if(transform->position.x > 0)
            {
                b = true;
            }
        }
    }

    AABB* simDist;
    bool b = false;
    float timer;
};

class ExampleApp : public Application
{
public:
    ExampleApp()
    {
        appName = "Example";
        spriteFilePath = "textures/exampleSprite.png";
    }

    GameObject* object;

    void OnCreate() override
    {
        // create object
        object = new GameObject();
        // add sprite renderer
        object->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        // custom scripts
        object->AddComponent(new CustomScript());
        object->AddComponent(new AnotherScript());
        // instantiate object
        AddObject(object);

        // uncomment the lines below to test the quadtree subdivision system

        GameObject* object2 = new GameObject();
        object2->transform->position.x += 315;
        object2->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        AddObject(object2);

        GameObject* object3 = new GameObject();
        object3->transform->position.x -= 315;
        object3->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        AddObject(object3);

        GameObject* object4 = new GameObject();
        object4->transform->position.x += 35;
        object4->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        AddObject(object4);

        GameObject* object5 = new GameObject();
        object5->transform->position.y += 35;
        object5->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        AddObject(object5);

        ////////////////////////////////////////////////////////////////////

        // uncomment the lines below to test the simulation/emulation system

        GameObject* object6 = new GameObject();
        object6->AddComponent(new CustomMovementScript(&simulationDistance));
        object6->AddComponent(new SpriteRenderer({0, 0, 218, 153}));
        AddObject(object6);

        ////////////////////////////////////////////////////////////////////
    }

    void OnUpdate() override
    {

    }
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