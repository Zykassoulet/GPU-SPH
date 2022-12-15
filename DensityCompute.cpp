#include "DensityCompute.h"
#include <array>



DensityCompute::DensityCompute() {
	createDescriptorPool();
	createDescriptorSets();
	createPipelines();
}


void DensityCompute::createDescriptorPool() {
	SimulatorComputeStage::createDescriptorPool(10, {
		{ vk::DescriptorType::eStorageBuffer,10 }
	});
}

void DensityCompute::createDescriptorSets() {
	auto density_buffer_bindings = std::array{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
	};


	vk::DescriptorSetLayoutCreateInfo layout_info({}, density_buffer_bindings);
	descriptor_sets.layout = m_vk_context->m_device.createDescriptorSetLayout(layout_info);
	deferDelete([descriptor_set_layout = descriptor_sets.layout](auto vk_context) {
		vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
	});

	vk::DescriptorSetAllocateInfo set_alloc_info(m_descriptor_pool, descriptor_sets.layout);

	vk::Result result;
	result = m_vk_context->m_device.allocateDescriptorSets(&set_alloc_info, &descriptor_sets.set);
	assert(result == vk::Result::eSuccess);
}

void DensityCompute::createPipelines() {

}

void DensityCompute::computeDensities(VulkanBuffer& position_buffer, u32 num_particles, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer) {

}

void DensityCompute::writeBuffersDescriptorSet(VulkanBuffer& position_buffer, u32 num_particles, VulkanBuffer& blocks_buffer, VulkanBuffer& density_buffer) {
	vk::DescriptorBufferInfo pos_desc_buf(position_buffer.get(), 0, )
}





