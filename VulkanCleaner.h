#pragma once

#include <vector>
#include <functional>
#include <memory>
#include "VulkanContext.h"

class VulkanCleaner {
public:
    explicit VulkanCleaner(std::shared_ptr<VulkanContext> vulkan_context) : m_vk_context(vulkan_context) {}
    ~VulkanCleaner();
    VulkanCleaner(const VulkanCleaner&) = delete;
    VulkanCleaner& operator=(const VulkanCleaner&) = delete;

    void deferDelete(std::function<void(std::shared_ptr<VulkanContext>)> deleter);
protected:
    std::shared_ptr<VulkanContext> m_vk_context;

private:
    std::vector<std::function<void(std::shared_ptr<VulkanContext>)>> m_deletion_queue;
};




