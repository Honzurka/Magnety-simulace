#include "imgui.h" // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h
#include "imgui-SFML.h" // for ImGui::SFML::* functions and SFML-specific overloads

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
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





struct Params { //under simulation???
    
};

class Magnet { //under simulation? //==Sprite ??
    //(static)radius - potencialne ruzne velke magnety
    //rotace - ze spritu
};

class Simulation {
    public:
        Params params;
        Simulation() {
            //vytvorit magnety
        }

        void Step() {
            //pohnout s magnety-1/2n^2
            //vyresit kolize
        }

        void Draw() {
            //sf::CircleShape shape(100.f);
            //shape.setFillColor(sf::Color::Green);

        }
    private:
        std::vector<Magnet> magnets;
        //textura
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
    
    sf::Clock deltaClock;
    Simulation sim;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        ShowMenu(window, sim.params);
        sim.Step();

        window.clear();
        sim.Draw();
        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::SFML::Shutdown();
    return 0;
}