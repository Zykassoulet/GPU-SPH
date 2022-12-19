#pragma once

#include "SimulatorComputeStage.h"
#include "SimulationParams.h"

class Compactor : protected SimulatorComputeStage {
public:
    Compactor(std::shared_ptr<VulkanContext> vulkan_context);

    vk::UniqueCommandBuffer compactBlocks(SimulationParams &simulation_params, VulkanBuffer &uncompacted_block_buffer,
                                          VulkanBuffer &compacted_block_buffer, VulkanBuffer &dispatch_indirect_buffer);
private:
    void createPipelineLayouts();
    void createPipelines() final;
    void createDescriptorPool() final;
    void createDescriptorSets() final;

    void writeDescriptorSet(VulkanBuffer &uncompacted_block_buffer, VulkanBuffer &compacted_block_buffer, VulkanBuffer &dispatch_indirect_buffer);

    vk::DescriptorSet m_descriptor_set;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipeline_layout;
};
