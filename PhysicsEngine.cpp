#include "PhysicsEngine.h"

PhysicsEngine::PhysicsEngine() {
	m_number_particles = 10;
}

void PhysicsEngine::initComputePipeline(App* app_ptr) {
	m_compute_pipeline.init(app_ptr, this);

	//TODO : other things?
}