#include "PhysicsEngine.h"



PhysicsEngine::PhysicsEngine() {
	m_number_particles = 10;
	for (int i = 0; i < m_number_particles; i++) {
		m_particles.push_back(glm::linearRand<glm::vec3>(glm::vec3(0),glm::vec3(1)));
	}
}