#include "imgui.h" // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h
#include "imgui-SFML.h" // for ImGui::SFML::* functions and SFML-specific overloads

#include <SFML/Graphics.hpp>
//#include <SFML/Graphics/CircleShape.hpp>
//#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <iostream> //debug
#include <vector>

/* https://eliasdaler.github.io/using-imgui-with-sfml-pt2/ */
/*
    Labels
        should be different (## for unique id)

    ImGui begin/end
        creates new window

*/
/* https://github.com/eliasdaler/imgui-sfml - ImGuI + SFML */
/*
    ImGui::SFML::Init(window)
    window.resetGLStates();
        call it if you only draw ImGui. Otherwise not needed.
    ImGui::SFML::Update
        clock used by ImGui
    ImGui::SFML::Render(window)
        misto ImGui::Render
    !!!MEZI Update a Render: volam ImGui tvorive fce

    SFML: clear before drawing -> then display
*/

using Vector2d = sf::Vector2<double>;

class Math {
    public:
    static sf::Vector2f Rotate(const sf::Vector2f& input, float angle) {
        double angleRad = DegToRad(angle);
        float x = cos(angleRad) * input.x - sin(angleRad) * input.y;
        float y = sin(angleRad) * input.x + cos(angleRad) * input.y;
        return sf::Vector2f(x, y);
    }
    static double DotProduct(sf::Vector2f v1, sf::Vector2f v2) {
        return (double)v1.x * v2.x + (double)v1.y * v2.y;
    }
    static double VecLen(sf::Vector2f vec) {
        return sqrt(vec.x * vec.x + vec.y * vec.y);
    }
    static sf::Vector2f NormalizeVec(sf::Vector2f vec) {
        float vecLen = VecLen(vec);
        return sf::Vector2f(vec.x / vecLen, vec.y / vecLen);
    }

    static double DegToRad(double deg) {
        return deg / 180 * pi;
    }

    static double RadToDeg(double rad) {
        return rad / pi * 180;
    }

    static Vector2d GetVec(Vector2d pos1, Vector2d pos2) {
        return pos2 - pos1;
    }

    static const double pi;
};
const double Math::pi = 3.14159265;

struct Params {
    sf::Texture texture;
    float magForceCoeff;

    Params SetDefault() {
        if (!texture.loadFromFile("textures/RB.png")) {
            throw; //should be tested
        }
        magForceCoeff = 10000;
    }
};

class Magnet {
    public:
        sf::Vector2f movement; //is init to 0??
        sf::Vector2f lastMovement;

        Magnet() = delete;
        Magnet(const sf::Texture& texture, const sf::Vector3f& loc, float radius = 30){    //chci absolutni coords????----------- //proc vec3f??
            shape.setRadius(radius);
            shape.setOrigin(radius, radius);
            shape.setPosition(loc.x,loc.y);
            shape.setRotation(loc.z);
            shape.setTexture(&texture);
        }
        void Move() {
            shape.move(movement);   //no rotation yet...----------
            lastMovement = movement;
            movement = sf::Vector2f(); //hopefully zero...
        }

        sf::CircleShape GetShape() const {
            return shape;
        }

        sf::Vector2f GetDirection() const {
            sf::Vector2f base(1, 0);
            sf::Vector2f dir = Math::Rotate(base, shape.getRotation());
            return dir;
        }
    private:
        sf::CircleShape shape;
};

class Simulation {
    public:
        Simulation() = delete; //jake ma dusledky--------------(na dtor,...)
        Simulation(sf::RenderWindow& win, const Params& params) : window(win),params(params) {

            Magnet m1(params.texture, sf::Vector3f(300,200, 0));
            magnets.push_back(m1);
            
            Magnet m2(params.texture, sf::Vector3f(900, 200, 0));
            magnets.push_back(m2);
        }

        void Step() {
            for (size_t i = 0; i < magnets.size(); ++i) {
                for (size_t j = i + 1; j < magnets.size(); ++j) {
                    sf::Vector2f magForce = GetMagForce(magnets[i], magnets[j]);
                    magnets[i].movement += magForce;
                    magnets[j].movement -= magForce;
                }
                magnets[i].Move();
            }
            //DetectColision(...)
        }

        void Draw() {
            for (auto&& m : magnets) {
                window.draw(m.GetShape());
            }
        }
    private:
        sf::RenderWindow& window;
        std::vector<Magnet> magnets;
        const Params& params;

        //
        sf::Vector2f GetMagForce(const Magnet& m1, const Magnet& m2) {
            auto shapeM1 = m1.GetShape();
            auto shapeM2 = m2.GetShape();

            //spojnice
            sf::Vector2f direction = shapeM2.getPosition() - shapeM1.getPosition(); //vec 1->2
            sf::Vector2f directionNorm = Math::NormalizeVec(direction);

            //m1 od spojnice
            auto dirM1 = m1.GetDirection();
            double angleM1 = acos(Math::DotProduct(dirM1, directionNorm)); //rad

            //m2 od spojnice
            auto dirM2 = m2.GetDirection();
            double angleM2 = acos(Math::DotProduct(dirM2, directionNorm));
            //std::cout << "1a: " << Math::RadToDeg(angleM1) << "2a: " << Math::RadToDeg(angleM2) << std::endl;

            //zatim bez rozliseni polorovin--------------------------------
            double distance = Math::VecLen(shapeM2.getPosition() - shapeM1.getPosition());
            double FMagnitude = cos(angleM1) * cos(angleM2) / (distance*distance) * params.magForceCoeff;
            //std::cout << "a1: "<< Math::RadToDeg(angleM1) << " a2: " << Math::RadToDeg(angleM2) << " Magnitude: "<< FMagnitude << std::endl;

            return sf::Vector2f(directionNorm.x * FMagnitude, directionNorm.y * FMagnitude);
        }
};

void ShowMenu(const sf::Window& win, Params& params) {
    ImGui::ShowDemoWindow();

    ImGui::Begin("Hello, world!");
    ImGui::Button("Look at this pretty button");
    ImGui::End();
}

int main() {
    sf::VideoMode fullscreen = sf::VideoMode::getDesktopMode();
    float winScale = 0.5;
    
    sf::RenderWindow window(sf::VideoMode(fullscreen.width * winScale, fullscreen.height*winScale), "Magnet Simulation");
    window.setVerticalSyncEnabled(true);
    ImGui::SFML::Init(window);

    Params params;
    params.SetDefault(); //can throw

    sf::Clock deltaClock;
    Simulation sim(window, params);
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        ShowMenu(window, params);
        sim.Step();

        window.clear(sf::Color::White);
        sim.Draw();
        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::SFML::Shutdown();
    return 0;
}