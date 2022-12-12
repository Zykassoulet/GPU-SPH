#include "SimulatorComputeStage.h"

vk::UniqueCommandBuffer SimulatorComputeStage::createCommandBuffer() {
    vk::CommandBufferAllocateInfo alloc_info(m_vk_context->m_compute_command_pool, vk::CommandBufferLevel::eSecondary, 1);
    auto cmd_buf = std::move(m_vk_context->m_device.allocateCommandBuffersUnique(alloc_info).front());
    auto inheritance_info = vk::CommandBufferInheritanceInfo();
    cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &inheritance_info));
    return std::move(cmd_buf);
}

void SimulatorComputeStage::createDescriptorPool(u32 max_sets, std::map<vk::DescriptorType, u32> counts) {
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    pool_sizes.resize(counts.size());

    std::transform(counts.begin(), counts.end(), pool_sizes.begin(), [](std::pair<vk::DescriptorType, u32> v){
        auto [descriptor_type, count] = v;
        return vk::DescriptorPoolSize(descriptor_type, count);
    });

    m_descriptor_pool = m_vk_context->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo(
            {},
            max_sets,
            pool_sizes
    ));
    deferDelete([pool = m_descriptor_pool](auto vk_context) {
        vk_context->m_device.resetDescriptorPool(pool);
        vk_context->m_device.destroyDescriptorPool(pool);
    });
}