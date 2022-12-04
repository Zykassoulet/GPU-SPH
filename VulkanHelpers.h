#pragma once

#include "VulkanInclude.h"
#include <spirv_reflect.h>

#include <GLFW/glfw3.h>

#include "utils.h"

class VulkanBuffer {
public:
    VulkanBuffer() = default;
    VulkanBuffer(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, VmaAllocationCreateInfo alloc_info = {});

    VulkanBuffer(const VulkanBuffer& other) = delete; // Disallow copying
    VulkanBuffer& operator=(const VulkanBuffer& other) = delete; // Disallow copying

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    ~VulkanBuffer();

    vk::Buffer& get();

private:
    vk::Buffer m_buffer;
    VmaAllocation m_allocation;
    VmaAllocator m_allocator;
};

struct ShaderModule {
    vk::ShaderModule vulkan;
    spv_reflect::ShaderModule reflection;
};

ShaderModule createShaderModuleFromFile(vk::Device& device, const std::string &file_name);