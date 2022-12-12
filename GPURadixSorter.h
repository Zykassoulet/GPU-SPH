#pragma once

#include <cstdint>
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
    GPURadixSorter(App& app, u32 max_keys);
    vk::CommandBuffer sort(u32 num_keys, VulkanBuffer &key_buf, VulkanBuffer &key_ping_pong_buf, VulkanBuffer &value_buf, VulkanBuffer& value_ping_pong_buf);

    void setMaxKeys(u32 max_keys);

    ~GPURadixSorter();
    GPURadixSorter(const GPURadixSorter&) = delete;
    GPURadixSorter& operator=(const GPURadixSorter&) = delete;
private:

    void allocateScratchBuffers(u32 scratch_buffer_size, u32 reduced_scratch_buffer_size);
    vk::PipelineLayout createSortPipelineLayout();
    void createDescriptorSets();
    vk::Pipeline createRadixPipeline(std::string shader_file, std::string entry_point, vk::PipelineLayout& layout);
    void createPipelines();
    void createDescriptorPool();
    vk::CommandBuffer createCommandBuffer();
    void bindConstantBuffer(vk::DescriptorBufferInfo& const_buf, vk::DescriptorSet& descriptor_set, u32 binding, u32 count);
    void bindInputOutputBuffers(vk::Buffer& key_buf, vk::Buffer& key_ping_pong_buf, vk::Buffer& payload_buf, vk::Buffer& payload_ping_pong_buf);
    void bindStaticBufferDescriptors();
    void bindBuffers(vk::Buffer* buffers, vk::DescriptorSet& descriptor_set, u32 binding, u32 count);
    vk::BufferMemoryBarrier bufferTransition(vk::Buffer buffer, vk::AccessFlags before, vk::AccessFlags after, u32 size);

    template<typename T>
    std::tuple<VulkanBuffer, vk::DescriptorBufferInfo> allocConstBuffer(T& data) {
        VulkanBuffer buffer = m_app->createCPUAccessibleBuffer(sizeof(T), vk::BufferUsageFlagBits::eUniformBuffer);

        buffer.store_data(&data, 1);

        vk::DescriptorBufferInfo buffer_info(buffer.get(), 0, (u32) sizeof(T));

        return std::make_tuple(std::move(buffer), buffer_info);
    }

    App* m_app;
    VulkanBuffer m_scratch_buffer;
    u32 m_scratch_buffer_size;
    VulkanBuffer m_reduced_scratch_buffer;
    u32 m_reduced_scratch_buffer_size;
    VulkanBuffer m_constant_buffer;
    vk::PipelineLayout m_pipeline_layout;
    GPURadixSorterPipelines m_pipelines;
    vk::DescriptorPool m_descriptor_pool;
    GPURadixSorterDescriptorSets m_descriptor_sets;
};

