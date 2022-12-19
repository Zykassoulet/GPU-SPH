#pragma once


#include "SimulatorComputeStage.h"


class Renderer {
public:

	Renderer(std::shared_ptr<VulkanContext> vulkan_context);


	vk::UniqueCommandBuffer renderScene(VulkanBuffer& position_buffer);


private:

	void createDescriptions();
	void createPipelineLayout();
	void createPipeline();

	vk::VertexInputBindingDescription binding_description;
	vk::VertexInputAttributeDescription attribute_description;
	vk::Pipeline pipeline;
	vk::PipelineLayout pipeline_layout;
	std::shared_ptr<VulkanContext> vk_context;

};
