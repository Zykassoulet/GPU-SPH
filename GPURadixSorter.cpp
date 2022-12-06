#include "GPURadixSorter.h"

std::tuple<VulkanBuffer&, VulkanBuffer&> GPURadixSorter::sort(
        vk::CommandBuffer cmd_buf,
        u32 num_keys,
        VulkanBuffer& key_buf,
        VulkanBuffer& key_ping_pong_buf,
        VulkanBuffer& value_buf,
        VulkanBuffer& value_ping_pong_buf) {

    FFX_ParallelSortCB constantBufferData = {0};
    u32 num_threadgroups_to_run, num_reduced_threadgroups_to_run;

    FFX_ParallelSort_SetConstantAndDispatchData(num_keys, 800, constantBufferData, num_threadgroups_to_run,
                                                num_reduced_threadgroups_to_run);

}

GPURadixSorter::GPURadixSorter(App& app) {
    m_app = &app;
    u32 scratch_buffer_size, reduced_scrath_buffer_size;
    // FIXME: Make max_num_keys dynamic and re-allocate scratch buffers if necessary
    FFX_ParallelSort_CalculateScratchResourceSize(1024, scratch_buffer_size, reduced_scrath_buffer_size);

    allocateScratchBuffers(scratch_buffer_size, reduced_scrath_buffer_size);
    createDescriptorPool();
    createDescriptorSets();
    createPipelines();
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
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_Count.spv", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_CountReduce.spv", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_Scan.spv", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_ScanAdd.spv", layout),
            createRadixPipeline("shaders/build/ParallelSortCS_FPS_Scatter.spv", layout)
    };
}

vk::Pipeline GPURadixSorter::createRadixPipeline(std::string shader_file, vk::PipelineLayout& layout) {
    auto shader_module = createShaderModuleFromFile(m_app->m_device, shader_file);
    auto shader_state_create_info = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eCompute, shader_module.vulkan, shader_file.data(), {});
    auto create_info = vk::ComputePipelineCreateInfo({}, shader_state_create_info, layout, {}, {});
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
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll, {}) // Constant buffer table
    };

    auto indirect_constant_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll, {}) // Constant buffer table
    };

    auto input_output_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // SrcBuffer
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // DstBuffer
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // SrcPayload
        vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}) // DstPayload
    };

    auto scan_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // ScanSrc
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // ScanDst
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}) // ScanScratch
    };

    auto scratch_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // Scratch
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}) // Scratch (reduced)
    };

    auto indirect_bindings = std::array {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // NumKeys
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // CBufferUAV
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}), // CountScatterArgs
        vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll, {}) // ReduceScanArgs
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
    auto pool_sizes = std::array {
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 50),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 10)
    };

    m_descriptor_pool = m_app->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo(
            {},
            15,
            pool_sizes
    ));
}

GPURadixSorter::~GPURadixSorter() {
    // TODO
}

std::array<vk::DescriptorSetLayout, 6> GPURadixSorterDescriptorSets::get_layouts() {
    return std::array<vk::DescriptorSetLayout, 6> {
        constant_layout, indirect_constant_layout, input_output_layout, scan_layout, scratch_layout, indirect_layout
    };
}
