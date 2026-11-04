cmake_minimum_required(VERSION 3.16)
project(ecolab_logo_opengl C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(ecolab_opengl
    src/main.c
    src/math3d.c
    src/petal.c
)

target_include_directories(ecolab_opengl PRIVATE src)

target_link_libraries(ecolab_opengl PRIVATE OpenGL::GL GLUT::GLUT)

if(WIN32)
    target_link_libraries(ecolab_opengl PRIVATE winmm)
endif()

if(UNIX)
    target_link_libraries(ecolab_opengl PRIVATE m)
endif()

if(MSVC)
    target_compile_options(ecolab_opengl PRIVATE /utf-8 /W3)
else()
    target_compile_options(ecolab_opengl PRIVATE -Wall -Wextra -Wpedantic)
endif()
