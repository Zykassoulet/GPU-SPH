#pragma once

#include "utils.h"
#include "VulkanInclude.h"
#include "VulkanHelpers.h"
#include "glm/glm.hpp"

class App;
class PhysicsEngine;

class ComputePipeline {
public:
	void init(App* app_ptr, PhysicsEngine* phy_eng_ptr);

private:
	 App* m_app_ptr;
	 PhysicsEngine* m_phy_eng_ptr;
	 VulkanBuffer m_pos_buf;

};
