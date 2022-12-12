#pragma once

#include <map>
#include "VulkanInclude.h"
#include "utils.h"
#include "App.h"

class SimulatorComputeStage {
public:
    explicit SimulatorComputeStage(App* app) : m_app(app) {}
    ~SimulatorComputeStage();
    SimulatorComputeStage(const SimulatorComputeStage&) = delete;
    SimulatorComputeStage& operator=(const SimulatorComputeStage&) = delete;

protected:
    virtual void createPipelines() = 0;
    virtual void createDescriptorPool() = 0;
    void createDescriptorPool(u32 max_sets, std::map<vk::DescriptorType, u32> counts);
    virtual void createDescriptorSets() = 0;

    vk::CommandBuffer createCommandBuffer();

    App* m_app;
    vk::DescriptorPool m_descriptor_pool;
};




