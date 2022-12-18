#pragma once

#include "SimulatorComputeStage.h"
#include "SimulationParams.h"

struct BlockerPushConstants {
    u32 block_size;
};

class Blocker : protected SimulatorComputeStage {
public:
    Blocker(std::shared_ptr<VulkanContext> vulkan_context);

    vk::UniqueCommandBuffer computeBlocks(SimulationParams &simulation_params, VulkanBuffer &z_index_buffer,
                                          VulkanBuffer &uncompacted_block_buffer);
private:
    void createPipelineLayouts();
    virtual void createPipelines() final;
    virtual void createDescriptorPool() final;
    virtual void createDescriptorSets() final;

    void writeDescriptorSet(VulkanBuffer &z_index_buffer, VulkanBuffer &uncompacted_block_buffer);

    vk::DescriptorSet m_descriptor_set;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipeline_layout;
};




