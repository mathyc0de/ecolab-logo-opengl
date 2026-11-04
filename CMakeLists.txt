cmake_minimum_required(VERSION 3.16)
project(ecolab_logo_opengl C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(ecolab_exemplo
    examples/main.c
    examples/math3d.c
    examples/petal.c
)

target_include_directories(ecolab_exemplo PRIVATE examples)

target_link_libraries(ecolab_exemplo PRIVATE OpenGL::GL GLUT::GLUT)

if(WIN32)
    target_link_libraries(ecolab_exemplo PRIVATE winmm)
endif()

if(UNIX)
    target_link_libraries(ecolab_exemplo PRIVATE m)
endif()

if(MSVC)
    target_compile_options(ecolab_exemplo PRIVATE /utf-8 /W3)
else()
    target_compile_options(ecolab_exemplo PRIVATE -Wall -Wextra -Wpedantic)
endif()
