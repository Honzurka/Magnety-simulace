

cmake_minimum_required(VERSION 3.1)

project(magnety
  LANGUAGES CXX
  VERSION 0.1
)

find_library(SS sfml-system)
find_library(SG sfml-graphics)
find_library(SW sfml-window)

include_directories(imgui sfml-imgui)

add_executable(magnety
	Main.cpp
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	sfml-imgui/imgui-SFML.cpp
	)

target_link_libraries(magnety
	sfml-system sfml-graphics sfml-window GL)

