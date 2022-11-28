#pragma once


#include "utils.h"
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include "ComputePipeline.h"

class PhysicsEngine {

public:
	PhysicsEngine();


private:
	//parameters
	ComputePipeline m_compute_pipeline;
	std::vector<glm::vec3> m_particles;
	u32 m_number_particles;
};