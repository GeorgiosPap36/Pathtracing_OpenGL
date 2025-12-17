cmake_minimum_required(VERSION 3.26)
project(OpenGL VERSION 4.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(SOURCES
    src/main.cpp
    src/shader.cpp
    src/compute_shader.cpp
    src/camera.cpp
    src/scene.cpp
    src/model/model.cpp
    src/model/model_utils.cpp
    src/model/bvh_utils.cpp
    dependencies/glad.c)

add_executable(Raytracing_OpenGL ${SOURCES})

target_include_directories(Raytracing_OpenGL PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

target_link_libraries(Raytracing_OpenGL
    glfw
    glm::glm
    OpenGL::GL)

target_compile_definitions(Raytracing_OpenGL PRIVATE _CRT_SECURE_NO_WARNINGS)