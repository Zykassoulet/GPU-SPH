
#include "vkinit.h"



vk::BufferCreateInfo VKInit::createBuffer(const u32 buffer_size, const u32 family_index) {
    vk::BufferCreateInfo BufferCreateInfo{
    vk::BufferCreateFlags(),                    // Flags
    buffer_size,                                 // Size
    vk::BufferUsageFlagBits::eStorageBuffer,    // Usage
    vk::SharingMode::eExclusive,                // Sharing mode
    1,                                          // Number of queue family indices
    &family_index                  // List of queue family indices
    };
    return BufferCreateInfo;
}