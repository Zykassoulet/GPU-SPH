#pragma once

#include "utils.h"
#include <vulkan/vulkan.hpp>


class VKInit {

public:
	vk::BufferCreateInfo createBuffer(const u32 buffer_size, const u32 family_index);

};
