#pragma once

#include "GPURadixSorter.h"
#include "DensityCompute.h"
#include <array>
#include "VulkanContext.h"
#include "utils.h"
#include "VulkanInclude.h"
#include "VulkanHelpers.h"
#include "glm/glm.hpp"
#include <vector>
#include "SimulationParams.h"
#include "ZIndexer.h"
#include "Blocker.h"
#include "Compactor.h"
#include "DensityCompute.h"
#include "ForceCompute.h"

struct BlockData {
	u32 number_particles;
	u32 first_particles_offset;
};

struct PhysicsEngineBuffers {
	VulkanBuffer input_position;
	VulkanBuffer position[2];
    VulkanBuffer velocity[2];
    VulkanBuffer density;
    VulkanBuffer pressure;
    VulkanBuffer z_index;
    VulkanBuffer particle_index;
    VulkanBuffer sort_key_ping_pong;
    VulkanBuffer sort_val_ping_pong;
	VulkanBuffer uncompacted_block;
    VulkanBuffer compacted_block;
    VulkanBuffer dispatch_indirect;
};

class PhysicsEngine {

public:
	explicit PhysicsEngine(std::shared_ptr<VulkanContext> vulkan_context, SimulationParams sim_params, std::vector<glm::vec4> particles);
    static std::pair<std::vector<glm::vec4>, SimulationParams> createSimulationParams();

    void step();

    inline SimulationParams getSimulationParams() { return m_sim_params; }
    inline VulkanBuffer& getPositionBuffer() { return m_buffers.position[m_ping_pong_idx]; }

private:
    void initBuffers(std::vector<glm::vec4>& particle_positions);

	SimulationParams m_sim_params;
	PhysicsEngineBuffers m_buffers;
    u32 m_ping_pong_idx = 0;

    ZIndexer m_z_indexer;
	GPURadixSorter m_radix_sorter;
    Blocker m_blocker;
    Compactor m_compactor;
    DensityCompute m_density_compute;
    ForceCompute m_force_compute;
	std::shared_ptr<VulkanContext> m_vk_context;

};