#pragma once

#include "PhysicsEngineStructs.h"
#include "GPURadixSorter.h"
#include <array>



class App;

class PhysicsEngine {

public:
	PhysicsEngine(std::shared_ptr<App> app_ptr);
	~PhysicsEngine();

	void initSimulationParameters();
	void initBuffers();
	void initDescriptors();
	void initPipelines();

	void initInputPosBuffer();
	void initPosBuffer();
	void initZIndexBuffer();
	void initVelBuffer();
	void initDensBuffer();
	void initBlocksDataBuffer();



private:
	

	SimulationParameters sim_params;
	PhysicsEngineBuffers buffers;
	PhysicsEngineDescriptorSetLayouts layouts;
	PhysicsEnginePipelines pipelines;
	vk::DescriptorPool descriptor_pool;

	std::shared_ptr<GPURadixSorter> gpu_radix_sorter_ptr;
	std::weak_ptr<App> app_wptr;

	

};