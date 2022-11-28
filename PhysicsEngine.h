#pragma once


#include "utils.h"

class PhysicsEngine {

public:
	PhysicsEngine();


private:
	//parameters
	std::vector<glm::vec3> m_particles;
	u32 m_number_particles;
};