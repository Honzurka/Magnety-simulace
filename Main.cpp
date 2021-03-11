#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>


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

#include <iostream>
void SimulationLoop() {
    std::cout << "Simulation is running" << std::endl;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1080, 720), "MyTitle");
    window.setVerticalSyncEnabled(true);

    ImGui::SFML::Init(window);
    sf::Clock deltaClock;
    bool isRunning = false;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event); //??

            if (event.type == sf::Event::Closed) {
                window.close();
            }
            ImGui::SFML::Update(window, deltaClock.restart()); //??
            /*CALLING ImGui Funcs*/
            ImGui::ShowDemoWindow(); //check source for inspiration
            ImGui::Begin("test window");
            ImGui::Text("Hello world");
            if (ImGui::Button("Start",ImVec2(100,50))) {
                isRunning = !isRunning;
            }
            float f;
            ImGui::SliderFloat("slider", &f, 100, 200);

            ImGui::End();
            /*------------------*/

            if (isRunning) {    //not sure when should this calculation happen
                SimulationLoop();
                std::cout << "f value: " << f << std::endl;
            }


            window.clear(sf::Color::White);
            ImGui::SFML::Render(window);
            window.display();
        }
    }
    ImGui::SFML::Shutdown();
}