#pragma once

#define FFX_CPP
#include <cstdint>
#include <FFX_ParallelSort.h>
#include "VulkanHelpers.h"
#include "App.h"
#include <tuple>

class GPURadixSorter {
public:
    GPURadixSorter(App& app);
    std::tuple<VulkanBuffer&, VulkanBuffer&> sort(vk::CommandBuffer cmd_buf, VulkanBuffer &key_buf, VulkanBuffer &key_ping_pong_buf, VulkanBuffer &value_buf);

private:

    void allocateScratchBuffers(u32 scratch_buffer_size, u32 reduced_scratch_buffer_size);

    App* m_app;
    VulkanBuffer m_scratch_buffer;
    VulkanBuffer m_reduced_scratch_buffer;
};
