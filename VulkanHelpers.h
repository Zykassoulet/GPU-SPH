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
    void store_data(T* data, u32 count) {
        void* mapped;
        vmaMapMemory(m_allocator, m_allocation, &mapped);

        memcpy(mapped, data, count * sizeof(T));

        vmaUnmapMemory(m_allocator, m_allocation);
    }

    template<typename T>
    void load_data(T* data, u32 count) {
        void* mapped;
        vmaMapMemory(m_allocator, m_allocation, &mapped);

        memcpy(data, mapped, count * sizeof(T));

        vmaUnmapMemory(m_allocator, m_allocation);
    }


private:
    vk::Buffer m_buffer;
    VmaAllocation m_allocation;
    VmaAllocator m_allocator;
};

vk::ShaderModule createShaderModuleFromFile(vk::Device& device, const std::string &file_name);

vk::DescriptorSetLayoutBinding createDescriptorSetLayoutBinding(u32 binding, u32 count = 1,
    vk::DescriptorType type = vk::DescriptorType::eStorageBuffer,
    vk::ShaderStageFlagBits shader_stage = vk::ShaderStageFlagBits::eCompute);

vk::BufferMemoryBarrier
bufferTransition(vk::Buffer buffer, vk::AccessFlags before, vk::AccessFlags after, u32 size);