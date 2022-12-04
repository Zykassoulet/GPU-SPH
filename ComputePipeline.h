#pragma once

#include "utils.h"
#include "VulkanInclude.h"
#include "VulkanHelpers.h"

class ComputePipeline {
public:
	void init(App* app_ptr,PhysicsEngine* phy_eng_ptr);

private:
	 App* m_app_ptr;
	 PhysicsEngine* m_phy_eng_ptr;

};
