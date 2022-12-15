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


class DensityCompute : public SimulatorComputeStage {
public:
	DensityCompute();


	void computeDensities(VulkanBuffer& position_buffer, u32 num_particles, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer);


private:

	void createPipelines() final;
	void createDescriptorSets() final;
	void createDescriptorPool() final;

	void writeBuffersDescriptorSet(VulkanBuffer& position_buffer, u32 num_particles, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer);

	DensityComputeDescriptorSets descriptor_sets;

};
