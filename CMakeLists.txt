cmake_minimum_required(VERSION 3.22)
project(project)

set(CMAKE_CXX_STANDARD 20)

add_executable(project main.cpp App.cpp "PhysicsEngine.h" "PhysicsEngine.cpp" "vkinit.h" "vkinit.cpp" "ComputePipeline.h" "ComputePipeline.cpp")

#find_package(Vulkan REQUIRED)
add_subdirectory(vendor)




