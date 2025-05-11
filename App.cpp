#include <chrono>
#include <iostream>
#include <thread>
#include "App.h"


void App::mainLoop() {
    while(!glfwWindowShouldClose(m_vulkan_context->m_window)) {

        auto render_start = std::chrono::high_resolution_clock::now();
        while (std::chrono::high_resolution_clock::now() - render_start < std::chrono::milliseconds(1000 / 60)) {
            auto step_start = std::chrono::high_resolution_clock::now();
            glfwPollEvents();
            m_physics_engine->step();
            auto step_end = std::chrono::high_resolution_clock::now();
            // Don't run faster than real time
            auto step_duration = step_end - step_start;
            auto timestep = std::chrono::milliseconds((u32) (m_physics_engine->getSimulationParams().dt * 1000));
            if (step_duration < timestep) {
                std::this_thread::sleep_for(timestep - step_duration);
            }
            //std::cout << "Time per step [ms]: " << std::chrono::duration_cast<std::chrono::milliseconds>(step_duration).count() << std::endl;

        };

        //std::cout << "there" << std::endl;
        m_renderer->render(m_physics_engine->getSimulationParams(), m_physics_engine->getLastPositionBuffer(), m_physics_engine->getLastVelocityBuffer());
    }
}

App::App() {
    m_vulkan_context = std::make_shared<VulkanContext>();
    auto [particles, sim_params] = PhysicsEngine::createSimulationParams();
    m_physics_engine = std::make_shared<PhysicsEngine>(m_vulkan_context, sim_params, particles);
    m_renderer = std::make_shared<InstanceRenderer>(m_vulkan_context);
}