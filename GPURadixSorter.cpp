#include "GPURadixSorter.h"
#define FFX_CPP
#include <FFX_ParallelSort.h>


vk::CommandBuffer GPURadixSorter::sort(
        u32 num_keys,
        VulkanBuffer& key_buf,
        VulkanBuffer& key_ping_pong_buf,
        VulkanBuffer& value_buf,
        VulkanBuffer& value_ping_pong_buf) {

    vk::Buffer& read_buffer = key_buf.get();
    vk::Buffer& write_buffer = key_ping_pong_buf.get();
    vk::Buffer& read_payload_buffer = value_buf.get();
    vk::Buffer& write_payload_buffer = value_ping_pong_buf.get();

    bindInputOutputBuffers(key_buf.get(), key_ping_pong_buf.get(), value_buf.get(), value_ping_pong_buf.get());

    FFX_ParallelSortCB constantBufferData = {0};
    u32 num_threadgroups_to_run, num_reduced_threadgroups_to_run;

    FFX_ParallelSort_SetConstantAndDispatchData(num_keys, 800, constantBufferData, num_threadgroups_to_run,
                                                num_reduced_threadgroups_to_run);

    auto cmd_buf = createCommandBuffer();

    cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 4, m_descriptor_sets.scratch_set, {});

    auto [const_buf, const_buf_info] = allocConstBuffer(constantBufferData);
    m_constant_buffer = std::move(const_buf);
    bindConstantBuffer(const_buf_info, m_descriptor_sets.constant_set, 0, 1);

    cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 0, m_descriptor_sets.constant_set, {});

    vk::BufferMemoryBarrier barriers[3];
    u32 input_set = 0;
    for (u32 shift = 0; shift < 32u; shift += FFX_PARALLELSORT_SORT_BITS_PER_PASS) {
        cmd_buf.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eAll, 0, sizeof(u32), &shift);

        cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 2,
                                   m_descriptor_sets.input_output_sets[input_set], {});

        {
            cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipelines.count);
            cmd_buf.dispatch(num_threadgroups_to_run, 1, 1);
        }

        barriers[0] = bufferTransition(m_scratch_buffer.get(),
                                       vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                       vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                       m_scratch_buffer_size);
        cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {},
                                {}, barriers[0], {});

        {
            cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipelines.count_reduce);
            cmd_buf.dispatch(num_reduced_threadgroups_to_run, 1, 1);
            barriers[0] = bufferTransition(m_reduced_scratch_buffer.get(),
                                           vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                           vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                           m_reduced_scratch_buffer_size);
            cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                    {}, {}, barriers[0], {});
        }

        {
            cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 3,
                                       m_descriptor_sets.scan_sets[0], {});
            cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipelines.prefix_scan);
            cmd_buf.dispatch(1, 1, 1);
            barriers[0] = bufferTransition(m_reduced_scratch_buffer.get(),
                                           vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                           vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                           m_reduced_scratch_buffer_size);
            cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
                                    {}, {}, barriers[0], {});

            cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline_layout, 3,
                                       m_descriptor_sets.scan_sets[1], {});
            cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipelines.scan_add);
            cmd_buf.dispatch(num_reduced_threadgroups_to_run, 1, 1);
        }

        barriers[0] = bufferTransition(m_scratch_buffer.get(),
                                       vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                       vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                       m_scratch_buffer_size);
        cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {},
                                {}, barriers[0], {});

        {
            cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipelines.scatter);
            cmd_buf.dispatch(num_threadgroups_to_run, 1, 1);
        }

        int num_barriers = 0;
        barriers[num_barriers++] = bufferTransition(write_buffer,
                                                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                                    sizeof(u32) * num_keys);
        barriers[num_barriers++] = bufferTransition(write_payload_buffer,
                                                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                                                    sizeof(u32) * num_keys); // FIXME: Make payload not be u32
        cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, 0,
                                nullptr, num_barriers, barriers, 0, nullptr);

        std::swap(read_buffer, write_buffer);
        std::swap(read_payload_buffer, write_payload_buffer);
        input_set = !input_set;
    }

    cmd_buf.end();

    return cmd_buf;
}

GPURadixSorter::GPURadixSorter(std::shared_ptr<VulkanContext> vulkan_context, u32 max_keys) : SimulatorComputeStage(std::move(vulkan_context)) {
    FFX_ParallelSort_CalculateScratchResourceSize(max_keys, m_scratch_buffer_size, m_reduced_scratch_buffer_size);

    allocateScratchBuffers(m_scratch_buffer_size, m_reduced_scratch_buffer_size);
    createDescriptorPool();
    createDescriptorSets();
    createPipelines();
    bindStaticBufferDescriptors();
}

void GPURadixSorter::allocateScratchBuffers(u32 scratch_buffer_size,
                                       u32 reduced_scratch_buffer_size) {

    vk::BufferCreateInfo create_info;
    {
        using usage = vk::BufferUsageFlagBits;
        create_info.sharingMode = vk::SharingMode::eExclusive;
        create_info.usage = usage::eStorageTexelBuffer | usage::eStorageBuffer | usage::eTransferDst;
    }

    create_info.size = scratch_buffer_size;
    m_scratch_buffer = std::move(VulkanBuffer(m_app->m_allocator, create_info));
    create_info.size = reduced_scratch_buffer_size;
    m_reduced_scratch_buffer = std::move(VulkanBuffer(m_app->m_allocator, create_info));
}

void GPURadixSorter::createPipelines() {
    auto layout = createSortPipelineLayout();
    m_pipeline_layout = layout;
    m_pipelines = {
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_Count.spv", "FPS_Count", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_CountReduce.spv", "FPS_CountReduce", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_Scan.spv", "FPS_Scan", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_ScanAdd.spv", "FPS_ScanAdd", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_Scatter.spv", "FPS_Scatter", layout)
    };
}

vk::Pipeline GPURadixSorter::createRadixPipeline(std::string shader_file, std::string entry_point, vk::PipelineLayout& layout) {
    auto shader_module = createShaderModuleFromFile(m_app->m_device, shader_file);
    auto shader_stage_create_info = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eCompute, shader_module, entry_point.data(), {});
    auto create_info = vk::ComputePipelineCreateInfo({}, shader_stage_create_info, layout, {}, {});
    vk::Pipeline pipeline = m_app->m_device.createComputePipeline({}, create_info).value;
    return pipeline;
}

vk::PipelineLayout GPURadixSorter::createSortPipelineLayout() {
    vk::PushConstantRange constant_range(vk::ShaderStageFlagBits::eAll, 0, 4);

    auto descriptor_set_layouts = m_descriptor_sets.get_layouts();

    vk::PipelineLayoutCreateInfo layout_create_info({}, descriptor_set_layouts, constant_range);
    return m_app->m_device.createPipelineLayout(layout_create_info);
}

void GPURadixSorter::createDescriptorSets() {
    auto constant_bindings =  std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eAll, {}) // Constant buffer table
    };

    auto indirect_constant_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,  vk::ShaderStageFlagBits::eAll, {}) // Constant buffer table
    };

    auto input_output_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // SrcBuffer
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // DstBuffer
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // SrcPayload
        vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}) // DstPayload
    };

    auto scan_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // ScanSrc
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // ScanDst
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}) // ScanScratch
    };

    auto scratch_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // Scratch
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}) // Scratch (reduced)
    };

    auto indirect_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // NumKeys
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // CBufferUAV
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}), // CountScatterArgs
        vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eAll, {}) // ReduceScanArgs
    };

    m_descriptor_sets.constant_layout = m_app->m_device.createDescriptorSetLayout({{}, constant_bindings});
    m_descriptor_sets.indirect_constant_layout = m_app->m_device.createDescriptorSetLayout({{}, indirect_constant_bindings});
    m_descriptor_sets.input_output_layout = m_app->m_device.createDescriptorSetLayout({{}, input_output_bindings});
    m_descriptor_sets.scan_layout = m_app->m_device.createDescriptorSetLayout({{}, scan_bindings});
    m_descriptor_sets.scratch_layout = m_app->m_device.createDescriptorSetLayout({{}, scratch_bindings});
    m_descriptor_sets.indirect_layout = m_app->m_device.createDescriptorSetLayout({{}, indirect_bindings});

    auto constant_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.constant_layout);
    auto indirect_constant_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.indirect_constant_layout);
    auto input_output_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.input_output_layout);
    auto scan_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.scan_layout);
    auto scratch_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.scratch_layout);
    auto indirect_set_alloc_info = vk::DescriptorSetAllocateInfo(m_descriptor_pool, m_descriptor_sets.indirect_layout);

    vk::Result result;
    result = m_app->m_device.allocateDescriptorSets(&constant_set_alloc_info, &m_descriptor_sets.constant_set);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&indirect_constant_set_alloc_info, &m_descriptor_sets.indirect_constant_set);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&input_output_set_alloc_info, &m_descriptor_sets.input_output_sets[0]);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&input_output_set_alloc_info, &m_descriptor_sets.input_output_sets[1]);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&scan_set_alloc_info, &m_descriptor_sets.scan_sets[0]);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&scan_set_alloc_info, &m_descriptor_sets.scan_sets[1]);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&scratch_set_alloc_info, &m_descriptor_sets.scratch_set);
    assert(result == vk::Result::eSuccess);
    result = m_app->m_device.allocateDescriptorSets(&indirect_set_alloc_info, &m_descriptor_sets.indirect_set);
    assert(result == vk::Result::eSuccess);
}

void GPURadixSorter::createDescriptorPool() {
    SimulatorComputeStage::createDescriptorPool(15, {
            {vk::DescriptorType::eStorageBuffer, 50},
            {vk::DescriptorType::eUniformBuffer, 10}
    });
}

void GPURadixSorter::bindConstantBuffer(vk::DescriptorBufferInfo &const_buf, vk::DescriptorSet& descriptor_set, u32 binding, u32 count) {
    vk::WriteDescriptorSet descriptor_write(descriptor_set, binding, 0, vk::DescriptorType::eUniformBuffer, {}, const_buf, {});
    m_app->m_device.updateDescriptorSets(descriptor_write, {});
}

void GPURadixSorter::bindBuffers(vk::Buffer *buffers, vk::DescriptorSet &descriptor_set, u32 binding, u32 count) {
    std::vector<vk::DescriptorBufferInfo> buffer_infos;
    for (u32 i = 0; i < count; i++) {
        vk::DescriptorBufferInfo buffer_info(buffers[i], 0, VK_WHOLE_SIZE);
        buffer_infos.push_back(buffer_info);
    }

    vk::WriteDescriptorSet descriptor_write(descriptor_set, binding, 0, vk::DescriptorType::eStorageBuffer, {}, buffer_infos, {});
    m_app->m_device.updateDescriptorSets(descriptor_write, {});
}

void GPURadixSorter::bindStaticBufferDescriptors() {
    vk::Buffer buffers[3];

    buffers[0] = buffers[1] = m_reduced_scratch_buffer.get();
    bindBuffers(buffers, m_descriptor_sets.scan_sets[0], 0, 2);

    buffers[0] = buffers[1] = m_scratch_buffer.get();
    buffers[2] = m_reduced_scratch_buffer.get();
    bindBuffers(buffers, m_descriptor_sets.scan_sets[1], 0, 3);

    buffers[0] = m_scratch_buffer.get();
    buffers[1] = m_reduced_scratch_buffer.get();
    bindBuffers(buffers, m_descriptor_sets.scratch_set, 0, 2);
}

void GPURadixSorter::bindInputOutputBuffers(vk::Buffer& key_buf, vk::Buffer& key_ping_pong_buf, vk::Buffer& payload_buf, vk::Buffer& payload_ping_pong_buf) {
    vk::Buffer buffers[4];

    buffers[0] = key_buf;
    buffers[1] = key_ping_pong_buf;
    buffers[2] = payload_buf;
    buffers[3] = payload_ping_pong_buf;
    bindBuffers(buffers, m_descriptor_sets.input_output_sets[0], 0, 4);

    buffers[0] = key_ping_pong_buf;
    buffers[1] = key_buf;
    buffers[2] = payload_ping_pong_buf;
    buffers[3] = payload_buf;
    bindBuffers(buffers, m_descriptor_sets.input_output_sets[1], 0, 4);
}

void GPURadixSorter::setMaxKeys(u32 max_keys) {
    FFX_ParallelSort_CalculateScratchResourceSize(max_keys, m_scratch_buffer_size, m_reduced_scratch_buffer_size);

    allocateScratchBuffers(m_scratch_buffer_size, m_reduced_scratch_buffer_size);
    bindStaticBufferDescriptors();
}

std::array<vk::DescriptorSetLayout, 6> GPURadixSorterDescriptorSets::get_layouts() {
    return std::array<vk::DescriptorSetLayout, 6> {
        constant_layout, indirect_constant_layout, input_output_layout, scan_layout, scratch_layout, indirect_layout
    };
}
