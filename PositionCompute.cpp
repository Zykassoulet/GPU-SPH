#include "PositionCompute.h"
#include <array>



PositionCompute::PositionCompute(std::shared_ptr<VulkanContext> vulkan_context) : SimulatorComputeStage(vulkan_context) {
	createDescriptorPool();
	createDescriptorSets();
	createPipelineLayouts();
	createPipelines();
}


void PositionCompute::createDescriptorPool() {
	SimulatorComputeStage::createDescriptorPool(10, {
		{ vk::DescriptorType::eStorageBuffer,10 }
		});
}

void PositionCompute::createDescriptorSets() {
	auto position_buffer_bindings = std::array{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
	};


	vk::DescriptorSetLayoutCreateInfo layout_info({}, position_buffer_bindings);
	descriptor_sets.layout = m_vk_context->m_device.createDescriptorSetLayout(layout_info);
	deferDelete([descriptor_set_layout = descriptor_sets.layout](auto vk_context) {
		vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
	});

	vk::DescriptorSetAllocateInfo set_alloc_info(m_descriptor_pool, descriptor_sets.layout);

	vk::Result result;
	result = m_vk_context->m_device.allocateDescriptorSets(&set_alloc_info, &descriptor_sets.set);
	assert(result == vk::Result::eSuccess);
}

void PositionCompute::createPipelineLayouts() {
	vk::PushConstantRange constant_range(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PositionComputePushConstants));

	vk::PipelineLayoutCreateInfo layout_create_info({}, descriptor_sets.layout, constant_range);
	pipeline_layout = m_vk_context->m_device.createPipelineLayout(layout_create_info);
	deferDelete([layout = pipeline_layout](auto vk_context) {
		vk_context->m_device.destroyPipelineLayout(layout);
		});
}

void PositionCompute::createPipelines() {
	vk::ShaderModule shader_module = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/position.spv");
	vk::PipelineShaderStageCreateInfo shader_stage_create_info({}, vk::ShaderStageFlagBits::eCompute, shader_module, "main", {});
	vk::ComputePipelineCreateInfo create_info({}, shader_stage_create_info, pipeline_layout, {}, {});
	pipeline = m_vk_context->m_device.createComputePipeline({}, create_info).value;
	deferDelete([pipeline = pipeline](auto vk_context) {
		vk_context->m_device.destroyPipeline(pipeline);
		});
}

vk::UniqueCommandBuffer PositionCompute::computeVelocities(VulkanBuffer& position_buffer, VulkanBuffer& velocity_buffer, PositionComputePushConstants push_constants) {
	auto cmd_buf = createCommandBuffer();

	cmd_buf->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline_layout, 0, descriptor_sets.set, {});

	cmd_buf->pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, vk::ArrayProxy<const PositionComputePushConstants>(push_constants));

	auto barriers = std::array{
		bufferTransition(position_buffer.get(), vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, position_buffer.get_size()),
		bufferTransition(velocity_buffer.get(), vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, velocity_buffer.get_size())
	};


	cmd_buf->bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);

	cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

	cmd_buf->dispatch((push_constants.num_particles / 256) + 1, 1, 1);

	cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

	cmd_buf->end();

	return cmd_buf;
}

void PositionCompute::writeBuffersDescriptorSet(VulkanBuffer& position_buffer, VulkanBuffer& velocity_buffer) {
	auto buffers = std::array{
		position_buffer.getDescriptorBufferInfo(),
		velocity_buffer.getDescriptorBufferInfo()
	};

	vk::WriteDescriptorSet decriptor_set_write(descriptor_sets.set, 0, 0, vk::DescriptorType::eStorageBuffer, {}, buffers, {});

	m_vk_context->m_device.updateDescriptorSets(decriptor_set_write, {});
}





