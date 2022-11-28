#include "GPURadixSorter.h"

std::tuple<VulkanBuffer&, VulkanBuffer&> GPURadixSorter::sort(
        vk::CommandBuffer cmd_buf,
        VulkanBuffer &key_buf,
        VulkanBuffer &key_ping_pong_buf,
        VulkanBuffer &value_buf) {
    return {key_buf, value_buf};
}

GPURadixSorter::GPURadixSorter(App& app) {
    m_app = &app;
    u32 scratch_buffer_size, reduced_scrath_buffer_size;
    // FIXME: Make max_num_keys dynamic and re-allocate scratch buffers if necessary
    FFX_ParallelSort_CalculateScratchResourceSize(1024, scratch_buffer_size, reduced_scrath_buffer_size);

    allocateScratchBuffers(scratch_buffer_size, reduced_scrath_buffer_size);
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
