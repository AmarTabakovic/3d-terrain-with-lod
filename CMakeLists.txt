# Basic C++ and OpenGL CMake configuration
# Based on https://github.com/ArthurSonzogni/OpenGL_CMake_Skeleton
cmake_minimum_required(VERSION 3.5)

project(atlod)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_TARGET atlod)

add_executable(${APP_TARGET} src/camera.cpp src/shader.cpp src/main.cpp src/naiverenderer.cpp src/application.cpp src/heightmap.cpp)

add_definitions(-DGLEW_STATIC)
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)

target_link_libraries(${APP_TARGET}
  PRIVATE glfw
  PRIVATE libglew_static
  PRIVATE glm
)
#target_include_directories(atlod
#  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
#  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
#)
