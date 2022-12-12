#pragma once

#include "utils.h"
#include "App.h"
#include "particle.h"


#include <vector>

struct ZIndexLookupBuffers {
    VulkanBuffer x_lookup;
    VulkanBuffer y_lookup;
    VulkanBuffer z_lookup;
};

struct ZIndexerDescriptorSets {
    vk::DescriptorSetLayout particle_buffers_layout;
    vk::DescriptorSet particle_buffers;
    vk::DescriptorSetLayout interleave_buffers_layout;
    vk::DescriptorSet interleave_buffers;
};

class ZIndexer {
public:
    ZIndexer(App* app, u32 grid_x, u32 grid_y, u32 grid_z, f32 grid_unit_size);

    vk::CommandBuffer generateZIndices(VulkanBuffer particles, u32 num_particles, VulkanBuffer z_index_buffer, VulkanBuffer particle_index_buffer);

private:
    static inline u32 makeInterleavable(u32 value, u32 spacing, u32 offset) {
        u32 out = 0;
        u32 bit_position = 1 << offset;
        for (; value > 0; value >>= 1) {
            out |= (value & 1) * bit_position;
            bit_position <<= spacing;
        }
        return out;
    }

    std::vector<u32> generateInterleavableX();
    std::vector<u32> generateInterleavableY();
    std::vector<u32> generateInterleavableZ();

    void createLookupBuffers();

    void createPipeline();
    void createDescriptorSets();
    void createDescriptorPool();
    void writeInterleaveBuffersDescriptorSet();

    void writeParticleBuffersDescriptorSet(VulkanBuffer& particle_buffer, u32 num_particles, VulkanBuffer& particle_index_buffer, VulkanBuffer& z_index_buffer);


    App* m_app;
    u32 m_grid_x;
    u32 m_grid_y;
    u32 m_grid_z;
    f32 m_grid_unit_size;
    ZIndexLookupBuffers m_lookup_buffers;
    vk::PipelineLayout m_pipeline_layout;
    vk::Pipeline m_pipeline;
    ZIndexerDescriptorSets m_descriptor_sets;
    vk::DescriptorPool m_descriptor_pool;

    void createPipelineLayout();

    vk::CommandBuffer createCommandBuffer();

    vk::BufferMemoryBarrier
    bufferTransition(vk::Buffer buffer, vk::AccessFlags before, vk::AccessFlags after, u32 size);
};




