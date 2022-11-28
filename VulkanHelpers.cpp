//
// Created by Moritz Kuntze on 28/11/2022.
//

#include "VulkanHelpers.h"
#include "utils.h"
#include "spirv_reflect.h"
#include <fstream>
#include <iterator>
#include <tuple>

ShaderModule createShaderModuleFromFile(vk::Device& device, const std::string &file_name) {
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

    spv_reflect::ShaderModule reflection(shader_code.size(), reinterpret_cast<u8*>(shader_code.data()));

   return {shader_module, std::move(reflection)};
}

vk::Pipeline createPipeline(vk::Device& device, vk::PipelineLayout& layout) {
    //device.createComputePipeline()
}

VulkanBuffer::VulkanBuffer(VmaAllocator &allocator, vk::BufferCreateInfo &create_info,
                           VmaAllocationCreateInfo alloc_info) {

    if (alloc_info.usage == VMA_MEMORY_USAGE_UNKNOWN) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    }

    m_allocator = allocator;
}

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept : m_allocation(std::exchange(other.m_allocation, nullptr)), m_buffer(other.m_buffer), m_allocator(other.m_allocator) {}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept {
    std::swap(m_allocation, other.m_allocation);
    m_buffer = other.m_buffer;
    m_allocator = other.m_allocator;

    return *this;
}

VulkanBuffer::~VulkanBuffer() {
    if (m_allocation != nullptr) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
}

vk::Buffer& VulkanBuffer::get() {
    return m_buffer;
}
