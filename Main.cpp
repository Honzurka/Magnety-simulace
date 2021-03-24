#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <vector>
#include <cmath>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h> //M_PI

using V2 = sf::Vector2f;

namespace Math {
    const float pi = M_PI;
    int Random(int from, int to) {
        int r = rand();
        return r % (to-from) + from;
    }

    float DegToRad(float deg) {
        return deg / 180 * pi;
    }

    float RadToDeg(float rad) {
        return rad / pi * 180;
    }

    float DotProduct(const V2& v1, const V2& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }

    float VecLen(const V2& vec) {
        return sqrt(vec.x * vec.x + vec.y * vec.y);
    }

    float GetAngleRad(const V2& v1, const V2& v2) {
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

    float GetAngleDeg(const V2& v1, const V2& v2) {
        float angleRad = GetAngleRad(v1, v2);
        return RadToDeg(angleRad);
    }

    V2 Rotate(const V2& input, float angle) {
        float angleRad = DegToRad(angle);
        float x = cos(angleRad) * input.x - sin(angleRad) * input.y;
        float y = sin(angleRad) * input.x + cos(angleRad) * input.y;
        return V2(x, y);
    }

    //if zero vector is passed then its returned back
    V2 NormalizeVec(const V2& vec) {
        float vecLen = VecLen(vec);
        if (vecLen == 0) {
            return vec;
        }
        return V2(vec.x / vecLen, vec.y / vecLen);
    }

    V2 SetLen(const V2& vec, float len) {
        V2 res = NormalizeVec(vec);
        res *= len;
        return res;
    }

    //returns angle(DEG) between inVec and (1,0) rotated by deg
    float AngleFromBaseX(float deg, const V2& inVec) {
        V2 xBase(1, 0);
        V2 xDir = Math::Rotate(xBase, deg);
        return Math::GetAngleDeg(xDir, inVec);
    }

    //returns angle(DEG) between inVec and (0,-1) rotated by deg
    float AngleFromBaseY(float deg, const V2& inVec) {
        V2 yBase(0, -1);
        V2 yDir = Math::Rotate(yBase, deg);
        return Math::GetAngleDeg(yDir, inVec);
    }

    //returns angle(in DEG) from interval (-180,180) between inVec and vector(1,0)
    float TrueAngleFromBase(const V2& inVec, float deg) {
        float xAngle = AngleFromBaseX(deg, inVec);
        float yAngle = AngleFromBaseY(deg, inVec);
        auto getTrueAngle = [&xAngle, &yAngle]() {
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
        };
        return getTrueAngle();
    }
};

struct Params {
    sf::Texture texture;
    float radius = 20;
    int magCount = 6;

    float movCoef = 8.f; //max 2x*radius otherwise magnets jump over each other
    float rotationCoef = 0.5f; //interval (0,1)
    float inertia = 0.1f; //how much energy should be conserved from previous movement

    const float fConstBase = 0.000001f;
    float fConst = fConstBase; //artificially increases magnetic force coefficient, allows better magnet-like behavior
    float fConstMult = 10.f;
    float fCoefAttrLim = 0.8; //(0,1)

    bool isRunning = true;
    const int menuWidth = 200;

    void SetDefault() {
        if (!texture.loadFromFile("textures/RB.png")) {
            throw "Unable to load texture from file";
        }
    }
};

class Magnet {
    public:
        V2 movement;
        V2 lastMovement;
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
            movement = V2();
        }

        //applies inertia and movement/rotation coefficients
        void ApplyCoeffs() {
            movement = ((1 - params.inertia) * movement + params.inertia * lastMovement) * params.movCoef;
            rotation = ((1 - params.inertia) * rotation + params.inertia * lastRotation) * params.rotationCoef;
        }

        void ResolveMagCollision(Magnet& other) {
            if (!CollidesWith(other)) return;
            float dist = Math::VecLen(shape.getPosition() - other.shape.getPosition());
            float freeDist = dist - params.radius - other.params.radius;
            //...

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

        void ResolveWinCollision(sf::Vector2u winSz) {
            V2 pos = shape.getPosition();
            float radius = params.radius;
            float nextX = pos.x + movement.x;
            float nextY = pos.y + movement.y;
            #define SIDE_COLLISION(cond, movReduc, compMov, otherMov, compCoord) \
                if(cond){ \
                    float newMovCoord = compMov - movReduc; \
                    otherMov = otherMov * (newMovCoord-compCoord) / (compMov-compCoord); \
                    compMov = newMovCoord; \
                    nextX = pos.x + movement.x; \
                    nextY = pos.y + movement.y; \
                }
            SIDE_COLLISION(nextX - radius < params.menuWidth, (float)(nextX - radius - params.menuWidth), movement.x, movement.y, pos.x);
            SIDE_COLLISION(nextX + radius > winSz.x, (float)(nextX + radius - winSz.x), movement.x, movement.y, pos.x);
            SIDE_COLLISION(nextY - radius < 0, (float)(nextY - radius), movement.y, movement.x, pos.y);
            SIDE_COLLISION(nextY + radius > winSz.y, (float)(nextY + radius - winSz.y), movement.y, movement.x, pos.y);
            #undef SIDE_COLLISION
        }

        //alters magnets rotation based on input args
        void RotateAttract(V2 fDir, float fCoeff) {
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
        void RotateRepel(V2 fDir, float fCoeff) {
            float angleFromBase = Math::TrueAngleFromBase(fDir, shape.getRotation());
            rotation += angleFromBase * fCoeff;
        }

        bool CollidesWith(const Magnet& other) {
            V2 pos1 = this->shape.getPosition() + this->movement;
            V2 pos2 = other.shape.getPosition() + other.movement;
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
        //seed - for random generator
        void Generate(size_t seed = time(0)) {
            magnets.clear();
            srand(seed);
            while (magnets.size() != params.magCount) {
                auto [m, spawned] = SpawnMagnet();
                if (spawned) {
                    magnets.push_back(m);
                }
            }
        }

        void Step() {
            for (size_t i = 0; i < magnets.size(); ++i) {
                for (size_t j = i + 1; j < magnets.size(); ++j) {
                    AlterForces(magnets[i], magnets[j]);
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

        std::pair<float,float> GetRandomCoords(){
            int radiusAsInt = static_cast<int>(params.radius);
            auto sz = window.getSize();
            float x = static_cast<float>(Math::Random(params.menuWidth + radiusAsInt, sz.x - radiusAsInt));
            float y = static_cast<float>(Math::Random(radiusAsInt, sz.y - radiusAsInt));
            return std::make_pair(x, y);
        }

        //stepLim - How many times will be tried to place last magnet
        std::pair<Magnet,bool> SpawnMagnet(size_t stepLim = 100) {
            auto [x, y] = GetRandomCoords();
            size_t deg = rand() % 360;
            Magnet m(params.texture, sf::Vector3<size_t>(x, y, deg), params);
            
            size_t step = 0;
            bool collides = true;
            while (collides && step < stepLim) {
                collides = false;
                for (size_t i = 0; i < magnets.size(); ++i) {
                    if (m.CollidesWith(magnets[i])) {
                        auto [newX, newY] = GetRandomCoords();
                        m.shape.setPosition(V2(newX, newY));
                        collides = true;
                        step++;
                        break;
                    }
                }
            }

            if (step == stepLim) {
                params.magCount = magnets.size();
                return std::make_pair(m, false);
            }
            return std::make_pair(m, true);
        }

        //alters movement and rotation of both magnets
        void AlterForces(Magnet& m1, Magnet& m2) {
            float fCoeff = GetForceCoeff(m1, m2);
            V2 fDir = Math::NormalizeVec(m2.shape.getPosition() - m1.shape.getPosition());
            V2 force = fDir * fCoeff;

            m1.movement += force;
            m2.movement -= force;
            if (fCoeff > 0) {
                m1.RotateAttract(fDir, fCoeff);
                m2.RotateAttract(fDir, fCoeff);
            }
            else {
                V2 fDirNeg(-fDir.x, -fDir.y);
                m1.RotateRepel(fDirNeg, fCoeff);
                m2.RotateRepel(fDirNeg, fCoeff);
            }
        }

        //retval in (-1,1)
        float GetForceCoeff(const Magnet& m1, const Magnet& m2) {
            V2 dir = m2.shape.getPosition() - m1.shape.getPosition();
            float angleM1 = Math::DegToRad(Math::AngleFromBaseX(m1.shape.getRotation(), dir));
            float angleM2 = Math::DegToRad(Math::AngleFromBaseX(m2.shape.getRotation(), dir));
            float dist = Math::VecLen(dir);
            
            float fMagnitude = cos(angleM1) * cos(angleM2) / fmax(pow(2 * params.radius, 2), pow(dist, 2));
            float fMax = 1 / pow(2 * params.radius, 2);
            fMagnitude < 0 ? fMagnitude -= params.fConst : fMagnitude += params.fConst;
            fMax += params.fConst;

            float fCoeff = fMagnitude / fMax;
            if (fCoeff > params.fCoefAttrLim) return 0;
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

void ShowMenu(const sf::RenderWindow& win, Params& params, Simulation& sim) {
    auto menuWidth = params.menuWidth;
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
    params.movCoef = fmin(params.movCoef, 1.0f/params.inertia);
    ImGui::SliderFloat("force const", &params.fConstMult, 1, 200);
    params.fConst = params.fConstBase * params.fConstMult;
    ImGui::SliderFloat("attract lim", &params.fCoefAttrLim, 0, 1);
    ImGui::End();
}

int main() {
    sf::VideoMode fullscreen = sf::VideoMode::getDesktopMode();
    float winScale = 0.5;
    
    sf::RenderWindow window(sf::VideoMode(static_cast<size_t>(fullscreen.width * winScale),
                            static_cast<size_t>(fullscreen.height * winScale)),
                            "Magnet Simulation", sf::Style::Close);
    //window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);
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
