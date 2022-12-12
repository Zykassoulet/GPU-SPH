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



VulkanBuffer::VulkanBuffer(VmaAllocator &allocator, vk::BufferCreateInfo &create_info, VmaAllocationCreateInfo alloc_info = {}) {
    init(allocator, create_info, alloc_info);
}

VulkanBuffer::VulkanBuffer(VmaAllocator& allocator, size_t alloc_size,
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer,
    VmaAllocationCreateFlags memoryFlag = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) {

    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = alloc_size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaallocInfo.flags = memoryFlag;
    VulkanBuffer(allocator, bufferInfo, vmaallocInfo);

    init(allocator, bufferInfo, vmaallocInfo);
}

void VulkanBuffer::init(VmaAllocator& allocator, vk::BufferCreateInfo& create_info, VmaAllocationCreateInfo alloc_info = {}) {

    if (alloc_info.usage == VMA_MEMORY_USAGE_UNKNOWN) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    }

    m_allocator = allocator;
    auto result = vmaCreateBuffer(m_allocator, &static_cast<VkBufferCreateInfo&>(create_info), &alloc_info, &reinterpret_cast<VkBuffer&>(m_buffer), &m_allocation, nullptr);
    assert(result == VK_SUCCESS);

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

template<typename T>
void VulkanBuffer::load_data(std::vector<T> data_vec) { //IS IT CLEAN?
    void* data;
    vmaMapMemory(m_allocator, m_allocation, &data);

    memcpy(data, data_vec.data(), data_vec.size() * sizeof(T));

    vmaUnmapMemory(m_allocator, m_allocation);
}

vk::DescriptorSetLayoutBinding createDescriptorSetLayoutBinding(u32 binding, u32 count = 1, 
    vk::DescriptorType type = vk::DescriptorType::eStorageBuffer, 
    vk::ShaderStageFlagBits shader_stage = vk::ShaderStageFlagBits::eCompute) {
        vk::DescriptorSetLayoutBinding layout_binding = {};
        layout_binding.binding = binding;
        layout_binding.descriptorCount = count;
        layout_binding.descriptorType = type;
        layout_binding.stageFlags = shader_stage;
        return layout_binding;
}
