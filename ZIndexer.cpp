#include "ZIndexer.h"


ZIndexer::ZIndexer(std::shared_ptr<VulkanContext> vulkan_context, u32 grid_x, u32 grid_y, u32 grid_z, f32 grid_unit_size) :
    SimulatorComputeStage(vulkan_context),
    m_grid_x(grid_x),
    m_grid_y(grid_y),
    m_grid_z(grid_z),
    m_grid_unit_size(grid_unit_size) {
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayout();
    createPipelines();
    createLookupBuffers();
    writeInterleaveBuffersDescriptorSet();
}

std::vector<u32> ZIndexer::generateInterleavableX() {
    std::vector<u32> out;
    out.reserve(m_grid_x);

    for (u32 i = 0; i < m_grid_x; i++) {
        out.push_back(makeInterleavable(i, 3, 0));
    }

    return std::move(out);
}

std::vector<u32> ZIndexer::generateInterleavableY() {
    std::vector<u32> out;
    out.reserve(m_grid_y);

    for (u32 i = 0; i < m_grid_y; i++) {
        out.push_back(makeInterleavable(i, 3, 1));
    }

    return std::move(out);
}

std::vector<u32> ZIndexer::generateInterleavableZ() {
    std::vector<u32> out;
    out.reserve(m_grid_z);

    for (u32 i = 0; i < m_grid_z; i++) {
        out.push_back(makeInterleavable(i, 3, 2));
    }

    return std::move(out);
}

void ZIndexer::createLookupBuffers() {
    auto x_data = generateInterleavableX();
    auto y_data = generateInterleavableY();
    auto z_data = generateInterleavableZ();

    ZIndexLookupBuffers buffers = {
            m_vk_context->createCPUAccessibleBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), x_data.size()),
            m_vk_context->createCPUAccessibleBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), y_data.size()),
            m_vk_context->createCPUAccessibleBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(u32), z_data.size()),
    };

    buffers.x_lookup.load_data(x_data.data(), x_data.size());
    buffers.y_lookup.load_data(y_data.data(), y_data.size());
    buffers.z_lookup.load_data(z_data.data(), z_data.size());

    m_lookup_buffers = std::move(buffers);
}

struct ShaderPushConstants {
    u32 grid_size_x;
    u32 grid_size_y;
    u32 grid_size_z;
    f32 grid_unit_size;
};

vk::UniqueCommandBuffer ZIndexer::generateZIndices(VulkanBuffer& particles, u32 num_particles, VulkanBuffer& z_index_buffer,
                                             VulkanBuffer& particle_index_buffer) {
    writeParticleBuffersDescriptorSet(particles, num_particles, particle_index_buffer, z_index_buffer);

    auto cmd_buf = createCommandBuffer();

    auto descriptor_sets = std::array {m_descriptor_sets.particle_buffers, m_descriptor_sets.interleave_buffers};
    cmd_buf->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 0, descriptor_sets, {});

    auto push_constants = ShaderPushConstants {
        m_grid_x,
        m_grid_y,
        m_grid_z,
        m_grid_unit_size
    };

    cmd_buf->pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eAll, 0, vk::ArrayProxy<const ShaderPushConstants>(push_constants));

    std::array<vk::BufferMemoryBarrier, 3> barriers;

    barriers[0] = bufferTransition(particles.get(), vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, particles.get_size());
    barriers[1] = bufferTransition(z_index_buffer.get(), vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, num_particles * sizeof(u32));
    barriers[2] = bufferTransition(particle_index_buffer.get(), vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, num_particles * sizeof(u32));

    cmd_buf->bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);

    cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

    cmd_buf->dispatch((num_particles / 256) + 1, 1, 1);

    cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});

    cmd_buf->end();
    return std::move(cmd_buf);
}

void ZIndexer::createPipelines() {
    auto shader_module = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/z_indexer.spv");
    auto shader_stage_create_info = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eCompute, shader_module, "main", {});
    auto create_info = vk::ComputePipelineCreateInfo({}, shader_stage_create_info, m_pipeline_layout, {}, {});
    m_pipeline = m_vk_context->m_device.createComputePipeline({}, create_info).value;
    deferDelete([pipeline = m_pipeline](auto vk_context){
        vk_context->m_device.destroyPipeline(pipeline);
    });
}

void ZIndexer::createPipelineLayout() {
    vk::PushConstantRange constant_range(vk::ShaderStageFlagBits::eAll, 0, sizeof(ShaderPushConstants));

    std::array<vk::DescriptorSetLayout, 2> descriptor_set_layouts = {m_descriptor_sets.particle_buffers_layout, m_descriptor_sets.interleave_buffers_layout};

    vk::PipelineLayoutCreateInfo layout_create_info({}, descriptor_set_layouts, constant_range);
    m_pipeline_layout = m_vk_context->m_device.createPipelineLayout(layout_create_info);
    deferDelete([layout = m_pipeline_layout](auto vk_context){
        vk_context->m_device.destroyPipelineLayout(layout);
    });
}

void ZIndexer::createDescriptorSets() {
    auto particle_buffer_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}),
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {})
    };

    auto interleave_buffer_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}),
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {})
    };

    m_descriptor_sets.particle_buffers_layout = m_vk_context->m_device.createDescriptorSetLayout({{}, particle_buffer_bindings});
    deferDelete([descriptor_set_layout = m_descriptor_sets.particle_buffers_layout](auto vk_context) {
       vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
    });
    m_descriptor_sets.interleave_buffers_layout = m_vk_context->m_device.createDescriptorSetLayout({{}, interleave_buffer_bindings});
    deferDelete([descriptor_set_layout = m_descriptor_sets.interleave_buffers_layout](auto vk_context) {
        vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
    });

    auto particle_buffer_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.particle_buffers_layout);
    auto interleave_buffer_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.interleave_buffers_layout);

    vk::Result result;
    result = m_vk_context->m_device.allocateDescriptorSets(&particle_buffer_set_alloc_info, &m_descriptor_sets.particle_buffers);
    assert(result == vk::Result::eSuccess);
    result = m_vk_context->m_device.allocateDescriptorSets(&interleave_buffer_set_alloc_info, &m_descriptor_sets.interleave_buffers);
    assert(result == vk::Result::eSuccess);
}

void ZIndexer::createDescriptorPool() {
    SimulatorComputeStage::createDescriptorPool(15, {
            {vk::DescriptorType::eStorageBuffer, 50},
    });
}

void ZIndexer::writeInterleaveBuffersDescriptorSet() {
    auto buffers = std::array {
        vk::DescriptorBufferInfo(m_lookup_buffers.x_lookup.get(), 0, (u32) m_grid_x * sizeof(u32)),
        vk::DescriptorBufferInfo(m_lookup_buffers.y_lookup.get(), 0, (u32) m_grid_y * sizeof(u32)),
        vk::DescriptorBufferInfo(m_lookup_buffers.z_lookup.get(), 0, (u32) m_grid_z * sizeof(u32)),
    };
    vk::WriteDescriptorSet descriptor_write(m_descriptor_sets.interleave_buffers, 0, 0, vk::DescriptorType::eStorageBuffer, {}, buffers, {});

    m_vk_context->m_device.updateDescriptorSets(descriptor_write, {});
}

void ZIndexer::writeParticleBuffersDescriptorSet(VulkanBuffer &particle_buffer, u32 num_particles,
                                                 VulkanBuffer &particle_index_buffer, VulkanBuffer &z_index_buffer) {
    auto buffers = std::array {
            vk::DescriptorBufferInfo(particle_buffer.get(), 0, particle_buffer.get_size()),
            vk::DescriptorBufferInfo(z_index_buffer.get(), 0, (u32) num_particles * sizeof(u32)),
            vk::DescriptorBufferInfo(particle_index_buffer.get(), 0, (u32) num_particles * sizeof(u32)),
    };
    vk::WriteDescriptorSet descriptor_write(m_descriptor_sets.particle_buffers, 0, 0, vk::DescriptorType::eStorageBuffer, {}, buffers, {});

    m_vk_context->m_device.updateDescriptorSets(descriptor_write, {});
}