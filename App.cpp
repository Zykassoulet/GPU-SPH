#include "App.h"


void App::mainLoop() {
    while(!glfwWindowShouldClose(m_vulkan_context->m_window)) {
        glfwPollEvents();
        m_physics_engine->step();
    }
}

App::App() {
    m_vulkan_context = std::make_shared<VulkanContext>();
    m_physics_engine = std::make_shared<PhysicsEngine>(m_vulkan_context);
}