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


const int menuWidth = 200;

class Math {
    public:
    static const float pi;
    static sf::Vector2f Rotate(const sf::Vector2f& input, float angle) {
        float angleRad = DegToRad(angle);
        float x = cos(angleRad) * input.x - sin(angleRad) * input.y;
        float y = sin(angleRad) * input.x + cos(angleRad) * input.y;
        return sf::Vector2f(x, y);
    }
    static float DotProduct(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    static float VecLen(const sf::Vector2f& vec) {
        return sqrt(vec.x * vec.x + vec.y * vec.y);
    }
    //if zero vector is passed then its returned back
    static sf::Vector2f NormalizeVec(const sf::Vector2f& vec) {
        float vecLen = VecLen(vec);
        if (vecLen == 0) {
            return vec;
        }
        return sf::Vector2f(vec.x / vecLen, vec.y / vecLen);
    }
    static sf::Vector2f SetLen(const sf::Vector2f& vec, float len) {
        sf::Vector2f res = NormalizeVec(vec);
        res *= len;
        return res;
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
    static float AngleFromBaseX(float deg, const sf::Vector2f& inVec) {
        sf::Vector2f xBase(1, 0);
        sf::Vector2f xDir = Math::Rotate(xBase, deg);
        return Math::GetAngleDeg(xDir, inVec);
    }
    //returns angle(DEG) between inVec and (0,-1) rotated by deg
    static float AngleFromBaseY(float deg, const sf::Vector2f& inVec) {
        sf::Vector2f yBase(0, -1);
        sf::Vector2f yDir = Math::Rotate(yBase, deg);
        return Math::GetAngleDeg(yDir, inVec);
    }
    //returns angle(in DEG) from interval (-180,180) between inVec and vector(1,0)
    static float TrueAngleFromBase(const sf::Vector2f& inVec, float deg) {
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
                    return -xAngle;
                }
                else { //0-90°
                    return xAngle;
                }
            }
            else {
                if (yAngle >= 90) { //180-270°
                    return -xAngle;
                }
                else { //90-180°
                    return xAngle;
                }
            }
        }
};
const float Math::pi = 3.14159265f;

//ToDo
struct Params {
    sf::Texture texture;
    float radius = 20;
    int magCount = 3;

    float movCoef = 8.f; //>1, <2*radius (jinak se preskakuji)
    float rotationCoef = 0.3f; //interval (0,1)
    float inertia = 0.1f; //(0,1) how much energy should be conserved from previous movement --might get wild with both movCoef and inertia high
    float fConst = 0.00001f;
    float fConstMult = 10.f;

    bool isRunning = true;
    void SetDefault() {
        if (!texture.loadFromFile("textures/RB.png")) {
            throw "Unable to load texture from file";
        }
    }
};

class Magnet {
    public:
        sf::Vector2f movement;
        sf::Vector2f lastMovement;
        float rotation = 0;
        float lastRotation = 0;
        const Params& params;
        sf::CircleShape shape;
        
        Magnet() = delete;
        Magnet(const sf::Texture& texture, const sf::Vector3<size_t>& loc, const Params& params) :params(params){
            shape.setRadius(params.radius);
            shape.setOrigin(params.radius, params.radius);
            shape.setPosition(static_cast<float>(loc.x), static_cast<float>(loc.y));
            shape.setRotation(static_cast<float>(loc.z));
            shape.setTexture(&texture);
        }
        void Move() {
            shape.move(movement);
            shape.rotate(rotation);

            lastRotation = rotation;
            rotation = 0;
            lastMovement = movement;
            movement = sf::Vector2f();
        }

        void ApplyCoeffs() {
            movement = ((1 - params.inertia) * movement + params.inertia * lastMovement) * params.movCoef;
            rotation = ((1 - params.inertia) * rotation + params.inertia * lastRotation) * params.rotationCoef;
        }

        //uprava-nezavisle x,y ?? chci zajistit odvalovani
        //todo: refactor
        void ResolveMagCollision(Magnet& other) {
            if (!CollidesWith(other)) return;
            float dist = Math::VecLen(this->shape.getPosition() - other.shape.getPosition());
            float freeDist = dist - this->params.radius - other.params.radius;

            float thisLen = Math::VecLen(this->movement);
            float otherLen = Math::VecLen(other.movement);
            if (thisLen == 0) {
                other.movement = Math::SetLen(other.movement, freeDist);
                return;
            }
            if (otherLen == 0) {
                this->movement = Math::SetLen(this->movement, freeDist);
                return;
            }

            float thisNewLen = freeDist / (thisLen + otherLen) * thisLen;
            this->movement = Math::SetLen(this->movement, thisNewLen);
            
            //other movement pricvaknu
            float otherNewLen = freeDist - thisNewLen; //bude fungovat presne??
            other.movement = Math::SetLen(other.movement, otherNewLen);
        }

        //zjednodusene, neni presne !!! y +ani- moc nefunguje...
        void ResolveWinCollision(sf::Vector2u winSz) {
            sf::Vector2f pos = shape.getPosition();
            float radius = params.radius;
            float nextX = pos.x + movement.x;
            float nextY = pos.y + movement.y;
            float comp;
            if ((comp = nextX - radius) < menuWidth) { //menu on left side
                movement.x -= comp - menuWidth;
                return;
            }
            if ((comp = nextX + radius) > winSz.x) {
                movement.x -= comp - winSz.x;
                return;
            }
            if ((comp = nextY - radius) < 0) {
                movement.y -= comp;
                return;
            }
            if ((comp = nextY + radius) > winSz.y) {
                movement.y -= comp - winSz.y;
                return;
            }
        }

        void RotateAttract(sf::Vector2f fDir, float fCoeff) {
            float angleFromBase = Math::TrueAngleFromBase(fDir, shape.getRotation());
            float rotDeg;
            if (abs(angleFromBase) <= 90) {
                rotDeg = -angleFromBase;
            }
            else {
                int sign = angleFromBase < 0 ? -1 : 1;
                rotDeg = (180 - abs(angleFromBase)) * sign;
            }
            rotation += rotDeg*fCoeff;

        }
        void RotateRepel(sf::Vector2f fDir, float fCoeff) {
            float angleFromBase = Math::TrueAngleFromBase(fDir, shape.getRotation());
            rotation += angleFromBase * fCoeff;
        }
    private:

        bool CollidesWith(const Magnet& other) {
            sf::Vector2f pos1 = this->shape.getPosition() + this->movement;
            sf::Vector2f pos2 = other.shape.getPosition() + other.movement;
            float distance = Math::VecLen(pos1 - pos2);
            float limit = this->params.radius + other.params.radius;
            return distance < limit;
        }
};

class Simulation {
    public:
        Simulation() = delete;
        Simulation(sf::RenderWindow& win, const Params& params) : window(win),params(params) {
            Generate();
        }

        //ToDo: random generation -- aby se negenerovali do sebe //nesmi kolidovat ani s oknem
        void Generate() {
            magnets.clear();
            auto sz = window.getSize();
            srand(time(0));
            for (size_t i = 0; i < params.magCount; ++i) {
                size_t x = rand() % sz.x;
                size_t y = rand() % sz.y;
                size_t deg = rand() % 360;
                Magnet m(params.texture, sf::Vector3<size_t>(x, y, deg), params);
                magnets.push_back(m);

                /*/
                Magnet m1(params.texture, sf::Vector3f(600, 200, -30), params);
                magnets.push_back(m1);
                Magnet m2(params.texture, sf::Vector3f(700, 200, 180), params);
                magnets.push_back(m2);
                /**/

                //Magnet m3(params.texture, sf::Vector3f(650, 300, 40), params);
                //magnets.push_back(m3);
            }
        }

        void Step() {
            for (size_t i = 0; i < magnets.size(); ++i) {
                for (size_t j = i + 1; j < magnets.size(); ++j) {
                    ApplyMagForce(magnets[i], magnets[j]);
                }
                magnets[i].ApplyCoeffs();
            }            
            MoveMagnets();
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
            float fCoeff = GetForceCoeff(m1, m2);
            auto dpos1 = m2.shape.getPosition();
            auto dpos2 = m1.shape.getPosition();


            sf::Vector2f fDir = Math::NormalizeVec(m2.shape.getPosition() - m1.shape.getPosition());
            sf::Vector2f force = fDir * fCoeff;
            sf::Vector2f fDirNeg(-fDir.x, -fDir.y);

            m1.movement += force;
            m2.movement -= force;

            if (fCoeff > 0) {
                m1.RotateAttract(fDir, fCoeff);
                m2.RotateAttract(fDir, fCoeff);
            }
            else {
                m1.RotateRepel(fDirNeg, fCoeff);
                m2.RotateRepel(fDirNeg, fCoeff);
            }
        }

        //returns magnetic force coeff in interval(-1,1)
        float GetForceCoeff(const Magnet& m1, const Magnet& m2) {
            sf::Vector2f dir = m2.shape.getPosition() - m1.shape.getPosition();
            float dist = Math::VecLen(dir);

            float angleM1 = Math::DegToRad(Math::AngleFromBaseX(m1.shape.getRotation(), dir));
            float angleM2 = Math::DegToRad(Math::AngleFromBaseX(m2.shape.getRotation(), dir));

            float fMagnitude = cos(angleM1) * cos(angleM2) / (dist * dist);
            float fMax = 1 * 1 / (params.radius * params.radius * 4); //test--na 1 na 3,...
            
            //adding constant => behaves more like magnet
            fMagnitude < 0 ? fMagnitude -= params.fConst : fMagnitude += params.fConst;
            fMax < 0 ? fMax -= params.fConst : fMax += params.fConst;


            float fCoeff = fMagnitude / fMax;
            //assert(fCoeff >= -1 && fCoeff <= 1);
            return fCoeff;
        }

        void MoveMagnets() {
            for (size_t i = 0; i < magnets.size(); ++i)
            {
                //j == magnety se kterymi muze i kolidovat
                for (size_t j = 0; j < magnets.size(); j++) {
                    if (i == j) continue;
                    magnets[i].ResolveMagCollision(magnets[j]);
                }
                magnets[i].ResolveWinCollision(window.getSize());

                magnets[i].Move();
            }
        }
};

void ShowMenu(const sf::Window& win, Params& params, Simulation& sim) {
    //ImGui::ShowDemoWindow(); //vzor

    ImGui::Begin("Main Menu");
    ImGui::PushItemWidth(menuWidth / 2.5);
    ImGui::SetWindowSize(ImVec2(menuWidth, static_cast<float>(win.getSize().y)));
    ImGui::SetWindowPos(ImVec2(0, 0));
    if (params.isRunning) {
        if (ImGui::Button("STOP")) {
            params.isRunning = false;
        }
    }
    else {
        if (ImGui::Button("START")) {
            params.isRunning = true;
        }
    }
    if(ImGui::Button("RESET")) {
        sim.Generate();
    }

    ImGui::Text(""); //space
    ImGui::Text("Visual - requires reset");
    ImGui::SliderFloat("radius", &params.radius, 5, 100);
    ImGui::SliderInt("magnet count", &params.magCount, 1, 100);

    ImGui::Text(""); //space
    ImGui::Text("Movement");
    ImGui::SliderFloat("movCoef", &params.movCoef, 0.1f, fmin(params.radius * 1.99f, 1.0f/params.inertia)); //jaky nastavit min???
    ImGui::SliderFloat("rotationCoef", &params.rotationCoef, 0, 1);
    ImGui::SliderFloat("inertia", &params.inertia, 0, 1);
    ImGui::SliderFloat("force const", &params.fConstMult, 1, 200);
    params.fConst = 0.000001f * params.fConstMult;

    ImGui::End();
}

int main() {
    sf::VideoMode fullscreen = sf::VideoMode::getDesktopMode();
    float winScale = 0.5;
    
    sf::RenderWindow window(sf::VideoMode(static_cast<size_t>(fullscreen.width * winScale), static_cast<size_t>(fullscreen.height * winScale)), "Magnet Simulation", sf::Style::Close);
    window.setVerticalSyncEnabled(true);
    //window.setFramerateLimit(20); //debug
    ImGui::SFML::Init(window);

    Params params;
    params.SetDefault();

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
        ShowMenu(window, params, sim);

        if (params.isRunning) {
            sim.Step();
        }

        window.clear(sf::Color::White);
        sim.Draw();
        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::SFML::Shutdown();
    return 0;
}