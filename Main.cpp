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
    static const double pi;

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
    static float GetAngleRad(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        float v1Len = VecLen(v1);
        float v2Len = VecLen(v2);
        float dot = DotProduct(v1, v2);
        if (v1Len == 0 || v2Len == 0)
            return 0; //correct???---
        dot = dot / v1Len / v2Len;
        return acos(dot);
    }
    static float GetAngleDeg(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        float angleRad = GetAngleRad(v1, v2);
        return RadToDeg(angleRad);
    }

    //returns degree between input
    //vector(1,0) or vector(-1,0)
    //always to the closer one
    static float AngleFromBase(sf::Vector2f inVec, float deg) {
        sf::Vector2f xBase(1, 0);
        sf::Vector2f yBase(0, -1);

        sf::Vector2f xDir = Math::Rotate(xBase, deg);
        sf::Vector2f yDir = Math::Rotate(yBase, deg);
        //std::cout << "x: " << xDir.x << " " << xDir.y << " " << "y: " << yDir.x << " " << yDir.y << " " << std::endl;

        float xAngle = Math::GetAngleDeg(xDir, inVec);
        float yAngle = Math::GetAngleDeg(yDir, inVec);

        std::cout << "xAngle:" << xAngle << " yAngle: " << yAngle << std::endl;
        std::cout << "TrueAngle: " << getTrueAngle(xAngle, yAngle) << std::endl;

        return getTrueAngle(xAngle, yAngle);
    }
    private:
        //returns degree based on quadrant
        static float getTrueAngle(float xAngle, float yAngle) {
            if (xAngle <= 90) {
                if (yAngle >= 90) { //270-360°
                    return -xAngle;// 360 - xAngle;
                }
                else { //0-90°
                    return xAngle;
                }
            }
            else {
                if (yAngle >= 90) { //180-270°
                    return -xAngle;// 360 - xAngle;
                }
                else { //90-180°
                    return xAngle;
                }
            }
        }
};
const double Math::pi = 3.14159265;

struct Params {
    sf::Texture texture;
    float magForceCoeff;
    float radius;

    Params SetDefault() {
        if (!texture.loadFromFile("textures/RB.png")) {
            throw; //should be tested
        }
        magForceCoeff = 10000;
        radius = 30;
    }
};

class Magnet {
    public:
        sf::Vector2f movement; //is init to 0??
        sf::Vector2f rotation;
        sf::Vector2f lastMovement;
        sf::Vector2f lastRotation;

        float radius; //in case of different sized magnets

        Magnet() = delete;
        Magnet(const sf::Texture& texture, const sf::Vector3f& loc, float radius){    //chci absolutni coords????----------- //proc vec3f??
            shape.setRadius(radius);
            shape.setOrigin(radius, radius);
            shape.setPosition(loc.x,loc.y);
            shape.setRotation(loc.z);
            shape.setTexture(&texture);
            this->radius = radius;
        }
        void Move() {
            shape.move(movement);   //disabled for now
            

            const int rot = 10;
            float degFromDir = Math::AngleFromBase(rotation, shape.getRotation());
            if (abs(degFromDir) <= 90) {
                shape.rotate(-degFromDir / rot);
            }
            else {
                shape.rotate((180 - abs(degFromDir)) / rot);
            }
            

            lastRotation = rotation; //std::move??
            rotation = sf::Vector2f();
            lastMovement = movement;
            movement = sf::Vector2f(); //hopefully zero...
        }

        //for simple collision detection
        void MoveReverse() {
            shape.move(-lastMovement);
        }

        bool CollidesWith(Magnet other) {
            double distance = Math::VecLen(this->shape.getPosition() - other.GetShape().getPosition());
            double limit = this->radius + other.radius;
            return distance < limit;
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

            Magnet m1(params.texture, sf::Vector3f(600,200, 20), params.radius);
            magnets.push_back(m1);
            
            Magnet m2(params.texture, sf::Vector3f(700, 200, 180), params.radius);
            magnets.push_back(m2);
        }

        void Step() {
            for (size_t i = 0; i < magnets.size(); ++i) {
                for (size_t j = i + 1; j < magnets.size(); ++j) {
                    ApplyMagForce(magnets[i], magnets[j]);
                }
                magnets[i].Move();
            }
            SimpleCollisionDetection();
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

        void ApplyMagForce(Magnet& m1, Magnet& m2) {
            float ForceMagitude = GetForceMag(m1, m2);
            sf::Vector2f ForceDir = Math::NormalizeVec(m2.GetShape().getPosition() - m1.GetShape().getPosition());
            sf::Vector2f Force = ForceMagitude * ForceDir;
            std::cout << "FM: " << ForceMagitude << std::endl;
            m1.movement += Force;
            m2.movement -= Force;

            m1.rotation += Force;
            m2.rotation += Force;
        }

        //
        float GetForceMag(const Magnet& m1, const Magnet& m2) {
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
            //kladna/zaporna - urcuje pritahovani/odpuzovani => rotaci


            //std::cout << "a1: "<< Math::RadToDeg(angleM1) << " a2: " << Math::RadToDeg(angleM2) << " Magnitude: "<< FMagnitude << std::endl;

            return FMagnitude;
            //return sf::Vector2f(directionNorm.x * FMagnitude, directionNorm.y * FMagnitude);
        }

        void SimpleCollisionDetection() {
            //with magnets
            for (size_t i = 0; i < magnets.size(); ++i) {
                for (size_t j = i + 1; j < magnets.size(); ++j) {
                    if (magnets[i].CollidesWith(magnets[j])) {
                        magnets[i].MoveReverse();
                        magnets[j].MoveReverse();
                    }
                }
            }
            //with window

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
    
    sf::RenderWindow window(sf::VideoMode(fullscreen.width * winScale, fullscreen.height * winScale), "Magnet Simulation");
    //window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(5); //debug
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