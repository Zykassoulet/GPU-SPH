#pragma once

#include <map>
#include "VulkanInclude.h"
#include "utils.h"
#include "VulkanContext.h"

class SimulatorComputeStage {
public:
    explicit SimulatorComputeStage(std::shared_ptr<VulkanContext> app) : m_app(std::move(app)) {}
    ~SimulatorComputeStage();
    SimulatorComputeStage(const SimulatorComputeStage&) = delete;
    SimulatorComputeStage& operator=(const SimulatorComputeStage&) = delete;

protected:
    virtual void createPipelines() = 0;
    virtual void createDescriptorPool() = 0;
    void createDescriptorPool(u32 max_sets, std::map<vk::DescriptorType, u32> counts);
    virtual void createDescriptorSets() = 0;

    vk::CommandBuffer createCommandBuffer();

    std::shared_ptr<VulkanContext> m_app;
    vk::DescriptorPool m_descriptor_pool;
};




