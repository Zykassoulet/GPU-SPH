#pragma once


#include "utils.h"
#include "VulkanInclude.h"
#include "VulkanHelpers.h"
#include "glm/glm.hpp"
#include <vector>


struct SimulationParameters {
	u32 number_particles;
	std::vector<glm::vec3> particles;
};

struct BlockData {
	u32 number_particles;
	u32 first_particles_offset;
};


struct PhysicsEngineBuffers {
	VulkanBuffer input_pos_buf;
	VulkanBuffer pos_buf;
	VulkanBuffer zindex_buf;
	VulkanBuffer vel_buf;
	VulkanBuffer dens_buf;
	VulkanBuffer block_buf;
};

struct PhysicsEngineDescriptorSetLayouts {
	vk::DescriptorSetLayout blocks_data_layout;
	vk::DescriptorSetLayout density_layout;
	vk::DescriptorSetLayout forces_layout;
};

struct PhysicsEngineDescriptorSets {

};

struct PhysicsEnginePipelines {

};


