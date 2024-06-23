cmake_minimum_required(VERSION 3.16.3)

project(atari2600)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(IMGUI_DIR imgui)
add_library(imgui STATIC ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp)
target_include_directories(imgui PRIVATE ${IMGUI_DIR})

add_library(imgui_glut STATIC ${IMGUI_DIR}/backends/imgui_impl_glut.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp)
target_include_directories(imgui_glut PRIVATE ${IMGUI_DIR})

add_library(atari2600 STATIC atari2600.cpp mos6502.cpp tia.cpp util.cpp)

add_executable(imgui_main imgui_main.cpp)
target_link_libraries(imgui_main atari2600 imgui imgui_glut GL GLU glut)
target_include_directories(imgui_main PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends/)

enable_testing()
find_package(GTest REQUIRED)

add_executable(atari2600_test atari2600_test.cpp)
target_link_libraries(atari2600_test atari2600)
target_link_libraries(atari2600_test GTest::gtest gtest GTest::gtest_main)


include(GoogleTest)
gtest_discover_tests(atari2600_test DISCOVERY_MODE PRE_TEST)