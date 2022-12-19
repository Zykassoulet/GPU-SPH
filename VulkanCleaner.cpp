//
// Created by Moritz Kuntze on 12/12/2022.
//

#include "VulkanCleaner.h"

#include <ranges>

VulkanCleaner::~VulkanCleaner() {
    for (int i = m_deletion_queue.size() - 1; i >= 0; i--) {
        m_deletion_queue[i](m_vk_context);
    }
}

void VulkanCleaner::deferDelete(std::function<void(std::shared_ptr<VulkanContext>)> deleter) {
    m_deletion_queue.push_back(deleter);
}
