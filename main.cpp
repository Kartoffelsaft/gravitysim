#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <chrono>
#include <thread>
#include <vector>
#include <math.h>
#include <random>
#include <fstream>
#include <string>
#include <cstring>


#define __name__ "grav"

struct MassPoint
{
public:
    float posX;
    float posY;

    float velX;
    float velY;

    MassPoint(){}

    MassPoint(std::normal_distribution<float> rand, std::default_random_engine &g, float velocityRatio)
    {
        posX = rand(g);
        posY = rand(g);
        velX = (rand(g) - rand.mean()) / velocityRatio;
        velY = (rand(g) - rand.mean()) / velocityRatio;
    }

    float getDist(MassPoint other)
    {
        return std::sqrt
        (
            (this->posX - other.posX) *
            (this->posX - other.posX) +
            (this->posY - other.posY) *
            (this->posY - other.posY) 
        );
    }

    float calcForce(MassPoint other, float bigG)
    {
        if(this->posX == other.posY && this->posY == other.posY) return 0;
        return bigG/
        (
            (this->posX - other.posX) *
            (this->posX - other.posX) +
            (this->posY - other.posY) *
            (this->posY - other.posY) 
        );
    }

    void deltaV(MassPoint other, float bigG)
    {
        auto dist = this->getDist(other);
        if(dist == 0) return;
        if(dist < 4)
        {
            auto velXAvg = (this->velX + other.velX)/2;
            auto velYAvg = (this->velY + other.velY)/2;

            this->velX = velXAvg;
            this->velY = velYAvg;

            return;
        }

        auto force = this->calcForce(other, bigG);

        velX += force * ((this->posX-other.posX)/dist);
        velY += force * ((this->posY-other.posY)/dist);
    }

    void deltaD()
    {
        this->posX -= this->velX;
        this->posY -= this->velY;
    }
};

struct ConfigInfo
{
public:
    int resX;
    int resY;
    int antialiasing;

    float zoom;
    float zoomRate;

    float bigG;
    size_t massCount;

    float randDeviation;
    float velocityRatio;

    ConfigInfo(char* const configFile)
    {
        std::ifstream fs(configFile);
        if(!fs.is_open())
        {throw "config file dissapeared";}
        while(!fs.eof())
        {
            std::string line;
            getline(fs, line);

            int splitIndex = line.find(":");
            if(splitIndex < 0) continue;
            const char* varName = line.substr(0, splitIndex).c_str();
            
            if(!strcmp(varName, "resX"))
            {resX = std::stoi(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "resY"))
            {resY = std::stoi(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "antialiasing"))
            {antialiasing = std::stoi(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "zoom"))
            {zoom = std::stof(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "zoomRate"))
            {zoomRate = std::stof(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "bigG"))
            {bigG = std::stof(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "massCount"))
            {massCount = std::stoi(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "randDeviation"))
            {randDeviation = std::stof(line.substr(splitIndex+1));}
            else if(!strcmp(varName, "velocityRatio"))
            {velocityRatio = std::stof(line.substr(splitIndex+1));}
            else std::cout << "config parser could not interperet " << line;
        }
        fs.close();
    }
};

void calcMasses(std::vector<MassPoint> &masses, float bigG)
{
    for(auto &m1 : masses)
    {
        for(auto &m2 : masses)
        {
            m1.deltaV(m2, bigG);
        }
    }

    for(auto &m : masses)
    {m.deltaD();}
}

int main()
{
    ConfigInfo cfg("./config.txt");

    sf::ContextSettings settings; settings.antialiasingLevel = cfg.antialiasing;
    sf::RenderWindow window(sf::VideoMode(cfg.resX, cfg.resY), __name__, sf::Style::Default, settings);

    std::default_random_engine g((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
    std::normal_distribution<float> rand{0, cfg.randDeviation};

    std::vector<MassPoint> objects(cfg.massCount);
    for(auto &m : objects)
    {m = MassPoint(rand, g, cfg.velocityRatio);}

    while(window.isOpen())
    {
        sf::Event event;
        while(window.pollEvent(event))
        {
            switch(event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::MouseWheelScrolled:
                    cfg.zoom -= event.mouseWheelScroll.delta * 0.1;
                    break;
                default: {};
            }
        }

        calcMasses(objects, cfg.bigG);

        window.clear(sf::Color::Black);
        
        for(auto o : objects)
        {
            sf::CircleShape object(2/cfg.zoom);
            object.setFillColor(sf::Color::White);
            object.setPosition(o.posX/cfg.zoom + cfg.resX/2, o.posY/cfg.zoom + cfg.resY/2);
            window.draw(object);
        }

        window.display();

        cfg.zoom += cfg.zoomRate;

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    return 0;
}

