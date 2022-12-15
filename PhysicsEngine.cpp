#include <random>
#include "PhysicsEngine.h"

PhysicsEngine::PhysicsEngine(std::shared_ptr<VulkanContext> vulkan_context)
    : m_radix_sorter(vulkan_context, 1024) {
    m_vk_context = vulkan_context;
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
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer;
	VmaAllocationCreateFlags memory_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buffers.input_pos_buf = std::shared_ptr<VulkanContext>(m_vk_context)->createCPUAccessibleBuffer(usage, sizeof(glm::vec3), sim_params.number_particles);
}

void PhysicsEngine::initPosBuffer() {
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.pos_buf = std::shared_ptr<VulkanContext>(m_vk_context)->createBuffer(usage, sizeof(glm::vec3), sim_params.number_particles);
}

void PhysicsEngine::initZIndexBuffer() {
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.zindex_buf = std::shared_ptr<VulkanContext>(m_vk_context)->createBuffer(usage, sizeof(glm::ivec3), sim_params.number_particles);
}

void PhysicsEngine::initVelBuffer() {
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.vel_buf = std::shared_ptr<VulkanContext>(m_vk_context)->createBuffer(usage, sizeof(glm::vec3), sim_params.number_particles);
}

void PhysicsEngine::initDensBuffer() {
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.dens_buf = std::shared_ptr<VulkanContext>(m_vk_context)->createBuffer(usage, sizeof(float), sim_params.number_particles);
}

void PhysicsEngine::initBlocksDataBuffer() {
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.dens_buf = std::shared_ptr<VulkanContext>(m_vk_context)->createBuffer(usage, sizeof(BlockData), sim_params.number_particles);
}

PhysicsEngine::~PhysicsEngine() {

}

void PhysicsEngine::step() {
    std::vector<u32> data(256);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), std::mt19937(std::random_device()()));

    VulkanBuffer key_buffer = m_vk_context->createCPUAccessibleBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), data.size());
    VulkanBuffer key_ping_pong_buffer = m_vk_context->createBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), data.size());
    VulkanBuffer value_buffer = m_vk_context->createCPUAccessibleBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), data.size());
    VulkanBuffer value_ping_pong_buffer = m_vk_context->createBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), data.size());

    key_buffer.store_data(data.data(), data.size());
    value_buffer.store_data(data.data(), data.size());

    auto sort_cmd_buf = m_radix_sorter.sort(data.size(), key_buffer, key_ping_pong_buffer, value_buffer, value_ping_pong_buffer);

    vk::CommandBufferAllocateInfo primary_cmd_buf_alloc_info(m_vk_context->m_compute_command_pool, vk::CommandBufferLevel::ePrimary, 1);
    auto primary_cmd_buf = std::move(m_vk_context->m_device.allocateCommandBuffersUnique(primary_cmd_buf_alloc_info).front());
    primary_cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    primary_cmd_buf->executeCommands(sort_cmd_buf.get());
    primary_cmd_buf->end();
    vk::SubmitInfo submit_info({}, {}, primary_cmd_buf.get(), {});
    vk::Fence finish_fence = m_vk_context->m_device.createFence({});
    m_vk_context->m_queues.compute.submit(submit_info, finish_fence);

    auto _ = m_vk_context->m_device.waitForFences(finish_fence, VK_TRUE, std::numeric_limits<u64>::max());

    key_buffer.load_data(data.data(), data.size());
}
