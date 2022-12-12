//
// Created by Moritz Kuntze on 12/12/2022.
//

#include "VulkanCleaner.h"

#include <ranges>

VulkanCleaner::~VulkanCleaner() {
    for (auto& deleter : std::ranges::reverse_view(m_deletion_queue)) {
        deleter(m_vk_context);
    }
}

void VulkanCleaner::deferDelete(std::function<void(std::shared_ptr<VulkanContext>)> deleter) {
    m_deletion_queue.push_back(deleter);
}
