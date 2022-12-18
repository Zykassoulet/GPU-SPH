#include <random>
#include "PhysicsEngine.h"
#include <math.h>
#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>

PhysicsEngine::PhysicsEngine(std::shared_ptr<VulkanContext> vulkan_context, SimulationParams sim_params, std::vector<glm::vec4> particles) :
        m_z_indexer(vulkan_context, sim_params.grid_size.x, sim_params.grid_size.y, sim_params.grid_size.z, sim_params.grid_unit),
        m_radix_sorter(vulkan_context, sim_params.num_particles * 2),
        m_blocker(vulkan_context),
        m_compactor(vulkan_context),
        m_density_compute(vulkan_context),
        m_force_compute(vulkan_context),
        m_vk_context(vulkan_context),
        m_sim_params(sim_params),
        m_ping_pong_idx(0) {
	initBuffers(particles);
}

std::pair<std::vector<glm::vec4>, SimulationParams> PhysicsEngine::createSimulationParams() {
    SimulationParams sim_params;
    auto real_region_size = glm::vec3(1.f, 1.f, 1.f);
    auto initial_liquid_region = glm::vec3(1.f, 0.5f, 1.f);
    sim_params.rest_density = 1000.f;
    sim_params.stiffness = sim_params.rest_density;
    float initial_spacing = 0.05f;
    sim_params.kernel_radius = 10.f * initial_spacing;
    sim_params.particle_mass = sim_params.rest_density * pow(initial_spacing, 3);

    std::array<float, 3> k_min{ 0.f, 0.f, 0.f };
    std::array<float, 3> k_max = {initial_liquid_region.x, initial_liquid_region.y, initial_liquid_region.z };
    std::vector<std::array<float, 3>> poisson_sampling = thinks::PoissonDiskSampling(initial_spacing, k_min, k_max);
    sim_params.num_particles = poisson_sampling.size();

    std::vector<glm::vec4> particles;
    particles.resize(sim_params.num_particles);
    std::transform(poisson_sampling.begin(), poisson_sampling.end(), particles.begin(),
                   [](std::array<float, 3> coords) {return glm::vec4(coords[0], coords[1], coords[2], 0.0); }
    );

    float particle_volume = initial_liquid_region.x * initial_liquid_region.y * initial_liquid_region.z;
    float particles_per_unit_volume = sim_params.num_particles / particle_volume;
    u32 grid_units_per_real_unit = std::ceil(std::cbrt(particles_per_unit_volume / 256)) * 1.5;
    sim_params.grid_unit = 1.0f / grid_units_per_real_unit;
    sim_params.grid_size = glm::vec4(
        std::ceil(real_region_size.x * grid_units_per_real_unit),
        std::ceil(real_region_size.y * grid_units_per_real_unit),
        std::ceil(real_region_size.z * grid_units_per_real_unit),
        0
    );

    sim_params.block_size = (1 << (u32) std::ceil(std::log2((sim_params.kernel_radius / sim_params.grid_unit))));
    sim_params.num_blocks = (sim_params.grid_size.x / sim_params.block_size + 1)
                            * (sim_params.grid_size.y / sim_params.block_size + 1)
                            * (sim_params.grid_size.z / sim_params.block_size + 1);

    sim_params.gas_gamma = 1.0;
    sim_params.dt = 0.01; //TO TWEAK

    return {particles, sim_params};
}

void PhysicsEngine::initBuffers(std::vector<glm::vec4>& particle_positions) {
    // Create buffers
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer;
    VmaAllocationCreateFlags memory_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    m_buffers.input_position = m_vk_context->createCPUAccessibleBuffer(usage, sizeof(glm::vec4), m_sim_params.num_particles);

    usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
    m_buffers.position[0] = m_vk_context->createBuffer(usage, sizeof(glm::vec4), m_sim_params.num_particles);
    m_buffers.position[1] = m_vk_context->createBuffer(usage, sizeof(glm::vec4), m_sim_params.num_particles);

    m_buffers.velocity[0] = m_vk_context->createBuffer(usage, sizeof(glm::vec4), m_sim_params.num_particles);
    m_buffers.velocity[1] = m_vk_context->createBuffer(usage, sizeof(glm::vec4), m_sim_params.num_particles);

    m_buffers.density = m_vk_context->createBuffer(usage, sizeof(f32), m_sim_params.num_particles);
    m_buffers.pressure = m_vk_context->createBuffer(usage, sizeof(f32), m_sim_params.num_particles);

    m_buffers.z_index = m_vk_context->createBuffer(usage, sizeof(u32), m_sim_params.num_particles);
    m_buffers.particle_index = m_vk_context->createBuffer(usage, sizeof(u32), m_sim_params.num_particles);
    m_buffers.sort_key_ping_pong = m_vk_context->createBuffer(usage, sizeof(u32), m_sim_params.num_particles);
    m_buffers.sort_val_ping_pong = m_vk_context->createBuffer(usage, sizeof(u32), m_sim_params.num_particles);

    m_buffers.uncompacted_block = m_vk_context->createBuffer(usage, sizeof(BlockData), m_sim_params.num_blocks);
    m_buffers.compacted_block = m_vk_context->createBuffer(usage, sizeof(BlockData), m_sim_params.num_blocks);

    usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer;
    m_buffers.dispatch_indirect = m_vk_context->createBuffer(usage, sizeof(vk::DispatchIndirectCommand), 1);

    // Initialize buffers
    m_buffers.input_position.store_data(particle_positions.data(), particle_positions.size());

    vk::CommandBufferAllocateInfo cmd_buf_alloc_info(m_vk_context->m_compute_command_pool, vk::CommandBufferLevel::ePrimary, 1);
    auto cmd_buf = std::move(m_vk_context->m_device.allocateCommandBuffersUnique(cmd_buf_alloc_info).front());
    cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    cmd_buf->copyBuffer(
        m_buffers.input_position.get(),
        m_buffers.position[0].get(),
        vk::BufferCopy {0, 0, m_buffers.input_position.get_size()}
    );
    cmd_buf->fillBuffer(m_buffers.position[1].get(), 0, m_buffers.position[1].get_size(), 0u);
    cmd_buf->fillBuffer(m_buffers.velocity[0].get(), 0, m_buffers.velocity[0].get_size(), 0u);
    cmd_buf->fillBuffer(m_buffers.velocity[1].get(), 0, m_buffers.velocity[1].get_size(), 0u);
    cmd_buf->fillBuffer(m_buffers.uncompacted_block.get(), 0, m_buffers.uncompacted_block.get_size(), 0u);
    cmd_buf->fillBuffer(m_buffers.compacted_block.get(), 0, m_buffers.compacted_block.get_size(), 0u);
    cmd_buf->end();
    vk::SubmitInfo submit_info({}, {}, cmd_buf.get(), {});
    vk::Fence finish_fence = m_vk_context->m_device.createFence({});
    m_vk_context->m_queues.compute.submit(submit_info, finish_fence);

    auto _ = m_vk_context->m_device.waitForFences(finish_fence, VK_TRUE, std::numeric_limits<u64>::max());
}

void PhysicsEngine::step() {
    u32 next_ping_pong_idx = m_ping_pong_idx ^ 1;

    auto z_index_cmd_buf = m_z_indexer.generateZIndices(m_buffers.position[m_ping_pong_idx],
                                                        m_sim_params.num_particles,
                                                        m_buffers.z_index,
                                                        m_buffers.particle_index);

    auto sort_cmd_buf = m_radix_sorter.sort(m_sim_params.num_particles,
                                            m_buffers.z_index,
                                            m_buffers.sort_key_ping_pong,
                                            m_buffers.particle_index,
                                            m_buffers.sort_val_ping_pong);

    auto blocker_cmd_buf = m_blocker.computeBlocks(
            m_sim_params,
            m_buffers.z_index,
            m_buffers.uncompacted_block);

    auto compacter_cmd_buf = m_compactor.compactBlocks(m_sim_params,
                                                       m_buffers.uncompacted_block,
                                                       m_buffers.compacted_block,
                                                       m_buffers.dispatch_indirect);

    auto density_cmd_buf = m_density_compute.computeDensities(m_sim_params,
                                                              m_buffers.z_index,
                                                              m_buffers.particle_index,
                                                              m_buffers.position[m_ping_pong_idx],
                                                              m_buffers.density,
                                                              m_buffers.pressure,
                                                              m_buffers.compacted_block,
                                                              m_buffers.uncompacted_block,
                                                              m_buffers.dispatch_indirect);

    auto force_cmd_buf = m_force_compute.computeVelocities(m_sim_params,
                                                           m_buffers.z_index,
                                                           m_buffers.particle_index,
                                                           m_buffers.position[m_ping_pong_idx],
                                                           m_buffers.density,
                                                           m_buffers.velocity[m_ping_pong_idx],
                                                           m_buffers.pressure,
                                                           m_buffers.uncompacted_block,
                                                           m_buffers.compacted_block,
                                                           m_buffers.position[next_ping_pong_idx],
                                                           m_buffers.velocity[next_ping_pong_idx],
                                                           m_buffers.dispatch_indirect);

    vk::CommandBufferAllocateInfo primary_cmd_buf_alloc_info(m_vk_context->m_compute_command_pool, vk::CommandBufferLevel::ePrimary, 1);
    auto primary_cmd_buf = std::move(m_vk_context->m_device.allocateCommandBuffersUnique(primary_cmd_buf_alloc_info).front());
    primary_cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    primary_cmd_buf->executeCommands(z_index_cmd_buf.get());
    primary_cmd_buf->executeCommands(sort_cmd_buf.get());
    primary_cmd_buf->executeCommands(blocker_cmd_buf.get());
    primary_cmd_buf->executeCommands(compacter_cmd_buf.get());
    primary_cmd_buf->executeCommands(density_cmd_buf.get());
    primary_cmd_buf->executeCommands(force_cmd_buf.get());
    primary_cmd_buf->end();
    vk::SubmitInfo submit_info({}, {}, primary_cmd_buf.get(), {});
    vk::Fence finish_fence = m_vk_context->m_device.createFence({});
    m_vk_context->m_queues.compute.submit(submit_info, finish_fence);

    auto _ = m_vk_context->m_device.waitForFences(finish_fence, VK_TRUE, std::numeric_limits<u64>::max());

    m_ping_pong_idx = next_ping_pong_idx;
}
