#include <random>
#include "PhysicsEngine.h"

PhysicsEngine::PhysicsEngine(std::shared_ptr<App> app)
    : m_radix_sorter(app, 1024) {
    m_app = app;
	initSimulationParameters();
	initBuffers();
}

void PhysicsEngine::initSimulationParameters() {
	int n_part = 10;
	sim_params.number_particles = n_part;
	sim_params.particles = std::vector<glm::vec3>(n_part);

}

void PhysicsEngine::initBuffers() {
	initInputPosBuffer();
	initPosBuffer();
	initZIndexBuffer();
	initVelBuffer();
	initDensBuffer();
	initBlocksDataBuffer();
}

void PhysicsEngine::initInputPosBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::vec3);
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer;
	VmaAllocationCreateFlags memory_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buffers.input_pos_buf = std::move(VulkanBuffer(std::shared_ptr<App>(m_app)->m_allocator, alloc_size, usage, memory_flags));
}

void PhysicsEngine::initPosBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::vec3);
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.pos_buf = std::move(VulkanBuffer(std::shared_ptr<App>(m_app)->m_allocator, alloc_size, usage));
}

void PhysicsEngine::initZIndexBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::ivec3);
	buffers.zindex_buf = std::move(VulkanBuffer(std::shared_ptr<App>(m_app)->m_allocator, alloc_size));
}

void PhysicsEngine::initVelBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::vec3);
	buffers.vel_buf = std::move(VulkanBuffer(std::shared_ptr<App>(m_app)->m_allocator, alloc_size));
}

void PhysicsEngine::initDensBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(float);
	buffers.dens_buf = std::move(VulkanBuffer(std::shared_ptr<App>(m_app)->m_allocator, alloc_size));
}

void PhysicsEngine::initBlocksDataBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(BlockData);
	buffers.dens_buf = std::move(VulkanBuffer(std::shared_ptr<App>(m_app)->m_allocator, alloc_size));
}

PhysicsEngine::~PhysicsEngine() {

}

void PhysicsEngine::step() {
    std::vector<u32> data(256);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), std::mt19937(std::random_device()()));

    VulkanBuffer key_buffer = m_app->createCPUAccessibleBuffer(data.size() * sizeof(u32), vk::BufferUsageFlagBits::eStorageBuffer);
    VulkanBuffer key_ping_pong_buffer = m_app->createBuffer(data.size() * sizeof(u32), vk::BufferUsageFlagBits::eStorageBuffer);
    VulkanBuffer value_buffer = m_app->createCPUAccessibleBuffer(data.size() * sizeof(u32), vk::BufferUsageFlagBits::eStorageBuffer);
    VulkanBuffer value_ping_pong_buffer = m_app->createBuffer(data.size() * sizeof(u32), vk::BufferUsageFlagBits::eStorageBuffer);

    key_buffer.store_data(data.data(), data.size());
    value_buffer.store_data(data.data(), data.size());

    auto sort_cmd_buf = m_radix_sorter.sort(data.size(), key_buffer, key_ping_pong_buffer, value_buffer, value_ping_pong_buffer);

    vk::CommandBufferAllocateInfo primary_cmd_buf_alloc_info(m_app->m_compute_command_pool, vk::CommandBufferLevel::ePrimary, 1);
    auto primary_cmd_buf = m_app->m_device.allocateCommandBuffers(primary_cmd_buf_alloc_info).front();
    primary_cmd_buf.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    primary_cmd_buf.executeCommands(sort_cmd_buf);
    primary_cmd_buf.end();
    vk::SubmitInfo submit_info({}, {}, primary_cmd_buf, {});
    vk::Fence finish_fence = m_app->m_device.createFence({});
    m_app->m_queues.compute.submit(submit_info, finish_fence);

    auto _ = m_app->m_device.waitForFences(finish_fence, VK_TRUE, std::numeric_limits<u64>::max());

    key_buffer.load_data(data.data(), data.size());
}
