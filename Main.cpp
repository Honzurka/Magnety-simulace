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


class Math {
    public:
    static const float pi;
    static sf::Vector2f Rotate(const sf::Vector2f& input, float angle) {
        float angleRad = DegToRad(angle);
        float x = cos(angleRad) * input.x - sin(angleRad) * input.y;
        float y = sin(angleRad) * input.x + cos(angleRad) * input.y;
        return sf::Vector2f(x, y);
    }
    static float DotProduct(sf::Vector2f v1, sf::Vector2f v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    static float VecLen(sf::Vector2f vec) {
        return sqrt(vec.x * vec.x + vec.y * vec.y);
    }
    static sf::Vector2f NormalizeVec(sf::Vector2f vec) {
        float vecLen = VecLen(vec);
        if (vecLen == 0) {
            throw "Cannot normalize zero vector";
        }
        return sf::Vector2f(vec.x / vecLen, vec.y / vecLen);
    }
    static float DegToRad(float deg) {
        return deg / 180 * pi;
    }
    static float RadToDeg(float rad) {
        return rad / pi * 180;
    }
    static float GetAngleRad(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        float v1Len = VecLen(v1);
        float v2Len = VecLen(v2);
        if (v1Len == 0 || v2Len == 0)
            return 0;

        float dot = DotProduct(v1, v2);
        dot = dot / v1Len / v2Len;
        
        //number rounding fix
        if (dot > 1)
            dot = 1;
        if (dot < -1)
            dot = -1;

        return acos(dot);
    }
    static float GetAngleDeg(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        float angleRad = GetAngleRad(v1, v2);
        return RadToDeg(angleRad);
    }
    //returns angle(DEG) between inVec and (1,0) rotated by deg
    static float AngleFromBaseX(float deg, sf::Vector2f inVec) {
        sf::Vector2f xBase(1, 0);
        sf::Vector2f xDir = Math::Rotate(xBase, deg);
        return Math::GetAngleDeg(xDir, inVec);
    }
    //returns angle(DEG) between inVec and (0,-1) rotated by deg
    static float AngleFromBaseY(float deg, sf::Vector2f inVec) {
        sf::Vector2f yBase(0, -1);
        sf::Vector2f yDir = Math::Rotate(yBase, deg);
        return Math::GetAngleDeg(yDir, inVec);
    }
    //returns angle(in DEG) from interval (-180,180) between inVec and vector(1,0)
    static float TrueAngleFromBase(sf::Vector2f inVec, float deg) {
        float xAngle = AngleFromBaseX(deg, inVec);
        float yAngle = AngleFromBaseY(deg, inVec);
        auto xx = AngleFromBaseX(deg, inVec);
        return GetTrueAngle(xAngle, yAngle);
    }
    private:
    //returns angle(DEG) based on quadrant determined by args
    static float GetTrueAngle(float xAngle, float yAngle) {
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
const float Math::pi = 3.14159265;

//ToDo
struct Params {
    sf::Texture texture;
    float magForceCoef; //multiplies Magnets movement vector
    float radius;
    float rotationCoef; //interval (0,1> - how much should magnet rotate towards final destination
    //float inertia //<0,1) how much energy should be conserved from previous movement

    Params SetDefault() {
        if (!texture.loadFromFile("textures/RB.png")) {
            throw "Unable to load texture from file";
        }
        magForceCoef = 100000;
        radius = 20;
        rotationCoef = 20; //debug, should be ~10
    }
};

class Magnet {
    public:
        sf::Vector2f movement;
        sf::Vector2f rotation;
        sf::Vector2f lastMovement;
        const Params& params;
        sf::CircleShape shape;
        
        Magnet() = delete;
        Magnet(const sf::Texture& texture, const sf::Vector3f& loc, const Params& params) :params(params){
            shape.setRadius(params.radius);
            shape.setOrigin(params.radius, params.radius);
            shape.setPosition(loc.x,loc.y);
            shape.setRotation(loc.z);
            shape.setTexture(&texture);
        }
        void Move() {
            shape.move(movement); //disabled
            shape.rotate(GetRotationDeg());

            rotation = sf::Vector2f();
            lastMovement = movement;
            movement = sf::Vector2f();
        }

        //for simple collision detection-----------------------------
        void MoveReverse() {
            shape.move(-lastMovement);
        }

        bool CollidesWith(Magnet other) {
            float distance = Math::VecLen(this->shape.getPosition() - other.shape.getPosition());
            float limit = this->params.radius + other.params.radius;
            return distance < limit;
        }

    private:
        //returns degree by which will be magnet rotated in next step
        float GetRotationDeg() {
            float angleFromBase = Math::TrueAngleFromBase(rotation, shape.getRotation());
            float rotDeg;
            if (abs(angleFromBase) <= 90) {
                rotDeg = -angleFromBase / params.rotationCoef;
            }
            else {
                rotDeg = (180 - abs(angleFromBase)) / params.rotationCoef;
            }
            return rotDeg;
        }
};

class Simulation {
    public:
        Simulation() = delete;
        Simulation(sf::RenderWindow& win, const Params& params) : window(win),params(params) {
            //ToDo: random generation
            auto sz = window.getSize();
            srand(time(0));
            for (size_t i = 0; i < 30; ++i) {
                size_t x = rand() % sz.x;
                size_t y = rand() % sz.y;
                size_t deg = rand() % 360;
                Magnet m(params.texture, sf::Vector3f(x, y, deg), params);
                magnets.push_back(m);
            }

            /*/
            Magnet m1(params.texture, sf::Vector3f(600,200, 20), params);
            magnets.push_back(m1);
            Magnet m2(params.texture, sf::Vector3f(700, 200, 180), params);
            magnets.push_back(m2);
            Magnet m3(params.texture, sf::Vector3f(650, 300, 40), params);
            magnets.push_back(m3);
            /**/
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
                window.draw(m.shape);
            }
        }
    private:
        sf::RenderWindow& window;
        std::vector<Magnet> magnets;
        const Params& params;

        //alters magnets movement and rotaions
        void ApplyMagForce(Magnet& m1, Magnet& m2) {
            float ForceMagitude = GetForceMag(m1, m2);
            sf::Vector2f ForceDir = Math::NormalizeVec(m2.shape.getPosition() - m1.shape.getPosition());
            sf::Vector2f Force = ForceMagitude * ForceDir;

            m1.movement += Force;
            m2.movement -= Force;

            m1.rotation += Force;
            m2.rotation += Force;
        }

        //returns magnitude of magnetic force between 2 magnets
        float GetForceMag(const Magnet& m1, const Magnet& m2) {
            sf::Vector2f dir = m2.shape.getPosition() - m1.shape.getPosition();
            float dist = Math::VecLen(dir);

            float angleM1 = Math::DegToRad(Math::AngleFromBaseX(m1.shape.getRotation(), dir));
            float angleM2 = Math::DegToRad(Math::AngleFromBaseX(m2.shape.getRotation(), dir));

            float FMagnitude = cos(angleM1) * cos(angleM2) / (dist * dist) * params.magForceCoef;
            return FMagnitude;
        }

        //ToDo------------------------------------------------------------------
        void SimpleCollisionDetection() {
            //with magnets
            for (size_t i = 0; i < magnets.size(); ++i) {
                if (CollidesWithWindow(magnets[i])) {
                    magnets[i].MoveReverse();
                }
                for (size_t j = i + 1; j < magnets.size(); ++j) {
                    if (magnets[i].CollidesWith(magnets[j])) {
                        magnets[i].MoveReverse();
                        //magnets[j].MoveReverse();
                    }
                }
            }

            //with window
        }

        //beta-window collision
        bool CollidesWithWindow(Magnet m) {
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
    window.setVerticalSyncEnabled(true);
    //window.setFramerateLimit(20); //debug
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