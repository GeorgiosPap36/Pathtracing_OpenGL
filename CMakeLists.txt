cmake_minimum_required(VERSION 3.26)
project(OpenGL VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)

set(SOURCES
    src/main.cpp
    src/shader.cpp
    src/compute_shader.cpp
    src/camera.cpp
    src/model/model.cpp
    src/model/model_utils.cpp
    dependencies/glad.c
)

add_executable(Raytracing_OpenGL ${SOURCES})

target_include_directories(Raytracing_OpenGL PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
)

if (WIN32)
    set(GLFW_INCLUDE_DIR "C:/Cpp_libraries/glfw-3.4.bin.WIN64/include")
    set(GLFW_LIB "C:/Cpp_libraries/glfw-3.4.bin.WIN64/lib-mingw-w64/libglfw3.a")
    set(GLM_DIR "C:/Cpp_libraries/glm")

    target_include_directories(Raytracing_OpenGL PRIVATE ${GLFW_INCLUDE_DIR} ${GLM_DIR})
    target_link_libraries(Raytracing_OpenGL ${GLFW_LIB} OpenGL::GL)
    target_compile_definitions(Raytracing_OpenGL PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    find_package(glfw3 REQUIRED)
    find_package(glm CONFIG REQUIRED)

    target_link_libraries(Raytracing_OpenGL 
        glfw
        glm::glm
        OpenGL::GL
    )
endif()
