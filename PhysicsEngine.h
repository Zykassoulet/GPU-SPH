#pragma once

#include "utils.h"
#include "ComputePipeline.h"
#include "glm/glm.hpp"

class PhysicsEngine {

public:
	PhysicsEngine();

	void initComputePipeline(App* app);


private:
	//parameters
	u32 m_number_particles;
	std::vector<glm::vec3> m_particles;
	ComputePipeline m_compute_pipeline;

	friend class ComputePipeline;
};