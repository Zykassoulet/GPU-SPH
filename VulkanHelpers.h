#pragma once

#include "VulkanInclude.h"
#include <spirv_reflect.h>
#include <cassert>

#include <GLFW/glfw3.h>

#include "utils.h"

class VulkanBuffer {
public:
    VulkanBuffer() = default;
    VulkanBuffer(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, size_t object_size, size_t object_count, VmaAllocationCreateInfo alloc_info = {});


    VulkanBuffer(const VulkanBuffer& other) = delete; // Disallow copying
    VulkanBuffer& operator=(const VulkanBuffer& other) = delete; // Disallow copying

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    ~VulkanBuffer();

    void init(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, VmaAllocationCreateInfo alloc_info = {});

    vk::Buffer& get();
    size_t get_size();

    vk::DescriptorBufferInfo getDescriptorBufferInfo();

    template<typename T>
    void store_data(T* data, u32 count) {

        assert(sizeof(T) == object_size && count <= object_count);
        void* mapped;
        vmaMapMemory(m_allocator, m_allocation, &mapped);

        memcpy(mapped, data, count * sizeof(T));

        vmaUnmapMemory(m_allocator, m_allocation);
    }

    template<typename T>
    void load_data(T* data, u32 count) {

        assert(sizeof(T) == object_size && count <= object_count);
        void* mapped;
        vmaMapMemory(m_allocator, m_allocation, &mapped);

        memcpy(data, mapped, count * sizeof(T));

        vmaUnmapMemory(m_allocator, m_allocation);
    }


private:
    vk::Buffer m_buffer;
    VmaAllocation m_allocation;
    VmaAllocator m_allocator;
    size_t object_size;
    size_t object_count;
};

vk::ShaderModule createShaderModuleFromFile(vk::Device& device, const std::string &file_name);


vk::BufferMemoryBarrier
bufferTransition(vk::Buffer buffer, vk::AccessFlags before, vk::AccessFlags after, u32 size);