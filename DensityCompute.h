#pragma once

#include "utils.h"
#include "VulkanContext.h"
#include "particle.h"
#include "SimulatorComputeStage.h"

#include <vector>
#include "glm/glm.hpp"
#include "SimulationParams.h"


struct DensityComputeDescriptorSets {
	vk::DescriptorSetLayout particle_buffers_layout;
	vk::DescriptorSet particle_buffers;
    vk::DescriptorSetLayout block_buffers_layout;
    vk::DescriptorSet block_buffers;

    std::array<vk::DescriptorSetLayout, 2> getLayouts() {
        return { particle_buffers_layout, block_buffers_layout };
    }

    std::array<vk::DescriptorSet, 2> getSets() {
        return { particle_buffers, block_buffers };
    }
};

struct DensityComputePushConstants {
    glm::ivec4 simulation_domain; // In Z-index space (real space/grid spacing)
    u32 block_size; // In Z-index space (real space/grid spacing)
    f32 kernel_radius;
    f32 particle_mass;
    f32 stiffness;
    f32 rest_density;
    f32 gamma;
};


class DensityCompute : public SimulatorComputeStage {
public:
	DensityCompute(std::shared_ptr<VulkanContext> vulkan_context);


	vk::UniqueCommandBuffer computeDensities(const SimulationParams &simulation_params, VulkanBuffer &z_index_buffer,
                                             VulkanBuffer &particle_index_buffer, VulkanBuffer &position_buffer,
                                             VulkanBuffer &density_buffer, VulkanBuffer &pressure_buffer,
                                             VulkanBuffer &compacted_blocks_buffer,
                                             VulkanBuffer &uncompacted_blocks_buffer,
                                             VulkanBuffer &dispatch_params_buffer);


private:

	void createPipelineLayouts();
	void createPipelines() final;
	void createDescriptorSets() final;
	void createDescriptorPool() final;

	void writeBuffersDescriptorSet(VulkanBuffer &z_index_buffer,
                                   VulkanBuffer &particle_index_buffer, VulkanBuffer &position_buffer,
                                   VulkanBuffer &density_buffer, VulkanBuffer &pressure_buffer,
                                   VulkanBuffer &compacted_blocks_buffer, VulkanBuffer &uncompacted_blocks_buffer);

	DensityComputeDescriptorSets m_descriptor_sets;
	vk::Pipeline m_pipeline;
	vk::PipelineLayout m_pipeline_layout;
};
