#include "PhysicsEngine.h"

PhysicsEngine::PhysicsEngine(std::shared_ptr<App> app_ptr) {
	gpu_radix_sorter_ptr = std::make_shared<GPURadixSorter>(app_ptr);
	app_wptr = std::weak_ptr<App>(app_ptr);
	initSimulationParameters();
	initSyncStructures();
	initBuffers();
	initDescriptors();
	initPipelines();
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
	buffers.input_pos_buf = std::move(VulkanBuffer(std::shared_ptr<App>(app_wptr)->m_allocator, alloc_size, usage, memory_flags));
}

void PhysicsEngine::initPosBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::vec3);
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;
	buffers.pos_buf = std::move(VulkanBuffer(std::shared_ptr<App>(app_wptr)->m_allocator, alloc_size, usage));
}

void PhysicsEngine::initZIndexBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::ivec3);
	buffers.zindex_buf = std::move(VulkanBuffer(std::shared_ptr<App>(app_wptr)->m_allocator, alloc_size));
}

void PhysicsEngine::initVelBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(glm::vec3);
	buffers.vel_buf = std::move(VulkanBuffer(std::shared_ptr<App>(app_wptr)->m_allocator, alloc_size));
}

void PhysicsEngine::initDensBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(float);
	buffers.dens_buf = std::move(VulkanBuffer(std::shared_ptr<App>(app_wptr)->m_allocator, alloc_size));
}

void PhysicsEngine::initBlocksDataBuffer() {
	size_t alloc_size = sim_params.number_particles * sizeof(BlockData);
	buffers.dens_buf = std::move(VulkanBuffer(std::shared_ptr<App>(app_wptr)->m_allocator, alloc_size));
}

void PhysicsEngine::initDescriptors() {

	vk::Device device = std::shared_ptr<App>(app_wptr)->m_device;

	//create descriptor pool
	auto pool_sizes = std::array{
	vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 50)
	};

	descriptor_pool = device.createDescriptorPool(vk::DescriptorPoolCreateInfo({}, 10, pool_sizes));

	//create layout bindings
	auto blocks_data_bindings = std::array{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
	};

	auto density_bindings = std::array{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
	};

	auto forces_bindings = std::array{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
	};
	

	//create layouts
	vk::DescriptorSetLayoutCreateInfo blocks_data_set_info({}, 2, blocks_data_bindings.data());
	layouts.blocks_data_layout = device.createDescriptorSetLayout(blocks_data_set_info);
	vk::DescriptorSetLayoutCreateInfo density_set_info({}, 2, density_bindings.data());
	layouts.density_layout = device.createDescriptorSetLayout(density_set_info);
	vk::DescriptorSetLayoutCreateInfo forces_set_info({}, 4, forces_bindings.data());
	layouts.forces_layout = device.createDescriptorSetLayout(forces_set_info);

	//allocate sets

	//TODO

}

void PhysicsEngine::initPipelines() {

}

PhysicsEngine::~PhysicsEngine() {
	vk::Device device = std::shared_ptr<App>(app_wptr)->m_device;

	device.destroyDescriptorSetLayout(layouts.blocks_data_layout);
	device.destroyDescriptorSetLayout(layouts.density_layout);
	device.destroyDescriptorSetLayout(layouts.forces_layout);
	device.destroyDescriptorPool(descriptor_pool);
}
