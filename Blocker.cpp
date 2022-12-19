#include "Blocker.h"

Blocker::Blocker(std::shared_ptr<VulkanContext> vulkan_context) : SimulatorComputeStage(vulkan_context) {
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayouts();
    createPipelines();
}

void Blocker::createDescriptorPool() {
    SimulatorComputeStage::createDescriptorPool(10, {
            { vk::DescriptorType::eStorageBuffer, 20 }
    });
}

void Blocker::createDescriptorSets() {
    auto bindings = std::array {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
    };

    vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
    m_descriptor_set_layout = m_vk_context->m_device.createDescriptorSetLayout(descriptor_set_layout_create_info);
    deferDelete([descriptor_set_layout = m_descriptor_set_layout](auto vk_context) {
        vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
    });

    vk::DescriptorSetAllocateInfo descriptor_set_alloc_info(m_descriptor_pool, m_descriptor_set_layout);

    vk::Result result;
    result = m_vk_context->m_device.allocateDescriptorSets(&descriptor_set_alloc_info, &m_descriptor_set);
    assert(result == vk::Result::eSuccess);
}

void Blocker::createPipelineLayouts() {
    vk::PushConstantRange constant_range(vk::ShaderStageFlagBits::eCompute, 0, sizeof(BlockerPushConstants));

    vk::PipelineLayoutCreateInfo layout_create_info({}, m_descriptor_set_layout, constant_range);
    m_pipeline_layout = m_vk_context->m_device.createPipelineLayout(layout_create_info);
    deferDelete([layout = m_pipeline_layout](auto vk_context) {
        vk_context->m_device.destroyPipelineLayout(layout);
    });
}

void Blocker::createPipelines() {
    vk::ShaderModule shader_module = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/blocker.spv");
    vk::PipelineShaderStageCreateInfo shader_stage_create_info({}, vk::ShaderStageFlagBits::eCompute, shader_module, "main", {});
    vk::ComputePipelineCreateInfo create_info({}, shader_stage_create_info, m_pipeline_layout, {}, {});
    m_pipeline = m_vk_context->m_device.createComputePipeline({}, create_info).value;
    deferDelete([pipeline = m_pipeline](auto vk_context) {
        vk_context->m_device.destroyPipeline(pipeline);
    });
}

vk::UniqueCommandBuffer Blocker::computeBlocks(SimulationParams &simulation_params, VulkanBuffer &z_index_buffer,
                                               VulkanBuffer &uncompacted_block_buffer) {
    writeDescriptorSet(z_index_buffer, uncompacted_block_buffer);

    auto cmd_buf = createCommandBuffer();

    cmd_buf->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 0, m_descriptor_set, {});

    auto push_constants = BlockerPushConstants {
        simulation_params.block_size
    };
    cmd_buf->pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, vk::ArrayProxy<const BlockerPushConstants>(push_constants));

    auto barriers = std::array {
        generalReadWriteBarrier(z_index_buffer),
        generalReadWriteBarrier(uncompacted_block_buffer)
    };

    cmd_buf->bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

    cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

    cmd_buf->dispatch(simulation_params.num_particles / 256 + 1, 1, 1);

    cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

    cmd_buf->end();

    return cmd_buf;
}

void Blocker::writeDescriptorSet(VulkanBuffer &z_index_buffer, VulkanBuffer &uncompacted_block_buffer) {
    auto buffers = std::array {
        z_index_buffer.getDescriptorBufferInfo(),
        uncompacted_block_buffer.getDescriptorBufferInfo()
    };

    vk::WriteDescriptorSet buffers_write(m_descriptor_set, 0, 0, vk::DescriptorType::eStorageBuffer, {}, buffers, {});

    m_vk_context->m_device.updateDescriptorSets(buffers_write, {});

}
