#include <imgui.h> // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h
#include <imgui-SFML.h> // for ImGui::SFML::* functions and SFML-specific overloads
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <vector>
#include <iostream> //debug
#include <cmath>

class Math {
    public:
    static const float pi;
    static int Random(int from, int to) {
        int r = rand();
        return r % (to-from) + from;
    }
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

const int menuWidth = 200;
struct Params {
    sf::Texture texture;
    float radius = 20;
    int magCount = 3;

    float movCoef = 8.f; //max 2x*radius otherwise magnets will jump over each other
    float rotationCoef = 0.5f; //interval (0,1)
    float inertia = 0.1f; //how much energy should be conserved from previous movement

    const float fConstBase = 0;// 0.000001f;
    float fConst = fConstBase; //artificially increases magnetic force coefficient, allows better magnet-like behavior
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
        Magnet(const sf::Texture& texture, const sf::Vector3<size_t>& loc, const Params& params):params(params){
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

        //applies inertia and movement/rotation coefficients
        void ApplyCoeffs() {
            movement = ((1 - params.inertia) * movement + params.inertia * lastMovement) * params.movCoef;
            rotation = ((1 - params.inertia) * rotation + params.inertia * lastRotation) * params.rotationCoef;
        }

        //In case of collision sticks magnets together
        //mostly disables magnets ability to move after joining other magnet => non-magnetic behavior
        void ResolveMagCollision(Magnet& other) {
            if (!CollidesWith(other)) return;
            float dist = Math::VecLen(shape.getPosition() - other.shape.getPosition());
            float freeDist = dist - params.radius - other.params.radius;

            float thisLen = Math::VecLen(movement);
            float otherLen = Math::VecLen(other.movement);
            if (thisLen == 0) {
                other.movement = Math::SetLen(other.movement, freeDist);
                return;
            }
            if (otherLen == 0) {
                movement = Math::SetLen(movement, freeDist);
                return;
            }

            float thisNewLen = freeDist / (thisLen + otherLen) * thisLen;
            movement = Math::SetLen(movement, thisNewLen);
            
            float otherNewLen = freeDist - thisNewLen; //might lead to little rounding inaccuracy
            other.movement = Math::SetLen(other.movement, otherNewLen);
        }

        //In case of collision sticks magnet to window
        //simplified, doesnt exactly follow movement vector
        void ResolveWinCollision(sf::Vector2u winSz) {
            sf::Vector2f pos = shape.getPosition();
            float radius = params.radius;
            float nextX = pos.x + movement.x;
            float nextY = pos.y + movement.y;
            float comp;
            if ((comp = nextX - radius) < menuWidth) { //menu on left side
                movement.x -= comp - menuWidth;
            }
            if ((comp = nextX + radius) > winSz.x) {
                movement.x -= comp - winSz.x;
            }
            if ((comp = nextY - radius) < 0) {
                movement.y -= comp;
            }
            if ((comp = nextY + radius) > winSz.y) {
                movement.y -= comp - winSz.y;
            }
        }

        //alters magnets rotation based on input args
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

        //alters magnets rotation based on input args
        void RotateRepel(sf::Vector2f fDir, float fCoeff) {
            float angleFromBase = Math::TrueAngleFromBase(fDir, shape.getRotation());
            rotation += angleFromBase * fCoeff;
        }

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
        Simulation(sf::RenderWindow& win, Params& params) : window(win),params(params) {
            Generate();
        }


        //Randomly populates collection of magnets
        //stepLim - How many times will be tried to place last magnet
        //seed - for random generator
        void Generate(size_t stepLim = 100, size_t seed = time(0)) {
            magnets.clear();
            auto sz = window.getSize();
            srand(seed);
            int radiusAsInt = static_cast<int>(params.radius);
            while (magnets.size() != params.magCount) {
                size_t x = Math::Random(menuWidth + radiusAsInt, sz.x - radiusAsInt);
                size_t y = Math::Random(radiusAsInt, sz.y - radiusAsInt);
                size_t deg = rand() % 360;
                Magnet m(params.texture, sf::Vector3<size_t>(x, y, deg), params);
                size_t step = 0;
                bool collides = true;
                while (collides && step < stepLim) {
                    collides = false;
                    for (size_t i = 0; i < magnets.size(); ++i) {
                        if (m.CollidesWith(magnets[i])) {
                            float newX = static_cast<float>(Math::Random(menuWidth + radiusAsInt, sz.x - radiusAsInt));
                            float newY = static_cast<float>(Math::Random(radiusAsInt, sz.y - radiusAsInt));
                            m.shape.setPosition(sf::Vector2f(newX, newY));
                            collides = true;
                            step++;
                            break;
                        }
                    }
                }
                if (step == stepLim) {
                    params.magCount = magnets.size();
                    break;
                }
                else {
                    magnets.push_back(m);
                }
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
        Params& params;

        //alters movement and rotation of both magnets
        void ApplyMagForce(Magnet& m1, Magnet& m2) {
            float fCoeff = GetForceCoeff(m1, m2);
            std::cout << fCoeff << std::endl;
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

        //retval in (-1,1)
        //MOSTLY - might return other values as well(if magnets are inside each other == due to bad collision handling)
        float GetForceCoeff(const Magnet& m1, const Magnet& m2) {
            sf::Vector2f dir = m2.shape.getPosition() - m1.shape.getPosition();
            float dist = Math::VecLen(dir);
            float angleM1 = Math::DegToRad(Math::AngleFromBaseX(m1.shape.getRotation(), dir));
            float angleM2 = Math::DegToRad(Math::AngleFromBaseX(m2.shape.getRotation(), dir));

            float fMagnitude = cos(angleM1) * cos(angleM2) / (dist * dist);
            float fMax = 1 / (params.radius * params.radius * 4);
            
            fMagnitude < 0 ? fMagnitude -= params.fConst : fMagnitude += params.fConst;
            fMax < 0 ? fMax -= params.fConst : fMax += params.fConst;

            float fCoeff = fMagnitude / fMax;
            return fCoeff;
        }

        void MoveMagnets() {
            for (size_t i = 0; i < magnets.size(); ++i)
            {
                //magnets[j] - magnets which could collide with magnets[i] - could be optimized(QuadTree)
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
    //ImGui::ShowDemoWindow(); //source of inspiration
    ImGui::Begin("Main Menu");
    ImGui::SetWindowSize(ImVec2(menuWidth, static_cast<float>(win.getSize().y)));
    ImGui::SetWindowPos(ImVec2(0, 0));
    if (params.isRunning) {
        if (ImGui::Button("STOP", ImVec2(menuWidth*0.9, 20))) {
            params.isRunning = false;
        }
    }
    else {
        if (ImGui::Button("START", ImVec2(menuWidth*0.9,20))) {
            params.isRunning = true;
        }
    }
    if(ImGui::Button("RESET", ImVec2(menuWidth*0.9, 20))) {
        sim.Generate();
    }

    ImGui::PushItemWidth(menuWidth / 2.5);
    ImGui::Text("");
    ImGui::Text("Visual - requires reset");
    ImGui::SliderFloat("radius", &params.radius, 5, 100);
    ImGui::SliderInt("magnet count", &params.magCount, 1, 100);

    ImGui::Text("");
    ImGui::Text("Movement");
    ImGui::SliderFloat("movCoef", &params.movCoef, 0.1f, fmin(params.radius * 1.99f, 1.0f/params.inertia));
    ImGui::SliderFloat("rotationCoef", &params.rotationCoef, 0, 1);
    ImGui::SliderFloat("inertia", &params.inertia, 0, 1);
    ImGui::SliderFloat("force const", &params.fConstMult, 1, 200);
    params.fConst = params.fConstBase * params.fConstMult;

    ImGui::End();
}

int main() {
    sf::VideoMode fullscreen = sf::VideoMode::getDesktopMode();
    float winScale = 0.5;
    
    sf::RenderWindow window(sf::VideoMode(static_cast<size_t>(fullscreen.width * winScale),
                            static_cast<size_t>(fullscreen.height * winScale)),
                            "Magnet Simulation", sf::Style::Close);
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
