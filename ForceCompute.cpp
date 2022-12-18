#include "ForceCompute.h"
#include "SimulationParams.h"
#include <array>



ForceCompute::ForceCompute(std::shared_ptr<VulkanContext> vulkan_context) : SimulatorComputeStage(vulkan_context) {
	createDescriptorPool();
	createDescriptorSets();
	createPipelineLayouts();
	createPipelines();
}


void ForceCompute::createDescriptorPool() {
	SimulatorComputeStage::createDescriptorPool(10, {
		{ vk::DescriptorType::eStorageBuffer,100 }
    });
}

void ForceCompute::createDescriptorSets() {
	auto particle_buffer_bindings = std::array {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
        vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
        vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
	};

    auto block_buffer_bindings = std::array {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
    };

    auto output_buffer_bindings = std::array {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
    };


	vk::DescriptorSetLayoutCreateInfo particle_buffers_layout_create_info({}, particle_buffer_bindings);
    m_descriptor_sets.particle_buffers_layout = m_vk_context->m_device.createDescriptorSetLayout(particle_buffers_layout_create_info);
	deferDelete([descriptor_set_layout = m_descriptor_sets.particle_buffers_layout](auto vk_context) {
		vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
	});

    vk::DescriptorSetLayoutCreateInfo block_buffers_layout_create_info({}, block_buffer_bindings);
    m_descriptor_sets.block_buffers_layout = m_vk_context->m_device.createDescriptorSetLayout(block_buffers_layout_create_info);
    deferDelete([descriptor_set_layout = m_descriptor_sets.block_buffers_layout](auto vk_context) {
        vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
    });

    vk::DescriptorSetLayoutCreateInfo output_buffers_layout_create_info({}, output_buffer_bindings);
    m_descriptor_sets.output_buffers_layout = m_vk_context->m_device.createDescriptorSetLayout(output_buffers_layout_create_info);
    deferDelete([descriptor_set_layout = m_descriptor_sets.output_buffers_layout](auto vk_context) {
        vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
    });

	vk::DescriptorSetAllocateInfo particle_buffers_set_alloc_info(m_descriptor_pool, m_descriptor_sets.particle_buffers_layout);
    vk::DescriptorSetAllocateInfo block_buffers_set_alloc_info(m_descriptor_pool, m_descriptor_sets.block_buffers_layout);
    vk::DescriptorSetAllocateInfo output_buffers_set_alloc_info(m_descriptor_pool, m_descriptor_sets.output_buffers_layout);

	vk::Result result;
	result = m_vk_context->m_device.allocateDescriptorSets(&particle_buffers_set_alloc_info, &m_descriptor_sets.particle_buffers);
	assert(result == vk::Result::eSuccess);
    result = m_vk_context->m_device.allocateDescriptorSets(&block_buffers_set_alloc_info, &m_descriptor_sets.block_buffers);
    assert(result == vk::Result::eSuccess);
    result = m_vk_context->m_device.allocateDescriptorSets(&output_buffers_set_alloc_info, &m_descriptor_sets.output_buffers);
    assert(result == vk::Result::eSuccess);
}

void ForceCompute::createPipelineLayouts() {
	vk::PushConstantRange constant_range(vk::ShaderStageFlagBits::eCompute, 0, sizeof(ForceComputePushConstants));

    auto descriptor_set_layouts = m_descriptor_sets.getLayouts();

	vk::PipelineLayoutCreateInfo layout_create_info({}, descriptor_set_layouts, constant_range);
    m_pipeline_layout = m_vk_context->m_device.createPipelineLayout(layout_create_info);
	deferDelete([layout = m_pipeline_layout](auto vk_context) {
		vk_context->m_device.destroyPipelineLayout(layout);
    });
}

void ForceCompute::createPipelines() {
	vk::ShaderModule shader_module = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/forces.spv");
	vk::PipelineShaderStageCreateInfo shader_stage_create_info({}, vk::ShaderStageFlagBits::eCompute, shader_module, "main", {});
	vk::ComputePipelineCreateInfo create_info({}, shader_stage_create_info, m_pipeline_layout, {}, {});
    m_pipeline = m_vk_context->m_device.createComputePipeline({}, create_info).value;
	deferDelete([pipeline = m_pipeline](auto vk_context) {
		vk_context->m_device.destroyPipeline(pipeline);
    });
}

vk::UniqueCommandBuffer
ForceCompute::computeVelocities(SimulationParams simulation_params, VulkanBuffer &z_index_buffer,
                                VulkanBuffer &particle_index_buffer, VulkanBuffer &position_buffer,
                                VulkanBuffer &density_buffer, VulkanBuffer &velocity_buffer,
                                VulkanBuffer &pressure_buffer, VulkanBuffer &uncompacted_blocks_buffer,
                                VulkanBuffer &compacted_blocks_buffer, VulkanBuffer &out_position_buffer,
                                VulkanBuffer &out_velocity_buffer, VulkanBuffer &dispatch_indirect_buffer) {
    writeBufferDescriptorSets(z_index_buffer,
                              particle_index_buffer,
                              position_buffer,
                              density_buffer,
                              velocity_buffer,
                              pressure_buffer,
                              uncompacted_blocks_buffer,
                              compacted_blocks_buffer,
                              out_position_buffer,
                              out_velocity_buffer);

	auto cmd_buf = createCommandBuffer();

	cmd_buf->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 0, m_descriptor_sets.getSets(), {});

    auto push_constants = ForceComputePushConstants {
        simulation_params.grid_size,
        simulation_params.block_size,
        simulation_params.kernel_radius,
        simulation_params.particle_mass,
        simulation_params.dt
    };
	cmd_buf->pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, vk::ArrayProxy<const ForceComputePushConstants>(push_constants));

	auto barriers = std::array{
            generalReadWriteBarrier(z_index_buffer),
            generalReadWriteBarrier(particle_index_buffer),
            generalReadWriteBarrier(position_buffer),
            generalReadWriteBarrier(density_buffer),
            generalReadWriteBarrier(velocity_buffer),
            generalReadWriteBarrier(pressure_buffer),
            generalReadWriteBarrier(uncompacted_blocks_buffer),
            generalReadWriteBarrier(compacted_blocks_buffer),
            generalReadWriteBarrier(out_position_buffer),
            generalReadWriteBarrier(out_velocity_buffer)
	};


	cmd_buf->bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

	cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

	cmd_buf->dispatchIndirect(dispatch_indirect_buffer.get(), 0);

	cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

	cmd_buf->end();

	return cmd_buf;
}

void ForceCompute::writeBufferDescriptorSets(VulkanBuffer &z_index_buffer,
                                             VulkanBuffer &particle_index_buffer, VulkanBuffer &position_buffer,
                                             VulkanBuffer &density_buffer, VulkanBuffer &velocity_buffer,
                                             VulkanBuffer &pressure_buffer, VulkanBuffer &uncompacted_blocks_buffer,
                                             VulkanBuffer &compacted_blocks_buffer, VulkanBuffer &out_position_buffer,
                                             VulkanBuffer &out_velocity_buffer) {
	auto particle_buffers = std::array {
        z_index_buffer.getDescriptorBufferInfo(),
        particle_index_buffer.getDescriptorBufferInfo(),
        position_buffer.getDescriptorBufferInfo(),
        density_buffer.getDescriptorBufferInfo(),
        velocity_buffer.getDescriptorBufferInfo(),
        pressure_buffer.getDescriptorBufferInfo()
    };

    auto block_buffers = std::array {
        uncompacted_blocks_buffer.getDescriptorBufferInfo(),
        compacted_blocks_buffer.getDescriptorBufferInfo()
    };

    auto output_buffers = std::array {
        out_position_buffer.getDescriptorBufferInfo(),
        out_velocity_buffer.getDescriptorBufferInfo()
    };

	vk::WriteDescriptorSet particle_buffers_write(m_descriptor_sets.particle_buffers, 0, 0, vk::DescriptorType::eStorageBuffer, {}, particle_buffers, {});
    vk::WriteDescriptorSet block_buffers_write(m_descriptor_sets.block_buffers, 0, 0, vk::DescriptorType::eStorageBuffer, {}, block_buffers, {});
    vk::WriteDescriptorSet output_buffers_write(m_descriptor_sets.output_buffers, 0, 0, vk::DescriptorType::eStorageBuffer, {}, output_buffers, {});


	m_vk_context->m_device.updateDescriptorSets({particle_buffers_write, block_buffers_write, output_buffers_write}, {});
}





