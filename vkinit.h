#pragma once

#include "utils.h"
#include <vulkan/vulkan.hpp>


class VKInit {

public:
	vk::BufferCreateInfo createBufferInfo(const u32 buffer_size, const u32 family_index);

};
