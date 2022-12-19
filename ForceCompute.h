#pragma once

#include "utils.h"
#include "VulkanContext.h"
#include "particle.h"
#include "SimulatorComputeStage.h"

#include <vector>
#include "glm/glm.hpp"
#include "SimulationParams.h"


struct VelocityComputeDescriptorSets {
	vk::DescriptorSetLayout particle_buffers_layout;
	vk::DescriptorSet particle_buffers;
    vk::DescriptorSetLayout block_buffers_layout;
    vk::DescriptorSet block_buffers;
    vk::DescriptorSetLayout output_buffers_layout;
    vk::DescriptorSet output_buffers;

    std::array<vk::DescriptorSetLayout, 3> getLayouts() {
        return {particle_buffers_layout, block_buffers_layout, output_buffers_layout};
    }

    std::array<vk::DescriptorSet, 3> getSets() {
        return {particle_buffers, block_buffers, output_buffers};
    }
};

struct ForceComputePushConstants {
    glm::ivec4 simulation_domain;
    u32 block_size;
    f32 kernel_radius;
    f32 particle_mass;
    f32 dt;
};


class ForceCompute : public SimulatorComputeStage {
public:
	ForceCompute(std::shared_ptr<VulkanContext> vulkan_context);


	vk::UniqueCommandBuffer computeVelocities(SimulationParams simulation_params, VulkanBuffer &z_index_buffer,
                                              VulkanBuffer &particle_index_buffer, VulkanBuffer &position_buffer,
                                              VulkanBuffer &density_buffer, VulkanBuffer &velocity_buffer,
                                              VulkanBuffer &pressure_buffer, VulkanBuffer &uncompacted_blocks_buffer,
                                              VulkanBuffer &compacted_blocks_buffer, VulkanBuffer &out_position_buffer,
                                              VulkanBuffer &out_velocity_buffer,
                                              VulkanBuffer &dispatch_indirect_buffer);


private:

	void createPipelineLayouts();
	void createPipelines() final;
	void createDescriptorSets() final;
	void createDescriptorPool() final;

	void writeBufferDescriptorSets(VulkanBuffer &z_index_buffer,
                                   VulkanBuffer &particle_index_buffer, VulkanBuffer &position_buffer,
                                   VulkanBuffer &density_buffer, VulkanBuffer &velocity_buffer,
                                   VulkanBuffer &pressure_buffer, VulkanBuffer &uncompacted_blocks_buffer,
                                   VulkanBuffer &compacted_blocks_buffer, VulkanBuffer &out_position_buffer,
                                   VulkanBuffer &out_velocity_buffer);

	VelocityComputeDescriptorSets m_descriptor_sets;
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_pipeline_layout;
};
