#pragma once

#include "utils.h"
#include "VulkanContext.h"
#include "particle.h"
#include "SimulatorComputeStage.h"

#include <vector>
#include "glm/glm.hpp"


struct PositionComputeDescriptorSets {
	vk::DescriptorSetLayout layout;
	vk::DescriptorSet set;
};

struct PositionComputePushConstants {
	u32 num_particles;
	float dt;
};


class PositionCompute : public SimulatorComputeStage {
public:
	PositionCompute(std::shared_ptr<VulkanContext> vulkan_context);


	vk::UniqueCommandBuffer computeVelocities(VulkanBuffer& position_buffer, VulkanBuffer& velocity_buffer, PositionComputePushConstants push_constants);


private:

	void createPipelineLayouts();
	void createPipelines() final;
	void createDescriptorSets() final;
	void createDescriptorPool() final;

	void writeBuffersDescriptorSet(VulkanBuffer& position_buffer, VulkanBuffer& velocity_buffer);

	PositionComputeDescriptorSets descriptor_sets;
	vk::Pipeline pipeline;
	vk::PipelineLayout pipeline_layout;
};
