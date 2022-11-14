cmake_minimum_required(VERSION 3.22)
project(project)

set(CMAKE_CXX_STANDARD 20)

add_executable(project main.cpp App.cpp)

find_package(Vulkan REQUIRED)
add_subdirectory(vendor/glfw)

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} vendor/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)

