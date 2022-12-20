#pragma once

#include "utils.h"
#include "VulkanContext.h"
#include "SimulatorComputeStage.h"
#include "Mesh.h"
#include "SimulationParams.h"

#include <vector>
#include "glm/glm.hpp"


struct InstanceRendererDescriptorSets {
    vk::DescriptorSetLayout layout;
    vk::DescriptorSet set;
};

struct InstanceRendererPushConstants {
    f32 particle_radius;
};


class InstanceRenderer : public SimulatorComputeStage {
public:
    InstanceRenderer(std::shared_ptr<VulkanContext> vulkan_context);


    void render(SimulationParams simulation_params, VulkanBuffer& position_buffer);


private:

    void createPipelineLayouts();
    void createPipelines() final;
    void createDescriptorSets() final;
    void createDescriptorPool() final;
    void createRenderPass();
    void createFrameBuffers();
    void createSyncStructures();
    void createMesh();
    vk::PipelineRasterizationStateCreateInfo createRasterStateInfo();
    vk::PipelineMultisampleStateCreateInfo createMSStateInfo();

    void writeBufferDescriptorSets(VulkanBuffer& position_buffer);

    InstanceRendererDescriptorSets descriptor_sets;
    vk::RenderPass render_pass;
	vk::PipelineLayout pipeline_layout;
	vk::Pipeline pipeline;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Fence finish_fence;
    vk::Semaphore present_semaphore;
    vk::Semaphore render_semaphore;
    Mesh triangle;

};



