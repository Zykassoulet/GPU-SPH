#include "Compactor.h"

Compactor::Compactor(std::shared_ptr<VulkanContext> vulkan_context) : SimulatorComputeStage(vulkan_context) {
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayouts();
    createPipelines();
}

void Compactor::createDescriptorPool() {
    SimulatorComputeStage::createDescriptorPool(10, {
            { vk::DescriptorType::eStorageBuffer, 30 }
    });
}

void Compactor::createDescriptorSets() {
    auto bindings = std::array {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{}),
            vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute,{})
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

void Compactor::createPipelineLayouts() {
    vk::PipelineLayoutCreateInfo layout_create_info({}, m_descriptor_set_layout, {});
    m_pipeline_layout = m_vk_context->m_device.createPipelineLayout(layout_create_info);
    deferDelete([layout = m_pipeline_layout](auto vk_context) {
        vk_context->m_device.destroyPipelineLayout(layout);
    });
}

void Compactor::createPipelines() {
    vk::ShaderModule shader_module = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/compactor.spv");
    vk::PipelineShaderStageCreateInfo shader_stage_create_info({}, vk::ShaderStageFlagBits::eCompute, shader_module, "main", {});
    vk::ComputePipelineCreateInfo create_info({}, shader_stage_create_info, m_pipeline_layout, {}, {});
    m_pipeline = m_vk_context->m_device.createComputePipeline({}, create_info).value;
    deferDelete([pipeline = m_pipeline](auto vk_context) {
        vk_context->m_device.destroyPipeline(pipeline);
    });
}

vk::UniqueCommandBuffer
Compactor::compactBlocks(SimulationParams &simulation_params, VulkanBuffer &uncompacted_block_buffer,
                         VulkanBuffer &compacted_block_buffer, VulkanBuffer &dispatch_indirect_buffer) {
    writeDescriptorSet(uncompacted_block_buffer, compacted_block_buffer, dispatch_indirect_buffer);

    auto cmd_buf = createCommandBuffer();

    cmd_buf->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 0, m_descriptor_set, {});

    auto barriers = std::array {
            generalReadWriteBarrier(uncompacted_block_buffer),
            generalReadWriteBarrier(compacted_block_buffer),
            generalReadWriteBarrier(dispatch_indirect_buffer)
    };

    cmd_buf->bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

    cmd_buf->fillBuffer(dispatch_indirect_buffer.get(), 0, VK_WHOLE_SIZE, 0);

    cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

    cmd_buf->dispatch(simulation_params.num_blocks / 256 + 1, 1, 1);

    cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

    cmd_buf->end();

    return cmd_buf;
}

void Compactor::writeDescriptorSet(VulkanBuffer &uncompacted_block_buffer, VulkanBuffer &compacted_block_buffer, VulkanBuffer &dispatch_indirect_buffer) {
    auto buffers = std::array {
            uncompacted_block_buffer.getDescriptorBufferInfo(),
            compacted_block_buffer.getDescriptorBufferInfo(),
            dispatch_indirect_buffer.getDescriptorBufferInfo()
    };

    vk::WriteDescriptorSet buffers_write(m_descriptor_set, 0, 0, vk::DescriptorType::eStorageBuffer, {}, buffers, {});

    m_vk_context->m_device.updateDescriptorSets(buffers_write, {});

}
