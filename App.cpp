#include <chrono>
#include <iostream>
#include "App.h"


void App::mainLoop() {
    while(!glfwWindowShouldClose(m_vulkan_context->m_window)) {
        auto step_start = std::chrono::high_resolution_clock::now();
        glfwPollEvents();
        m_physics_engine->step();
        auto step_end = std::chrono::high_resolution_clock::now();
        std::cout << "Time per step [ms]: " << std::chrono::duration_cast<std::chrono::milliseconds>(step_end - step_start).count() << std::endl;
    }
}

App::App() {
    m_vulkan_context = std::make_shared<VulkanContext>();
    auto [particles, sim_params] = PhysicsEngine::createSimulationParams();
    m_physics_engine = std::make_shared<PhysicsEngine>(m_vulkan_context, sim_params, particles);
}