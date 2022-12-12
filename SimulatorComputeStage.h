#pragma once

#include <map>
#include "VulkanInclude.h"
#include "utils.h"
#include "VulkanContext.h"
#include <functional>

class SimulatorComputeStage {
public:
    explicit SimulatorComputeStage(std::shared_ptr<VulkanContext> vulkan_context) : m_vk_context(std::move(vulkan_context)) {}
    ~SimulatorComputeStage();
    SimulatorComputeStage(const SimulatorComputeStage&) = delete;
    SimulatorComputeStage& operator=(const SimulatorComputeStage&) = delete;

protected:
    virtual void createPipelines() = 0;
    virtual void createDescriptorPool() = 0;
    void createDescriptorPool(u32 max_sets, std::map<vk::DescriptorType, u32> counts);
    virtual void createDescriptorSets() = 0;

    void deferDelete(std::function<void(std::shared_ptr<VulkanContext>)> deleter);

    vk::CommandBuffer createCommandBuffer();

    std::shared_ptr<VulkanContext> m_vk_context;
    vk::DescriptorPool m_descriptor_pool;
    std::vector<std::function<void(std::shared_ptr<VulkanContext>)>> m_deletion_queue;
};




