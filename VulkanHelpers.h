#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <spirv_reflect.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

class VulkanBuffer {
public:
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