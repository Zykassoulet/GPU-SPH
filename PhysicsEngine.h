#pragma once

#include "PhysicsEngineStructs.h"
#include "GPURadixSorter.h"
#include <array>
#include "App.h"

class PhysicsEngine {

public:
	explicit PhysicsEngine(std::shared_ptr<App> app);
    ~PhysicsEngine();

    void step();

private:
    void initSimulationParameters();
    void initBuffers();

    void initInputPosBuffer();
    void initPosBuffer();
    void initZIndexBuffer();
    void initVelBuffer();
    void initDensBuffer();
    void initBlocksDataBuffer();

	SimulationParameters sim_params;
	PhysicsEngineBuffers buffers;
	PhysicsEngineDescriptorSetLayouts layouts;
	PhysicsEnginePipelines pipelines;
	vk::DescriptorPool descriptor_pool;

	GPURadixSorter m_radix_sorter;
	std::shared_ptr<App> m_app;

	

};