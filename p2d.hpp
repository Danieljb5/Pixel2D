
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                                            //
  //  Pixel 2D - P2D                                                                                            //
  //                                                                                                            //
  //  A small graphics and input built in a single header file, using the SFML library                          //
  //                                                                                                            //
  //  Examples code can be found in the examples folder and built using the provided script                     //
  //  many of the example programs are ports from the OLC Pixel Game Engine, such as the circleVsRect example   //
  //                                                                                                            //
  //                                                                                                            //
  //    examples/example.cpp  -  creates a blank window with the default parameters                             //
  //    ----------------------------------------------------------------------------------------------------    //
  //1   #include "p2d.hpp"                                                                                      //
  //2                                                                                                           //
  //3   class Example : public p2d::Application                                                                 //
  //4   {                                                                                                       //
  //5   public:                                                                                                 //
  //6       Example()                                                                                           //
  //7       {                                                                                                   //
  //8           appName = "Example";                                                                            //
  //9       }                                                                                                   //
  //10                                                                                                          //
  //11      void onCreate() override                                                                            //
  //12      {                                                                                                   //
  //13          //init                                                                                          //
  //14      }                                                                                                   //
  //15                                                                                                          //
  //16      void onUpdate() override                                                                            //
  //17      {                                                                                                   //
  //18          //game logic                                                                                    //
  //19      }                                                                                                   //
  //20  };                                                                                                      //
  //21                                                                                                          //
  //22  int main()                                                                                              //
  //23  {                                                                                                       //
  //24      Example app;                                                                                        //
  //25      if(app.create()) // leaving create() empty sets up a window with the default parameters             //
  //26      {                                                                                                   //
  //27          app.start();                                                                                    //
  //28      }                                                                                                   //
  //29      return 0;                                                                                           //
  //30  }                                                                                                       //
  //    ----------------------------------------------------------------------------------------------------    //
  //                                                                                                            //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef P2D_HPP
#define P2D_HPP

#include <cstring>
#include <memory>
#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional>
#include <condition_variable>
#include <chrono>
#include <iterator>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <istream>
#include <ostream>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
// #define ASIO_STANDALONE
// #include <asio.hpp>
// #include <asio/ts/buffer.hpp>
// #include <asio/ts/internet.hpp>

namespace p2d
{
    // \brief Window flag - enables VSync
    #define VSYNC 1
    // \brief Window flag - shows the framerate in the window title
    #define SHOW_FPS 2
    // \brief Window flag - makes the window fullscreen and changes the resolution to fit the screen
    #define FULLSCREEN 4
    // \brief Window flag - default window flags
    #define DEFAULT_FLAGS 3

    #define DEFAULT_WINDOW_WIDTH 640
    #define DEFAULT_WINDOW_HEIGHT 480
    
    //Utility functions
    class Util
    {
    public:
        Util() {}

        // \brief Fast random number generator using the Lehmer generator
        // \return randomly generated number
        uint32_t random()
        {
            lehmer += 0xe120fc15;
            uint64_t tmp;
            tmp = (uint64_t)lehmer * 0x4a39b70d;
            uint32_t m1 = (tmp >> 32) ^ tmp;
            tmp = (uint64_t)m1 * 0x12fad5c9;
            uint32_t m2 = (tmp >> 32) ^ tmp;
            return m2;
        }

        // \brief Generates a random boolean using random() and a provided bias
        // \param bias          the bias, ranging from 0 to 1. 0.5 gives an equal chance of true and false.
        // 0 gives a 100% chance of false and 1 gives a 100% chance of true
        // \return true or false
        bool randomBool(float bias = 0.5f)
        {
            uint32_t num = random();
            return num % 256 < bias * 256;
        }

        // \brief Generates a random integer within the min and max parameters using random()
        int randomInt(int min, int max)
        {
            return (random() % (max - min)) + min;
        }

        // \brief Generates a random float within the min and max parameters using random()
        float randomFloat(float min, float max)
        {
            return (((float)random() / (float)(0x7FFFFFFF)) * (max - min)) + min;
        }
        
        // \brief Generates a random double within the min and max parameters using random()
        double randomDouble(double min, double max)
        {
            return (((double)random() / (double)(0x7FFFFFFF)) * (max - min)) + min;
        }

        // \brief Sets the seed for random()
        void randomSeed(uint32_t seed)
        {
            lehmer = seed;
        }

    private:
        uint32_t lehmer = 0;
    };

    //Input class
    class Input
    {
    public:
        Input() {}

        // \brief Clears the input buffer - internal function
        void clear()
        {
            keysDown.clear();
            keysUp.clear();
            buttonsDown.clear();
            buttonsUp.clear();
            deltaScroll = 0;
        }

        // \brief Polls input - internal function
        void poll(sf::Event e, sf::RenderWindow &window)
        {
            deltaScroll = 0;
            if(e.type == sf::Event::KeyPressed)
            {
                sf::Keyboard::Key k = e.key.code;
                keysDown.insert({k, true});
                if(keys.count(k) == 0) keys.insert({k, true});
                else keys[k] = true;
            }
            if(e.type == sf::Event::KeyReleased)
            {
                sf::Keyboard::Key k = e.key.code;
                keysUp.insert({k, true});
                if(keys.count(k) > 0) keys[k] = false;
            }
            if(e.type == sf::Event::MouseButtonPressed)
            {
                sf::Mouse::Button b = e.mouseButton.button;
                buttonsDown.insert({b, true});
                if(buttons.count(b) == 0) buttons.insert({b, true});
                else buttons[b] = true;
            }
            if(e.type == sf::Event::MouseButtonReleased)
            {
                sf::Mouse::Button b = e.mouseButton.button;
                buttonsUp.insert({b, true});
                if(buttons.count(b) > 0) buttons[b] = false;
            }
            if(e.type == sf::Event::MouseMoved)
            {
                mouseX = sf::Mouse::getPosition(window).x;
                mouseY = sf::Mouse::getPosition(window).y;
            }
            if(e.type == sf::Event::MouseWheelScrolled)
            {
                float scrollX = e.mouseWheel.x;
                scrollX /= abs(scrollX);
                scroll += scrollX;
                deltaScroll = scrollX;
            }
        }

        bool getKey(int key)
        {
            if(keys.count(key) > 0) return keys[key];
            return false;
        }

        bool getKeyDown(int key)
        {
            if(keysDown.count(key) > 0) return keysDown[key];
            return false;
        }

        bool getKeyUp(int key)
        {
            if(keysUp.count(key) > 0) return keysUp[key];
            return false;
        }

        bool getButton(int button)
        {
            if(buttons.count(button) > 0) return buttons[button];
            return false;
        }

        bool getButtonDown(int button)
        {
            if(buttonsDown.count(button) > 0) return buttonsDown[button];
            return false;
        }

        bool getButtonUp(int button)
        {
            if(buttonsUp.count(button) > 0) return buttonsUp[button];
            return false;
        }

        int getMouseX()
        {
            return mouseX;
        }

        int getMouseY()
        {
            return mouseY;
        }

        int getCenteredMouseX(int width)
        {
            return mouseX - width / 2;
        }

        int getCenteredMouseY(int height)
        {
            return mouseY - height / 2;
        }

        float getScroll()
        {
            return scroll;
        }

        float getDeltaScroll()
        {
            return deltaScroll;
        }

    private:
        std::map<int, bool> keys;
        std::map<int, bool> keysDown;
        std::map<int, bool> keysUp;
        std::map<int, bool> buttons;
        std::map<int, bool> buttonsDown;
        std::map<int, bool> buttonsUp;
        int mouseX, mouseY;
        float scroll, deltaScroll;
    };

    //Colour information class
    class Colour
    {
    public:
        Colour() {}

        enum BuiltInColours : uint32_t {
            Black = 0x000000FF, DarkSlateGrey = 0x2F4F4FFF, DimGrey = 0x696969FF, SlateGrey = 0x708090FF, DarkGrey = 0x808080FF, LightSlateGrey = 0x778899FF, Grey = 0xA9A9A9FF, Silver = 0xC0C0C0FF, LightGrey = 0xD3D3D3FF, VeryLightGrey = 0xDCDCDCFF, White = 0xFFFFFFFF, Ivory = 0xFFFFF0FF, Snow = 0xFFFAFAFF, MintCream = 0xF4FFFAFF, Azure = 0xF0FFFFFF, FloralWhite = 0xFFFAF0FF, HoneyDew = 0xF0FFF0FF, GhostWhite = 0xF8F8FFFF, Seashell = 0xFFF5EEFF, AliceBlue = 0xF0F8FFFF, OldLace = 0xFDF5E6FF, LavenderBlush = 0xFFF0F5FF, WhiteSmoke = 0xF5F5F5FF, Beige = 0xF5F5DCFF, Linen = 0xFAF0E6FF, AntiqueWhite = 0xFAEBD7FF, MistyRose = 0xFFE4E1FF, Lavender = 0xE6E6FAFF, Thistle = 0xD8BFD8FF, Plum = 0xDDA0DDFF, Violet = 0xEE82EEFF, Orchid = 0xDA70D6FF, MediumPurple = 0x9370DBFF, MediumOrchid = 0xBA55D3FF, MediumSlateBlue = 0x7B68EEFF, SlateBlue = 0x6A5ACDFF, Magenta = 0xFF00FFFF, Fuchsia = 0xFF00FFFF, DarkOrchid = 0x9932CCFF, BlueViolet = 0x8A2BE2FF, DarkSlateBlue = 0x483D8BFF, DarkViolet = 0x9400D3FF, DarkMagenta = 0x8B008BFF, Purple = 0x800080FF, Indigo = 0x4B0082FF, Cornsilk = 0xFFF8DCFF, BlanchedAlmond = 0xFFEBCDFF, Bisque = 0xFFE4C4FF, NavajoWhite = 0xFFDEADFF, Wheat = 0xF5DEB3FF, Burlywood = 0xDEB887FF, Tan = 0xD2B48CFF, SandyBrown = 0xF4A460FF, Goldenrod = 0xDAA520FF, RosyBrown = 0xBC8F8FFF, Peru = 0xCD853FFF, DarkGoldenRod = 0xB8860BFF, Chocolate = 0xD2691EFF, Sienna = 0xA0522DFF, SaddleBrown = 0x8B4513FF, Brown = 0xA52A2AFF, Maroon = 0x800000FF, LightYellow = 0xFFFFE0FF, LemonChiffon = 0xFFFACDFF, LightGoldenrodYellow = 0xFAFAD2FF, PapayaWhip = 0xFFEFD5FF, Moccasin = 0xFFE4B5FF, PaleGoldenrod = 0xEEE8AAFF, Yellow = 0xFFFF00FF, PeachPuff = 0xFFDAB9FF, Khaki = 0xF0E68CFF, Gold = 0xFFD700FF, DarkKhaki = 0xBDB76BFF, Orange = 0xFFA500FF, Coral = 0xFF7F50FF, DarkOrange = 0xFF8C00FF, Tomato = 0xFF6347FF, OrangeRed = 0xFF4500FF, LightSalmon = 0xFFA07AFF, DarkSalmon = 0xE9967AFF, Salmon = 0xFA8072FF, LightCoral = 0xF08080FF, IndianRed = 0xCD5C5CFF, Crimson = 0xDC143CFF, Firebrick = 0xB22222FF, Red = 0xFF0000FF, DarkRed = 0x8B0000FF, Pink = 0xFFC0CBFF, LightPink = 0xFFB6C1FF, HotPink = 0xFF69B4, PaleVioletRed = 0xDB7093FF, DeepPink = 0xFF1493FF, MediumVioletRed = 0xC71585FF, PowderBlue = 0xB0E0E6FF, LightBlue = 0xADD8E6FF, LightSteelBlue = 0xB0C4DEFF, LightSkyBlue = 0x87CEFAFF, SkyBlue = 0x87CEEBFF, CornflowerBlue = 0x6495EDFF, DeepSkyBlue = 0x00BFFFFF, DodgerBlue = 0x1E90FFFF, SteelBlue = 0x4682B4FF, RoyalBlue = 0x4169E1FF, MidnightBlue = 0x191970FF, Blue = 0x0000FFFF, MediumBlue = 0x0000CDFF, DarkBlue = 0x00008BFF, Navy = 0x000080FF, LightCyan = 0xE0FFFFFF, PaleTurquoise = 0xAFEEEEFF, Aquamarine = 0x7FFFD4FF, Cyan = 0x00FFFFFF, Aqua = 0x00FFFFFF, Turquoise = 0x40E0D0FF, MediumTurquoise = 0x48D1CCFF, DarkTurquoise = 0x00CED1FF, CadetBlue = 0x5F9EA0FF, LightSeaGreen = 0x20B2AAFF, DarkCyan = 0x008B8BFF, Teal = 0x008080FF, PaleGreen = 0x98FB98FF, GreenYellow = 0xADFF2FFF, LightGreen = 0x90EE90FF, Chartreuse = 0x7FFF00FF, LawnGreen = 0x7CFC00FF, YellowGreen = 0x9ACD32FF, MediumAquamarine = 0x66CDAAFF, DarkSeaGreen = 0x8FBC8FFF, MediumSpringGreen = 0x00FA9AFF, SpringGreen = 0x00FF7FFF, Lime = 0x00FF00FF, LimeGreen = 0x32CD32FF, MediumSeaGreen = 0x3CB371FF, OliveDrab = 0x6B8E23FF, Olive = 0x808000FF, SeaGreen = 0x2E8B57FF, ForestGreen = 0x228B22FF, DarkOliveGreen = 0x556B2FFF, Green = 0x008000FF, DarkGreen = 0x006400FF
        };

        int fromRGBA(int r, int g, int b, int a)
        {
            int colour;
            colour += r << 24;
            colour += g << 16;
            colour += b << 8;
            return colour + a;
        }
    };

    //Keyboard mapping class
    class Keyboard
    {
    public:
        Keyboard() {}

        enum Keys {
            Unknown = -1, A = 0, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Escape, LControl, LShift, LAlt, LSystem, RControl, RShift, RAlt, RSystem, Menu, LBracket, RBracket, Semicolon, Comma, Period, Quote, Slash, Backslash, Tilde, Equal, Hyphen, Space, Enter, Backspace, Tab, PageUp, PageDown, End, Home, Insert, Delete, Add, Subtract, Multiply, Divide, Left, Right, Up, Down, Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, Pause, KeyCount, Dash = Hyphen, BackSpace = Backspace, BackSlash = Backslash, SemiColon = Semicolon, Return = Enter
        };
    };

    //Mouse button mapping class
    class Mouse
    {
    public:
        Mouse() {}

        enum Buttons {
            Left, Right, Middle, XButton1, XButton2, ButtonCount
        };
    };

    class Time
    {
    public:
        Time()
        {
            clock.restart();
        }

        struct Date
        {
            uint year = 0;
            uint month = 0;
            uint day = 0;
            uint hour = 0;
            uint minute = 0;
            uint second = 0;

            friend std::ostream& operator<<(std::ostream& os, Date d)
            {
                os << d.day << "/" << d.month << "/" << d.year << " " << d.hour << ":" << d.minute << ":" << d.second;
                return os;
            }
        };

        float dt;

        // \brief Returns the time sincre launching the application in seconds
        float time()
        {
            return clock.getElapsedTime().asSeconds();
        }

        // \brief Returns the current date and time
        Date date()
        {
            time_t now = std::time(0);
            tm* ltm = localtime(&now);
            Date d;
            d.year = ltm->tm_year + 1900;
            d.month = ltm->tm_mon;
            d.day = ltm->tm_mday;
            d.hour = ltm->tm_hour;
            d.minute = ltm->tm_min;
            d.second = ltm->tm_sec;
            return d;
        }

    private:
        sf::Clock clock;
    };

    class Math
    {
    public:
        Math()
        {

        }

        // \brief Returns the smaller value of a and b
        float min(float a, float b)
        {
            if(a < b) return a;
            return b;
        }

        // \brief Returns the larger value of a and b
        float max(float a, float b)
        {
            if(a > b) return a;
            return b;
        }

        // \brief Returns the smaller value of a and b
        int min(int a, int b)
        {
            if(a < b) return a;
            return b;
        }

        // \brief Returns the larger value of a and b
        int max(int a, int b)
        {
            if(a > b) return a;
            return b;
        }

        // \brief Returns the magnitude of a vector
        float mag(sf::Vector2f vec)
        {
            return sqrt(pow(vec.x, 2) + pow(vec.y, 2));
        }

        // \brief Returns the smaller values of a and b
        sf::Vector2f min(sf::Vector2f a, sf::Vector2f b)
        {
            sf::Vector2f vec;
            vec.x = this->min(a.x, b.x);
            vec.y = this->min(a.y, b.y);
            return vec;
        }

        // \brief Returns the larger values of a and b
        sf::Vector2f max(sf::Vector2f a, sf::Vector2f b)
        {
            sf::Vector2f vec;
            vec.x = this->max(a.x, b.x);
            vec.y = this->max(a.y, b.y);
            return vec;
        }

        // \brief Clamps a float to a given range
        float clamp(float min, float max, float n)
        {
            n = this->max(min, this->min(max, n));
            return n;
        }

        // \brief Clamps an integer to a given range
        int clamp(int min, int max, int n)
        {
            n = this->max(min, this->min(max, n));
            return n;
        }

        // \brief Clamps a vector to a given range
        sf::Vector2f clamp(float min, float max, sf::Vector2f vec)
        {
            vec.x = this->clamp(min, max, vec.x);
            vec.y = this->clamp(min, max, vec.y);
            return vec;
        }

        // \brief Returns a normalised vector
        sf::Vector2f norm(float x, float y)
        {
            float den = sqrt(pow(x, 2) + pow(y, 2));
            if(den == 0) return {0, 0};
            x /= den;
            y /= den;
            return {x, y};
        }

        // \brief Returns a normalised vector
        sf::Vector2f norm(sf::Vector2f vec)
        {
            float den = sqrt(pow(vec.x, 2) + pow(vec.y, 2));
            if(den == 0) return {0, 0};
            vec.x /= den;
            vec.y /= den;
            return vec;
        }

        // \brief Returns 1 if n is positive, and -1 if n is negative
        // \param n         the number to check
        int getNegative(float n)
        {
            return abs(n) / n;
        }
    };

    class Console
    {
    public:
        Console()
        {
            initTime = false;
        }

        Console(p2d::Time &time)
        {
            this->time = time;
            initTime = true;
        }

        void log(const char* str, bool logTime = true)
        {
            if(logTime && initTime) std::cout << time.date() << " | LOG: " << str << std::endl;
            else std::cout << "LOG: " << str << std::endl;
        }

        void log(const char str, bool logTime = true)
        {
            if(logTime && initTime) std::cout << time.date() << " | LOG: " << str << std::endl;
            else std::cout << "LOG: " << str << std::endl;
        }

        // \brief Red error colour is UNIX specific
        void logError(const char* str, bool logTime = true)
        {
            if(logTime && initTime) std::cerr << "\033[31m" << time.date() << " | ERROR: " << str << std::endl << "\033[37m";
            else std::cerr << "\033[31mERROR: " << str << std::endl << "\033[37m";
        }

        // \brief Red error colour is UNIX specific
        void logError(const char str, bool logTime = true)
        {
            if(logTime && initTime) std::cerr << "\033[31m" << time.date() << " | ERROR: " << str << std::endl << "\033[37m";
            else std::cerr << "\033[31mERROR: " << str << std::endl << "\033[37m";
        }

        // \brief Yellow warning colour is UNIX specific
        void logWarning(const char* str, bool logTime = true)
        {
            if(logTime && initTime) std::cout << "\033[33m" << time.date() << " | WARNING: " << str << std::endl << "\033[37m";
            else std::cout << "\033[33mWARNING: " << str << std::endl << "\033[37m";
        }

        // \brief Yellow warning colour is UNIX specific
        void logWarning(const char str, bool logTime = true)
        {
            if(logTime && initTime) std::cout << "\033[33m" << time.date() << " | WARNING: " << str << std::endl << "\033[37m";
            else std::cout << "\033[33mWARNING: " << str << std::endl << "\033[37m";
        }

    private:
        p2d::Time time;
        bool initTime = false;
    };

    class sprite
    {
    public:
        sprite()
        {
            valid = false;
        }

        sprite(sf::Texture tex)
        {
            this->tex = tex;
            valid = true;
        }

        void setX(float newX)
        {
            x = newX;
            isModified = true;
        }

        void setY(float newY)
        {
            y = newY;
            isModified = true;
        }

        void setPos(float newX, float newY)
        {
            x = newX;
            y = newY;
            isModified = true;
        }

        void move(float offX, float offY)
        {
            x += offX;
            y += offY;
            isModified = true;
        }

        float getX()
        {
            return x;
        }

        float getY()
        {
            return y;
        }

        void setWidth(int newW)
        {
            width = newW;
            isModified = true;
        }

        void setHeight(int newH)
        {
            height = newH;
            isModified = true;
        }

        void setSize(int newW, int newH)
        {
            width = newW;
            height = newH;
            isModified = true;
        }

        float getWidth()
        {
            return width;
        }

        float getHeight()
        {
            return height;
        }

        void setRotation(float newR)
        {
            rotation = newR;
            isModified = true;
        }

        void rotate(float r)
        {
            rotation += r;
            isModified = true;
        }

        float getRotation()
        {
            return rotation;
        }

        void setDrawMode(int drawMode)
        {
            this->drawMode = drawMode;
            isModified = true;
        }

        int getDrawMode()
        {
            return drawMode;
        }

        void setTexture(sf::Texture newTex)
        {
            this->tex = newTex;
            this->valid = true;
            isModified = true;
        }

        sf::Texture getTexture()
        {
            if(valid)
            return tex;
            return sf::Texture();
        }

        bool isValid()
        {
            return valid;
        }

        // internal call used for sprite batching
        bool getModified()
        {
            return isModified;
        }

        // internal call used for sprite batching
        void setModified(bool v = false)
        {
            isModified = v;
        }

    private:
        sf::Texture tex;
        int drawMode = 0;
        float rotation = 0;
        int width = 0, height = 0;
        float x = 0, y = 0;
        bool valid = false;
        bool isModified = true;
    };

    class atlasSprite : public sprite
    {
    public:
        atlasSprite()
            : sprite()
        {
            atlasX = 0; atlasY = 0;
        }

        atlasSprite(int atlasX, int atlasY)
            : sprite()
        {
            this->atlasX = atlasX; this->atlasY = atlasY;
        }

        atlasSprite(int atlasX, int atlasY, int atlasW, int atlasH)
            : sprite()
        {
            this->atlasX = atlasX; this->atlasY = atlasY;
            this->atlasW = atlasW; this->atlasH = atlasH;
        }

        void setAtlasPos(int atlasX, int atlasY)
        {
            this->atlasX = atlasX;
            this->atlasY = atlasY;
            setModified(true);
        }

        void setAtlasSize(int atlasW, int atlasH)
        {
            this->atlasW = atlasW;
            this->atlasH = atlasH;
            setModified(true);
        }
        
        int getAtlasPosX()
        {
            return atlasX;
        }

        int getAtlasPosY()
        {
            return atlasY;
        }

        int getAtlasPosW()
        {
            return atlasW;
        }

        int getAtlasPosH()
        {
            return atlasH;
        }

        void setID(int newID)
        {
            id = newID;
            setModified(true);
        }

        int getID()
        {
            return id;
        }

    private:
        int atlasX, atlasY;
        int atlasW, atlasH;
        int id;
    };

    struct save
    {
        save()
        {

        }

        save(std::vector<uint8_t> fileData)
        {
            this->data = fileData;
        }

        std::vector<uint8_t> data;

        size_t size() const
        {
            return data.size();
        }

        template <typename DataType>
        friend save &operator << (save &s, const DataType &d)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be written");
            size_t i = s.data.size();
            s.data.resize(s.data.size() + sizeof(DataType));
            std::memcpy(s.data.data() + i, &d, sizeof(DataType));
            return s;
        }

        template <typename DataType>
        friend save &operator >> (save &s, DataType &d)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be read");
            size_t i = s.data.size() - sizeof(DataType);
            std::memcpy(&d, s.data.data() + i, sizeof(DataType));
            s.data.resize(i);
            return s;
        }

        // \brief Use this instead of << when inserting a string
        void putString(std::string str)
        {
            int sLen = str.length();
            int size = this->data.size();
            this->data.resize(size + sLen + sizeof(int));
            for(int i = 0; i < sLen; i++)
            {
                char c = str.data()[i];
                std::memcpy(this->data.data() + size + i, &c, sizeof(c));
            }
            std::memcpy(this->data.data() + size + sLen, &sLen, sizeof(sLen));
        }

        std::string getString()
        {
            int sLen;
            size_t size = this->data.size() - sizeof(int);
            std::memcpy(&sLen, this->data.data() + size, sizeof(int));
            this->data.resize(size);
            std::vector<char> message;
            for(int i = 0; i < sLen; i++)
            {
                char c;
                size_t s = this->data.size() - sizeof(char);
                std::memcpy(&c, this->data.data() + s, sizeof(char));
                this->data.resize(s);
                message.push_back(c);
            }
            std::reverse(message.begin(), message.end());
            std::string str(message.begin(), message.end());
            return str;
        }
    };

    class Assets
    {
    public:
        Assets()
        {
            init = false;
        }

        Assets(Console &console, Util &util)
        {
            this->console = console;
            this->util = util;
            init = true;
        }

        enum assetType {
            texture
        };

        int load(std::string filepath, int assetType = texture)
        {
            if(!init)
            {
                std::cout << "\033[33mWARNING: tried to load image before initialisation\033[37m" << std::endl;
                return -1;
            }
            if(assetType == texture)
            {
                sf::Texture tex;
                if(!tex.loadFromFile((char*)filepath.c_str()))
                {
                    console.logError((char*)((std::string)"Failed to load image: " + filepath).c_str());
                    return -1;
                }
                int sprIndex = unique<sprite>(sprites);
                sprites[sprIndex] = sprite(tex);
                return sprIndex;
            }
            return -2;
        }

        save loadSave(char* filepath)
        {
            if(!init)
            {
                std::cout << "\033[33mWARNING: tried to load data before initialisation\033[37m" << std::endl;
            }
            std::vector<uint8_t> fileData;
            std::cout << "opening file\n";
            std::ifstream in(filepath, std::ios::in | std::ifstream::binary);
            std::cout << "creating start iterator\n";
            std::istreambuf_iterator<char> iter{in};
            std::cout << "creating end iterator\n";
            std::istreambuf_iterator<char> end{};
            std::cout << "reading file\n";
            std::copy(iter, end, std::back_inserter(fileData));
            std::cout << "closing file\n";
            in.close();
            std::cout << "done\n";
            return {fileData};
        }

        void saveData(const char* filepath, save data)
        {
            std::ofstream out(filepath, std::ios::out | std::ofstream::binary);
            std::copy(data.data.begin(), data.data.end(), std::ostreambuf_iterator<char>(out));
            out.close();
        }

        sprite getSprite(int index)
        {
            if(sprites.count(index) > 0)
            {
                return sprites[index];
            }
            return sprite();
        }

        void clear()
        {
            sprites.clear();
        }
        
    private:
        Console console;
        Util util;
        bool init = false;
        std::unordered_map<int, sprite> sprites;

        template <typename T>
        int unique(std::unordered_map<int, T> map)
        {
            int i = util.random();
            while(map.count(i) > 0)
            {
                i = util.random();
            }
            return i;
        }
    };

    class TileMap : public sf::Drawable, public sf::Transformable
    {
    public:
        bool load(std::string filepath, sf::Vector2u textureSize, const int* tiles, unsigned int width, unsigned int height)
        {
            this->width = width;
            if(!m_texture.loadFromFile(filepath)) return false;

            m_vertices.setPrimitiveType(sf::Quads);
            m_vertices.resize(width * height * 4);

            for(int x = 0; x < (int)width; x++)
            {
                for(int y = 0; y < (int)height; y++)
                {
                    int tileNumber = tiles[x + y * width];
                    int tu = tileNumber % (m_texture.getSize().x / textureSize.x);
                    int tv = tileNumber / (m_texture.getSize().x / textureSize.x);
                    sf::Vertex* quad = &m_vertices[(x + y * width) * 4];
                    quad[0].position = sf::Vector2f(x * textureSize.x, y * textureSize.y);
                    quad[1].position = sf::Vector2f((x + 1) * textureSize.x, y * textureSize.y);
                    quad[2].position = sf::Vector2f((x + 1) * textureSize.x, (y + 1) * textureSize.y);
                    quad[3].position = sf::Vector2f(x * textureSize.x, (y + 1) * textureSize.y);
                    quad[0].texCoords = sf::Vector2f(tu * textureSize.x, tv * textureSize.y);
                    quad[1].texCoords = sf::Vector2f((tu + 1) * textureSize.x, tv * textureSize.y);
                    quad[2].texCoords = sf::Vector2f((tu + 1) * textureSize.x, (tv + 1) * textureSize.y);
                    quad[3].texCoords = sf::Vector2f(tu * textureSize.x, (tv + 1) * textureSize.y);
                }
            }
            return true;
        }

        void updateBounds(int x, int y, int w, int h)
        {
            m_tiles.clear();
            m_tiles.setPrimitiveType(sf::Quads);
            for(int xpos = x; xpos < x + w; xpos++)
            {
                for(int ypos = y; ypos < y + h; ypos++)
                {
                    sf::Vertex* quad = &m_vertices[(xpos + ypos * width) * 4];
                    m_tiles.append(quad[0]);
                    m_tiles.append(quad[1]);
                    m_tiles.append(quad[2]);
                    m_tiles.append(quad[3]);
                }
            }
        }

    private:
        virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
        {
            states.transform *= getTransform();
            states.texture = &m_texture;
            target.draw(m_tiles, states);
        }

        sf::VertexArray m_vertices;
        sf::VertexArray m_tiles;
        sf::Texture m_texture;
        unsigned int width;
    };

    class SpriteBuffer : public sf::Drawable, public sf::Transformable
    {
    public:
        SpriteBuffer() {}

        SpriteBuffer(sf::View &camera)
        {
            this->camera = camera;
            for(int i = 0; i < 32; i++)
            {
                vertices[i].setPrimitiveType(sf::Quads);
            }
        }

        ~SpriteBuffer() {}

        bool loadAtlas(std::string filepath)
        {
            return textureAtlas.loadFromFile(filepath);
        }

        bool insert(atlasSprite* sprite, int layer = 0)
        {
            if(layer < 0) layer = 0;
            if(layer >= 32) layer = 31;
            sprite->setID(generateID(layer));
            sprites[layer].insert({sprite->getID(), sprite});
            return true;
        }

        // internal call for improving draw efficiency
        int update(int drawCalls, int drawCallLimit)
        {
            int deltaDrawCalls = 0;
            for(int i = 0; i < 32; i++)
            {                
                if(sprites[i].size() == 0) continue;                
                int countChanged = 0;                
                auto it = sprites[i].begin();
                deltaDrawCalls += renderedSprites[i].size();
                for(int j = 0; j < sprites[i].size(); j++)
                {                    
                    if(it == sprites[i].end()) break;
                    atlasSprite* spr = it->second;                        

                    if(spr->getModified())
                    {
                        if(isOnScreen(spr) && deltaDrawCalls + drawCalls < drawCallLimit)
                        {
                            if(renderedSprites[i].count(spr->getID()) == 0)
                            {
                                renderedSprites[i].insert({spr->getID(), spr});
                                deltaDrawCalls++;
                            }
                        }  
                        else if(renderedSprites[i].count(spr->getID()) != 0)
                        {
                            renderedSprites[i].erase(spr->getID());
                        }
                        spr->setModified();
                    }
                    it++;
                }
            
                if(renderedSprites[i].size() == 0) continue;
                vertices[i].resize(renderedSprites[i].size() * 4);
                
                it = renderedSprites[i].begin();
                for(int j = 0; j < renderedSprites[i].size(); j++)
                {   
                    if(it == sprites[i].end()) break;

                    atlasSprite* spr = it->second;

                    sf::Vertex* quad = &vertices[i][j * 4];

                    quad[0].position = {spr->getX(), spr->getY()};
                    quad[1].position = {spr->getX() + spr->getWidth(), spr->getY()};
                    quad[2].position = {spr->getX() + spr->getWidth(), spr->getY() + spr->getHeight()};
                    quad[3].position = {spr->getX(), spr->getY() + spr->getHeight()};

                    quad[0].texCoords = {spr->getAtlasPosX(), spr->getAtlasPosY()};
                    quad[1].texCoords = {spr->getAtlasPosX() + spr->getAtlasPosW(), spr->getAtlasPosY()};
                    quad[2].texCoords = {spr->getAtlasPosX() + spr->getAtlasPosW(), spr->getAtlasPosY() + spr->getAtlasPosH()};
                    quad[3].texCoords = {spr->getAtlasPosX(), spr->getAtlasPosY() + spr->getAtlasPosH()};

                    it++;
                }
            }
            return deltaDrawCalls;
        }

    private:
        std::map<int, atlasSprite*> sprites[32];
        std::map<int, atlasSprite*> renderedSprites[32];
        sf::VertexArray vertices[32];
        sf::Texture textureAtlas;
        Util util;
        sf::View camera;

        virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
        {
            states.transform *= getTransform();
            states.texture = &textureAtlas;
            for(int i = 0; i < 32; i++)
            {
                target.draw(vertices[i], states);
            }
        }

        int generateID(int layer)
        {
            int id = util.random();
            while(sprites[layer].count(id) > 0)
            {
                id = util.random();
            }
            return id;
        }

        bool isOnScreen(atlasSprite* spr)
        {
            return !(spr->getX() >= camera.getCenter().x + (camera.getSize().x / 2.f) || spr->getX() + spr->getWidth() < camera.getCenter().x - (camera.getSize().x / 2.f) || spr->getY() >= camera.getCenter().y + (camera.getSize().y / 2.f) || spr->getY() + spr->getHeight() < camera.getCenter().y - (camera.getSize().y / 2.f));
        }
    };

    //Main window management class
    class Application
    {
    public:
        enum DrawMode {
            topLeft, center
        };

        Application() {}

        // \brief Called after the application is started and before the first update
        virtual void onCreate() {}
        // \brief Called every frame
        virtual void onUpdate() {}
        // \brief Called before the application exits
        virtual void onDestroy() {}

        // \brief Create the window and set flags
        // \param windowWidth                   defines the width of the window
        // \param windowHeight                  defines the height of the window
        // \param frameRate                     sets the framerate limit
        // \return true if window creation is successful
        bool create(unsigned int windowWidth = DEFAULT_WINDOW_WIDTH, unsigned int windowHeight = DEFAULT_WINDOW_HEIGHT,  int frameRate = 120, int windowFlags = DEFAULT_FLAGS, int drawCallLimit = 1000, bool cullingEnabled = true)
        {
            if((windowFlags & FULLSCREEN) == FULLSCREEN) fullscreen = true;
            int style = sf::Style::Default;
            if(fullscreen)
            {
                style = sf::Style::Fullscreen;
                windowWidth = sf::VideoMode::getDesktopMode().width;
                windowHeight = sf::VideoMode::getDesktopMode().height;
            }
            if(window.isOpen()) window.close();
            window.create(sf::VideoMode(windowWidth, windowHeight), appName, style);
            if((windowFlags & VSYNC) == VSYNC) window.setVerticalSyncEnabled(true);
            else window.setVerticalSyncEnabled(false);
            if((windowFlags & SHOW_FPS) == SHOW_FPS) showFps = true;
            window.setFramerateLimit(frameRate);
            sWidth = windowWidth;
            sHeight = windowHeight;
            camera.setCenter(windowWidth / 2.f, windowHeight / 2.f);
            camera.setSize(windowWidth, windowHeight);
            camera.zoom(1);
            console = {time};
            assets = {console, util};
            spriteBuffer = {camera};
            this->drawCallLimit = drawCallLimit;
            this->cullingEnabled = cullingEnabled;
            onCreate();
            return window.isOpen();
        }

        // \brief Start the application
        void start()
        {
            sf::Clock clock;
            sf::Time t;
            float currentTime, lastTime = 0;
            time.dt = 1;
            float timer;

            while(window.isOpen())
            {
                input.clear();
                sf::Event event;
                while(window.pollEvent(event))
                {
                    if(event.type == sf::Event::Closed)
                    {
                        assets.clear();
                        exit();
                    } 
                    else input.poll(event, window);
                }

                drawCalls = 0;

                window.setView(camera);

                onUpdate();

                drawCalls += spriteBuffer.update(drawCalls, drawCallLimit);
                window.draw(spriteBuffer);

                window.display();

                t = clock.getElapsedTime();
                currentTime = t.asSeconds();
                time.dt = currentTime - lastTime;
                lastTime = currentTime;

                if(showFps)
                {
                    timer += time.dt;
                    if(timer >= 0.5)
                    {
                        timer = 0;
                        int fps = 1 / time.dt;
                        window.setTitle(appName + " | " + std::to_string(fps) + " FPS");
                    }
                }
            }
        }

        // \brief Clears the screen
        // \param colour        the colour to clear to the screen
        void clear(int colour = Colour::Black)
        {
            window.clear(fromInt(colour));
        }

        // \brief Closes the window and exits
        // \param status        the exit status of the application
        void exit(int status = 0)
        {
            onDestroy();
            window.close();
            std::exit(status);
        }

        // \brief Sets a pixel in the pixel buffer
        void setPixel(int x, int y, int colour)
        {
            if(!renderPixels)
            {
                renderPixels = true;
                pixelBuffer.create(camera.getSize().x, camera.getSize().y);
            }
            pixelBuffer.setPixel(x, y, fromInt(colour));
        }

        void setPixels(int startX, int startY, int endX, int endY, int colour)
        {
            if(!renderPixels)
            {
                renderPixels = true;
                pixelBuffer.create(camera.getSize().x, camera.getSize().y);
            }
            for(int x = startX; x <= endX; x++)
            {
                for(int y = startY; y <= endY; y++)
                {
                    pixelBuffer.setPixel(x, y, fromInt(colour));
                }
            }
        }

        // \brief Draws the pixel buffer to the screen
        void drawPixels()
        {
            if(renderPixels)
            {
                if(drawCalls < drawCallLimit)
                {
                    drawCalls++;

                    renderPixels = false;
                    sf::Texture tex;
                    tex.loadFromImage(pixelBuffer);
                    sf::Sprite spr(tex);
                    spr.setOrigin({camera.getSize().x / 2.f, camera.getSize().y / 2.f});
                    spr.setPosition(camera.getCenter());
                    window.draw(spr);
                }
            }
        }

        // \brief Draws a rectangle outline
        // \param culling - if enabled, the renderer will not attempt to draw the object if it is not on screen.
        // only works if the window was created with the cullingEnabled parameter set to true
        void drawRect(int x, int y, int w, int h, int stroke = 1, int outlineColour = Colour::White, int drawMode = topLeft, bool culling = true)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                if(culling && cullingEnabled)
                {
                    if(drawMode == topLeft) if(x + w < camera.getCenter().x - (camera.getSize().x / 2.f) || x > camera.getCenter().x + (camera.getSize().x / 2.f) || y + h < camera.getCenter().y - (camera.getSize().y / 2.f) || y > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                    if(drawMode == center) if(x + (w / 2.f) < camera.getCenter().x - (camera.getSize().x / 2.f) || x - (w / 2.f) > camera.getCenter().x + (camera.getSize().x / 2.f) || y + (h / 2.f) < camera.getCenter().y - (camera.getSize().y / 2.f) || y - (h / 2.f) > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                }
                
                sf::RectangleShape rect({(float)w, (float)h});
                rect.setPosition({(float)x, (float)y});
                rect.setOutlineThickness(stroke);
                if(drawMode == center)
                {
                    rect.setOrigin({(float)w / 2.f, (float)h / 2.f});
                }
                rect.setOutlineColor(fromInt(outlineColour));
                rect.setFillColor(sf::Color::Transparent);
                window.draw(rect);
            }
        }

        // \brief Draws a filled rectangle
        // \param culling - if enabled, the renderer will not attempt to draw the object if it is not on screen.
        // only works if the window was created with the cullingEnabled parameter set to true
        void fillRect(int x, int y, int w, int h, int stroke = 1, int fillColour = Colour::White, int outlineColour = Colour::White, int drawMode = topLeft, bool culling = true)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                if(culling && cullingEnabled)
                {
                    if(drawMode == topLeft) if(x + w < camera.getCenter().x - (camera.getSize().x / 2.f) || x > camera.getCenter().x + (camera.getSize().x / 2.f) || y + h < camera.getCenter().y - (camera.getSize().y / 2.f) || y > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                    if(drawMode == center) if(x + (w / 2.f) < camera.getCenter().x - (camera.getSize().x / 2.f) || x - (w / 2.f) > camera.getCenter().x + (camera.getSize().x / 2.f) || y + (h / 2.f) < camera.getCenter().y - (camera.getSize().y / 2.f) || y - (h / 2.f) > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                }

                sf::RectangleShape rect({(float)w, (float)h});
                rect.setPosition({(float)x, (float)y});
                rect.setOutlineThickness(stroke);
                if(drawMode == center)
                {
                    rect.setOrigin({(float)w / 2.f, (float)h / 2.f});
                }
                rect.setOutlineColor(fromInt(outlineColour));
                rect.setFillColor(fromInt(fillColour));
                window.draw(rect);
            }
        }

        // \brief Draws a line
        void drawLine(int x1, int y1, int x2, int y2, int colour = Colour::White)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                sf::Vertex line[] = {
                    sf::Vertex({(float)x1, (float)y1}),
                    sf::Vertex({(float)x2, (float)y2})
                };
                line[0].color = fromInt(colour);
                line[1].color = fromInt(colour);
                window.draw(line, 2, sf::Lines);
            }
        }

        // \brief Draws a circle outline
        // \param culling - if enabled, the renderer will not attempy = y;t to draw the object if it is not on screen.
        // only works if the window was created with the cullingEnabled parameter set to true
        void drawCircle(int x, int y, int r, int stroke = 1, int outlineColour = Colour::White, int drawMode = topLeft, bool culling = true)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                if(culling && cullingEnabled)
                {
                    if(drawMode == topLeft) if(x + (r * 2) < camera.getCenter().x - (camera.getSize().x / 2.f) || x > camera.getCenter().x + (camera.getSize().x / 2.f) || y + (r * 2) < camera.getCenter().y - (camera.getSize().y / 2.f) || y > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                    if(drawMode == center) if(x + r < camera.getCenter().x - (camera.getSize().x / 2.f) || x - r > camera.getCenter().x + (camera.getSize().x / 2.f) || y + r < camera.getCenter().y - (camera.getSize().y / 2.f) || y - r > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                }

                sf::CircleShape circle((float)r);
                circle.setOutlineThickness(stroke);
                if(drawMode == center) circle.setOrigin({(float)r, (float)r});
                circle.setPosition({(float)x, (float)y});
                circle.setOutlineColor(fromInt(outlineColour));
                circle.setFillColor(sf::Color::Transparent);
                window.draw(circle);
            }
        }

        // \brief Draws a filled circle
        // \param culling - if enabled, the renderer will not attempt to draw the object if it is not on screen.
        // only works if the window was created with the cullingEnabled parameter set to true
        void fillCircle(int x, int y, int r, int stroke = 1, int fillColour = Colour::White, int outlineColour = Colour::White, int drawMode = topLeft, bool culling = true)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;
                
                if(culling && cullingEnabled)
                {
                    if(drawMode == topLeft) if(x + (r * 2) < camera.getCenter().x - (camera.getSize().x / 2.f) || x > camera.getCenter().x + (camera.getSize().x / 2.f) || y + (r * 2) < camera.getCenter().y - (camera.getSize().y / 2.f) || y > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                    if(drawMode == center) if(x + r < camera.getCenter().x - (camera.getSize().x / 2.f) || x - r > camera.getCenter().x + (camera.getSize().x / 2.f) || y + r < camera.getCenter().y - (camera.getSize().y / 2.f) || y - r > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                }

                sf::CircleShape circle((float)r);
                circle.setOutlineThickness(stroke);
                if(drawMode == center) circle.setOrigin({(float)r, (float)r});
                circle.setPosition({(float)x, (float)y});
                circle.setOutlineColor(fromInt(outlineColour));
                circle.setFillColor(fromInt(fillColour));
                window.draw(circle);
            }
        }

        // \brief Draws a triangle outline
        void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int colour = Colour::White)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                sf::Vertex line[] = {
                    sf::Vertex({(float)x1, (float)y1}),
                    sf::Vertex({(float)x2, (float)y2}),
                    sf::Vertex({(float)x3, (float)y3})
                };
                line[0].color = fromInt(colour);
                line[1].color = fromInt(colour);
                line[2].color = fromInt(colour);
                window.draw(line, 3, sf::Lines);
            }
        }

        // \brief Draws a filled triangle
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int colour = Colour::White)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;
                
                sf::VertexArray tri(sf::Triangles, 3);
                tri[0].position = {(float)x1, (float)y1};
                tri[1].position = {(float)x2, (float)y2};
                tri[2].position = {(float)x3, (float)y3};
                tri[0].color = fromInt(colour);
                tri[1].color = fromInt(colour);
                tri[2].color = fromInt(colour);
                window.draw(tri);
            }
        }

        // \brief Draws a single sprite to the screen. Use atlasSprite and spriteBuffer for better performance
        // \param culling - if enabled, the renderer will not attempt to draw the object if it is not on screen.
        // only works if the window was created with the cullingEnabled parameter set to true
        void drawSprite(sprite spr, bool culling = true)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                if(culling && cullingEnabled)
                {
                    if(spr.getDrawMode() == topLeft) if(spr.getX() + spr.getWidth() < camera.getCenter().x - (camera.getSize().x / 2.f) || spr.getX() > camera.getCenter().x + (camera.getSize().x / 2.f) || spr.getY() + spr.getHeight() < camera.getCenter().y - (camera.getSize().y / 2.f) || spr.getY() > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                    if(spr.getDrawMode() == center) if(spr.getX() + (spr.getWidth() / 2.f) < camera.getCenter().x - (camera.getSize().x / 2.f) || spr.getX() - (spr.getWidth() / 2.f) > camera.getCenter().x + (camera.getSize().x / 2.f) || spr.getY() + (spr.getHeight() / 2.f) < camera.getCenter().y - (camera.getSize().y / 2.f) || spr.getY() - (spr.getHeight() / 2.f) > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                }
                
                if(!spr.isValid())
                {
                    console.logError("Invalid sprite draw call");
                    return;
                }
                sf::Sprite s;
                s.setTexture(spr.getTexture());
                if(spr.getDrawMode() == center) s.setOrigin({spr.getTexture().getSize().x / 2.f, spr.getTexture().getSize().y / 2.f});
                s.setPosition({(float)spr.getX(), (float)spr.getY()});
                float scaleX = (float)spr.getWidth() / (float)spr.getTexture().getSize().x;
                float scaleY = (float)spr.getHeight() / (float)spr.getTexture().getSize().y;
                s.setScale(sf::Vector2f(scaleX, scaleY));
                s.setRotation(spr.getRotation());
                window.draw(s);
            }
        }

        // \brief Draws text
        // \param culling - if enabled, the renderer will not attempt to draw the object if it is not on screen.
        // only works if the window was created with the cullingEnabled parameter set to true
        void drawString(int x, int y, std::string str, int charSize = 8, int colour = Colour::White, int drawMode = topLeft, bool culling = true)
        {
            if(drawCalls < drawCallLimit)
            {
                drawCalls++;

                if(!loadedFont) return;
                sf::Text text;
                text.setFont(font);
                text.setString({str.c_str()});
                text.setCharacterSize(charSize);
                sf::FloatRect textRect = text.getLocalBounds();

                if(culling && cullingEnabled)
                {
                    float w = textRect.width;
                    float h = textRect.height;
                    if(drawMode == topLeft) if(x + w < camera.getCenter().x - (camera.getSize().x / 2.f) || x > camera.getCenter().x + (camera.getSize().x / 2.f) || y + h < camera.getCenter().y - (camera.getSize().y / 2.f) || y > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                    if(drawMode == center) if(x + (w / 2.f) < camera.getCenter().x - (camera.getSize().x / 2.f) || x - (w / 2.f) > camera.getCenter().x + (camera.getSize().x / 2.f) || y + (h / 2.f) < camera.getCenter().y - (camera.getSize().y / 2.f) || y - (h / 2.f) > camera.getCenter().y + (camera.getSize().y / 2.f)) return;
                }

                text.setFillColor(fromInt(colour));
                text.setOutlineColor(fromInt(colour));
                if(drawMode == center)
                {
                    text.setOrigin(textRect.width / 2.0f, textRect.height / 2.0f);
                }
                text.setPosition({(float)x, (float)y});
                window.draw(text);
            }
        }

        // \brief Loads a font for drawing text. This must be called at least once before drawText()
        // \param fontFile      the filepath of the font
        // \return true if the font loaded successfully
        bool loadFont(std::string fontFile)
        {
            if(!font.loadFromFile(fontFile.c_str()))
            {
                std::cout << "Error loading font" << std::endl;
                return false;
            }
            loadedFont = true;
            return true;
        }

        void drawTileMap(TileMap map)
        {
            window.draw(map);
        }

        // \brief Sets the x position of the camera
        void setCamX(int camX)
        {
            camera.setCenter(camX, camera.getCenter().y);
        }

        // \brief Sets the y position of the camera
        void setCamY(int camY)
        {
            camera.setCenter(camera.getCenter().x, camY);
        }

        // \brief Sets the position of the camera
        void setCamPos(int x, int y)
        {
            camera.setCenter(x, y);
        }

        // \brief Sets the position of the camera relative to its current position
        void moveCamera(int x, int y)
        {
            camera.move(x, y);
        }

        // \brief Sets the zoom of the camera
        void setCamZoom(float zoom)
        {
            camZoom = zoom + 1;
            camZoom = mathf.clamp(0.01f, 10.f, camZoom);
            camera.zoom(camZoom);
        }

        // \brief Adds to the zoom of the camera
        void addCamZoom(float zoom)
        {
            camZoom += zoom;
            camZoom = mathf.clamp(0.01f, 10.f, camZoom);
            camera.zoom(camZoom);
        }

        void startPan()
        {
            startMX = -(((float)input.getMouseX() / (float)screenWidth()) * camera.getSize().x);
            startMY = -(((float)input.getMouseY() / (float)screenHeight()) * camera.getSize().y);
        }
        
        void pan(int x, int y)
        {
            float posx = -(((float)x / (float)screenWidth()) * camera.getSize().x);
            float posy = -(((float)y / (float)screenHeight()) * camera.getSize().y);
            camera.move(posx - startMX, posy - startMY);
            startMX = posx;
            startMY = posy;
        }

        int getCamX()
        {
            return camera.getCenter().x;
        }

        int getCamY()
        {
            return camera.getCenter().y;
        }

        // \brief Returns the window width in pixels
        int screenWidth()
        {
            return sWidth;
        }

        // \brief Returns the window height in pixels
        int screenHeight()
        {
            return sHeight;
        }

        // \brief Set the limit of draw calls per frame
        void setDrawCallLimit(int limit)
        {
            drawCallLimit = limit;
        }

        // \brief Get the limit of draw calls per frame
        int getDrawCallLimit()
        {
            return drawCallLimit;
        }

        // \brief Get the number of draw calls processed so far this frame
        int getDrawCalls()
        {
            return drawCalls;
        }

        // \brief Returns the current frame rate
        int frameRate()
        {
            return 1.f / time.dt;
        }

        // \brief The name of the application. Set this before calling create()
        std::string appName;
        // \brief Input handler
        Input input;
        // \brief Utility functions
        Util util;
        // \brief HTML colours - visit https://en.wikipedia.org/wiki/Web_colors#Extended_colors for built in HTML colours and their hex values. Converts r, g, b and a values into a colour.
        Colour colour;
        // \brief Keyboard keycodes
        Keyboard keyboard;
        // \brief Mouse button values
        Mouse mouse;
        // \brief Time manager
        Time time;
        // \brief Maths functions
        Math mathf;
        // \brief Camera control
        sf::View camera;
        // \brief Console handler
        Console console;
        // \brief Asset manager
        Assets assets;
        // \brief A fast way to draw multiple sprites
        SpriteBuffer spriteBuffer;

    private:
        int sWidth, sHeight;
        sf::RenderWindow window;
        bool fullscreen = false;
        bool showFps = false;
        bool loadedFont = false;
        sf::Font font;
        sf::Image pixelBuffer;
        bool renderPixels = false;
        float camZoom = 1;
        float startMX, startMY;
        int drawCallLimit = 1000;
        int drawCalls;
        bool cullingEnabled = true;

        sf::Color fromInt(int colour)
        {
            return sf::Color({(unsigned int)colour});
        }
    };  
}

#endif
