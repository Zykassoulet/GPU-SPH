#include "VulkanHelpers.h"
#include "utils.h"
#include "spirv_reflect.h"
#include <fstream>
#include <iterator>
#include <tuple>

vk::ShaderModule createShaderModuleFromFile(vk::Device& device, const std::string &file_name) {
    std::ifstream ifs(file_name, std::ios::ate | std::ios::binary);

    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + file_name);
    }

    usize file_size = (usize) ifs.tellg();
    std::vector<char> shader_code(file_size);
    ifs.seekg(0);
    ifs.read(shader_code.data(), file_size);
    ifs.close();

    auto create_info = vk::ShaderModuleCreateInfo({}, file_size, reinterpret_cast<const u32*>(shader_code.data()));
    vk::ShaderModule shader_module = device.createShaderModule(create_info);
    return shader_module;
}



VulkanBuffer::VulkanBuffer(VmaAllocator &allocator, vk::BufferCreateInfo &create_info, size_t object_s, size_t object_c, VmaAllocationCreateInfo alloc_info) {
    object_count = object_c;
    object_size = object_s;
    init(allocator, create_info, alloc_info);
}

void VulkanBuffer::init(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, VmaAllocationCreateInfo alloc_info) {

    if (alloc_info.usage == VMA_MEMORY_USAGE_UNKNOWN) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    }

    m_allocator = allocator;
    auto result = vmaCreateBuffer(m_allocator, &static_cast<VkBufferCreateInfo&>(create_info), &alloc_info, &reinterpret_cast<VkBuffer&>(m_buffer), &m_allocation, nullptr);
    assert(result == VK_SUCCESS);
}

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
    : m_allocation(std::exchange(other.m_allocation, nullptr)),
    m_buffer(other.m_buffer),
    m_allocator(other.m_allocator),
    object_size(other.object_size),
    object_count(other.object_count) {}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept {
    m_allocation = nullptr;
    std::swap(m_allocation, other.m_allocation);
    m_buffer = other.m_buffer;
    m_allocator = other.m_allocator;
    object_size = other.object_size;
    object_count = other.object_count;

    return *this;
}

VulkanBuffer::~VulkanBuffer() {
    if (m_allocation != NULL) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
}

vk::Buffer& VulkanBuffer::get() {
    return m_buffer;
}

size_t VulkanBuffer::get_size() {
    return object_count * object_size;
}

vk::DescriptorBufferInfo VulkanBuffer::getDescriptorBufferInfo() {
    return vk::DescriptorBufferInfo(m_buffer, 0, object_count * object_size);
}

VulkanBuffer::VulkanBuffer(VmaAllocator &allocator, vk::BufferCreateInfo &create_info,
                           VmaAllocationCreateInfo alloc_info)
   : VulkanBuffer(allocator, create_info, 1, (size_t) create_info.size, alloc_info) {}


vk::BufferMemoryBarrier bufferTransition(vk::Buffer buffer, vk::AccessFlags before, vk::AccessFlags after, u32 size) {
    return {
            before, after, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, 0, size
    };
}

vk::BufferMemoryBarrier generalReadWriteBarrier(VulkanBuffer& buffer) {
    return bufferTransition(buffer.get(),
                            vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
                            vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
                            buffer.get_size());
}

vk::ImageCreateInfo VulkanImage::create_info(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent) {
    return {{}, vk::ImageType::e2D, format, extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage};
}


vk::ImageViewCreateInfo VulkanImage::view_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect_flags) {
    return {{}, image, vk::ImageViewType::e2D, format, {}, {aspect_flags, 0, 1, 0, 1}};
}
