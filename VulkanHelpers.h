#pragma once

#include "VulkanInclude.h"
#include <spirv_reflect.h>

#include <GLFW/glfw3.h>

#include "utils.h"

class VulkanBuffer {
public:
    VulkanBuffer() = default;
    VulkanBuffer(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, VmaAllocationCreateInfo alloc_info = {});
    VulkanBuffer(VmaAllocator& allocator, size_t alloc_size,
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer,
        VmaAllocationCreateFlags memory_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

    VulkanBuffer(const VulkanBuffer& other) = delete; // Disallow copying
    VulkanBuffer& operator=(const VulkanBuffer& other) = delete; // Disallow copying

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    ~VulkanBuffer();

    void init(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, VmaAllocationCreateInfo alloc_info = {});

    vk::Buffer& get();

    template<typename T>
    void load_data(std::vector<T> data_vec);


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
vk::DescriptorSetLayoutBinding createDescriptorSetLayoutBinding(u32 binding, u32 count = 1,
    vk::DescriptorType type = vk::DescriptorType::eStorageBuffer,
    vk::ShaderStageFlagBits shader_stage = vk::ShaderStageFlagBits::eCompute);