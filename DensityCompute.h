#pragma once

#include "utils.h"
#include "VulkanContext.h"
#include "particle.h"
#include "SimulatorComputeStage.h"

#include <vector>


struct DensityComputeDescriptorSets {
	vk::DescriptorSetLayout layout;
	vk::DescriptorSet set;
};

struct DensityComputePushConstants {
	u32 num_particles;
	float kernel_radius;
};


class DensityCompute : public SimulatorComputeStage {
public:
	DensityCompute(std::shared_ptr<VulkanContext> vulkan_context);


	vk::UniqueCommandBuffer computeDensities(VulkanBuffer& position_buffer, VulkanBuffer& zindex_buffer, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer, u32 num_particles, float kernel_radius);


private:

	void createPipelineLayouts();
	void createPipelines() final;
	void createDescriptorSets() final;
	void createDescriptorPool() final;

	void writeBuffersDescriptorSet(VulkanBuffer& position_buffer, VulkanBuffer& zindex_buffer, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer);

	DensityComputeDescriptorSets descriptor_sets;
	vk::Pipeline pipeline;
	vk::PipelineLayout pipeline_layout;
};
