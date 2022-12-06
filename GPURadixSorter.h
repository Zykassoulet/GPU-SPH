#pragma once

#define FFX_CPP
#include <cstdint>
#include <FFX_ParallelSort.h>
#include "VulkanHelpers.h"
#include "App.h"
#include <tuple>

struct GPURadixSorterPipelines {
    vk::Pipeline count;
    vk::Pipeline count_reduce;
    vk::Pipeline prefix_scan;
    vk::Pipeline scan_add;
    vk::Pipeline scatter;
};

struct GPURadixSorterDescriptorSets {
    vk::DescriptorSetLayout constant_layout;
    vk::DescriptorSet constant_set;
    vk::DescriptorSetLayout indirect_constant_layout;
    vk::DescriptorSet indirect_constant_set;
    vk::DescriptorSetLayout input_output_layout;
    vk::DescriptorSet input_output_sets[2];
    vk::DescriptorSetLayout scan_layout;
    vk::DescriptorSet scan_sets[2];
    vk::DescriptorSetLayout scratch_layout;
    vk::DescriptorSet scratch_set;
    vk::DescriptorSetLayout indirect_layout;
    vk::DescriptorSet indirect_set;

    std::array<vk::DescriptorSetLayout, 6> get_layouts();
};

class GPURadixSorter {
public:
    GPURadixSorter(App& app);
    std::tuple<VulkanBuffer&, VulkanBuffer&> sort(vk::CommandBuffer cmd_buf, u32 num_keys, VulkanBuffer &key_buf, VulkanBuffer &key_ping_pong_buf, VulkanBuffer &value_buf, VulkanBuffer& value_ping_pong_buf);

    ~GPURadixSorter();
    GPURadixSorter(const GPURadixSorter&) = delete;
    GPURadixSorter& operator=(const GPURadixSorter&) = delete;
private:

    void allocateScratchBuffers(u32 scratch_buffer_size, u32 reduced_scratch_buffer_size);
    vk::PipelineLayout createSortPipelineLayout();
    void createDescriptorSets();
    vk::Pipeline createRadixPipeline(std::string shader_file, vk::PipelineLayout& layout);
    void createPipelines();
    void createDescriptorPool();


    App* m_app;
    VulkanBuffer m_scratch_buffer;
    VulkanBuffer m_reduced_scratch_buffer;
    vk::PipelineLayout m_pipeline_layout;
    GPURadixSorterPipelines m_pipelines;
    vk::DescriptorPool m_descriptor_pool;
    GPURadixSorterDescriptorSets m_descriptor_sets;
};

