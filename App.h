#pragma once

#include <memory>
#include "VulkanContext.h"
#include "PhysicsEngine.h"

class App {
public:
    App();

    void mainLoop();

private:
    std::shared_ptr<VulkanContext> m_vulkan_context;
    std::shared_ptr<PhysicsEngine> m_physics_engine;
};




