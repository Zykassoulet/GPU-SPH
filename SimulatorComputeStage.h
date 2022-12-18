#pragma once

#include <map>
#include "VulkanInclude.h"
#include "utils.h"
#include "VulkanContext.h"
#include "VulkanCleaner.h"
#include <functional>
#include <utility>

class SimulatorComputeStage : protected VulkanCleaner {
public:
    explicit SimulatorComputeStage(std::shared_ptr<VulkanContext> vulkan_context) : VulkanCleaner(std::move(vulkan_context)) {}
    SimulatorComputeStage(const SimulatorComputeStage&) = delete;
    SimulatorComputeStage& operator=(const SimulatorComputeStage&) = delete;

protected:
    virtual void createPipelines() = 0;
    virtual void createDescriptorPool() = 0;
    void createDescriptorPool(u32 max_sets, std::map<vk::DescriptorType, u32> counts);
    virtual void createDescriptorSets() = 0;
    0
    vk::UniqueCommandBuffer createCommandBuffer();

    vk::DescriptorPool m_descriptor_pool;
};




