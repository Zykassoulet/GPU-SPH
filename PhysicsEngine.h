#pragma once

#include "GPURadixSorter.h"
#include <array>
#include "VulkanContext.h"
#include "utils.h"
#include "VulkanInclude.h"
#include "VulkanHelpers.h"
#include "glm/glm.hpp"
#include <vector>






struct SimulationParameters {
	u32 number_particles;
	std::vector<glm::vec3> particles;
	float kernel_radius;
	float particle_radius;
	float particle_mass;
	float rest_density;
	glm::vec3 box_size;
	glm::vec3 initial_liquid_domain;
	float blocks_size;
	glm::ivec3 blocks_count;
	float dt;
	float k;
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


class PhysicsEngine {

public:
	explicit PhysicsEngine(std::shared_ptr<VulkanContext> vulkan_context);
    ~PhysicsEngine();

    void step();

private:
    void initSimulationParameters();
    void initBuffers();

    void initInputPosBuffer();
    void initPosBuffer();
    void initZIndexBuffer();
    void initVelBuffer();
    void initDensBuffer();
    void initBlocksDataBuffer();

	SimulationParameters sim_params;
	PhysicsEngineBuffers buffers;

	GPURadixSorter m_radix_sorter;
	std::shared_ptr<VulkanContext> m_vk_context;

	

};