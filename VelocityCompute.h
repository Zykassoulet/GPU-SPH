#pragma once

#include "utils.h"
#include "VulkanContext.h"
#include "particle.h"
#include "SimulatorComputeStage.h"

#include <vector>
#include "glm/glm.hpp"


struct VelocityComputeDescriptorSets {
	vk::DescriptorSetLayout layout;
	vk::DescriptorSet set;
};

struct VelocityComputePushConstants {
	u32 num_particles;
	float kernel_radius;
	glm::ivec3 blocks_count;
	float particle_mass;
	float dt;
};


class VelocityCompute : public SimulatorComputeStage {
public:
	VelocityCompute(std::shared_ptr<VulkanContext> vulkan_context);


	vk::UniqueCommandBuffer computeVelocities(VulkanBuffer& position_buffer, VulkanBuffer& zindex_buffer, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer, VulkanBuffer& velocity_buffer, VelocityComputePushConstants push_constants);


private:

	void createPipelineLayouts();
	void createPipelines() final;
	void createDescriptorSets() final;
	void createDescriptorPool() final;

	void writeBuffersDescriptorSet(VulkanBuffer& position_buffer, VulkanBuffer& zindex_buffer, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer, VulkanBuffer& velocity_buffer);

	VelocityComputeDescriptorSets descriptor_sets;
	vk::Pipeline pipeline;
	vk::PipelineLayout pipeline_layout;
};
