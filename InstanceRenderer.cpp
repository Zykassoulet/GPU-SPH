#include "InstanceRenderer.h"
#include <array>
#include "glm/glm.hpp"
#include <array>
#include <iostream>



InstanceRenderer::InstanceRenderer(std::shared_ptr<VulkanContext> vulkan_context) : SimulatorComputeStage(vulkan_context) {
    createDescriptorPool();
    createDescriptorSets();
    createPipelineLayouts();
    createRenderPass();
    createFrameBuffers();
    createPipelines();
    createSyncStructures();
    createMesh();
}


void InstanceRenderer::createDescriptorPool() {
    SimulatorComputeStage::createDescriptorPool(10, {
        { vk::DescriptorType::eStorageBuffer,10 }
        });
}

void InstanceRenderer::createDescriptorSets() {

    auto position_buffer_binding = std::array{
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eVertex,{})
    };

    vk::DescriptorSetLayoutCreateInfo position_buffer_layout_create_info({}, position_buffer_binding);
    descriptor_sets.layout = m_vk_context->m_device.createDescriptorSetLayout(position_buffer_layout_create_info);
    deferDelete([descriptor_set_layout = descriptor_sets.layout](auto m_vk_context) {
        m_vk_context->m_device.destroyDescriptorSetLayout(descriptor_set_layout);
    });

    vk::DescriptorSetAllocateInfo position_buffer_set_alloc_info(m_descriptor_pool, descriptor_sets.layout);

    vk::Result result;
    result = m_vk_context->m_device.allocateDescriptorSets(&position_buffer_set_alloc_info, &descriptor_sets.set);
    assert(result == vk::Result::eSuccess);
}

void InstanceRenderer::createPipelineLayouts() {
    vk::PushConstantRange constant_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(InstanceRendererPushConstants));


    vk::PipelineLayoutCreateInfo layout_create_info({}, {}, constant_range);
    pipeline_layout = m_vk_context->m_device.createPipelineLayout(layout_create_info);
    deferDelete([layout = pipeline_layout](auto m_vk_context) {
        m_vk_context->m_device.destroyPipelineLayout(layout);
        });
}

void InstanceRenderer::createRenderPass() {
    vk::AttachmentDescription color_attachment;
    color_attachment.format = m_vk_context->m_swapchain_image_format;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;

    vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    vk::RenderPassCreateInfo render_pass_info({}, color_attachment, subpass);
    render_pass = m_vk_context->m_device.createRenderPass(render_pass_info);

    deferDelete([=](auto m_vk_context) {
        m_vk_context->m_device.destroyRenderPass(render_pass);
    });
}

void InstanceRenderer::createFrameBuffers() {
    vk::FramebufferCreateInfo framebuffer_info({}, render_pass, 1, {}, m_vk_context->m_window_extent.width, m_vk_context->m_window_extent.height, 1);
    u32 swapchain_image_count = m_vk_context->m_swapchain_images.size();

    for (int i = 0; i < swapchain_image_count; i++) {
        framebuffer_info.pAttachments = &m_vk_context->m_swapchain_image_views[i];
        framebuffers.push_back(m_vk_context->m_device.createFramebuffer(framebuffer_info));
        deferDelete([framebuffer= framebuffers[i]](auto m_vk_context) {
            m_vk_context->m_device.destroyFramebuffer(framebuffer);
            });
    }
}

void InstanceRenderer::createPipelines() {

    vk::ShaderModule shader_module_frag = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/instance_renderer_frag.spv");
    vk::PipelineShaderStageCreateInfo shader_stage_frag_create_info({}, vk::ShaderStageFlagBits::eFragment, shader_module_frag, "main", {});
    vk::ShaderModule shader_module_vert = createShaderModuleFromFile(m_vk_context->m_device, "shaders/build/instance_renderer_vert.spv");
    vk::PipelineShaderStageCreateInfo shader_stage_vert_create_info({}, vk::ShaderStageFlagBits::eVertex, shader_module_vert, "main", {});
    auto shader_stages_info = std::array{ shader_stage_vert_create_info, shader_stage_frag_create_info };


    vk::VertexInputBindingDescription binding_description(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    vk::VertexInputBindingDescription particle_pos_binding_description(1, sizeof(glm::vec4), vk::VertexInputRate::eInstance);
    auto attribute_description = Vertex::getVertexInputDescriptions(0);
    attribute_description.push_back(vk::VertexInputAttributeDescription(1, 1, vk::Format::eR32G32B32A32Sfloat, 0));

    auto binding_descriptions = std::array{
        binding_description,
        particle_pos_binding_description
    };
    
    vk::PipelineVertexInputStateCreateInfo vertex_input_state_info({}, binding_descriptions, attribute_description);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_info({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

    vk::PipelineRasterizationStateCreateInfo raster_state_info = createRasterStateInfo();

    vk::PipelineMultisampleStateCreateInfo ms_state_info = createMSStateInfo();

    vk::PipelineColorBlendAttachmentState color_blend_attachment_state;
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.colorWriteMask = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
    vk::PipelineColorBlendStateCreateInfo color_blend_state_info({}, VK_FALSE, vk::LogicOp::eCopy, color_blend_attachment_state, {});
    
    vk::Viewport viewport(0.f, 0.f, m_vk_context->m_window_extent.width, m_vk_context->m_window_extent.height, 0.f, 1.f);
    vk::Rect2D scissors({ 0, 0 }, m_vk_context->m_window_extent);
    vk::PipelineViewportStateCreateInfo viewport_state_info({}, viewport, scissors);

    vk::GraphicsPipelineCreateInfo create_info(
        {},
        shader_stages_info,
        &vertex_input_state_info,
        &input_assembly_state_info,
        {},
        &viewport_state_info,
        &raster_state_info,
        &ms_state_info,
        {},
        &color_blend_state_info,
        {},
        pipeline_layout,
        render_pass,
        0,
        VK_NULL_HANDLE,
        0
    );
    pipeline = m_vk_context->m_device.createGraphicsPipeline({}, create_info).value;
    deferDelete([pipeline = pipeline](auto m_vk_context) {
        m_vk_context->m_device.destroyPipeline(pipeline);
        });
}

vk::PipelineRasterizationStateCreateInfo InstanceRenderer::createRasterStateInfo() {
    return vk::PipelineRasterizationStateCreateInfo(
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0,
        0,
        0,
        1.0f
    );
}

vk::PipelineMultisampleStateCreateInfo InstanceRenderer::createMSStateInfo() {
    return vk::PipelineMultisampleStateCreateInfo(
        {},
        vk::SampleCountFlagBits::e1,
        VK_FALSE,
        1.f,
        nullptr,
        VK_FALSE,
        VK_FALSE
    );
}

void InstanceRenderer::createSyncStructures() {
    finish_fence = m_vk_context->m_device.createFence({});
    present_semaphore = m_vk_context->m_device.createSemaphore({});
    render_semaphore = m_vk_context->m_device.createSemaphore({});
}

void InstanceRenderer::createMesh() {
    triangle = Mesh::triangleMesh(m_vk_context);
}

void InstanceRenderer::render(SimulationParams simulation_params, VulkanBuffer& position_buffer) {

    m_vk_context->m_device.resetFences(finish_fence);

    auto result = m_vk_context->m_device.acquireNextImageKHR(m_vk_context->m_swapchain, std::numeric_limits<u64>::max(), present_semaphore);
    u32 image_index = result.value;

    vk::CommandBufferAllocateInfo primary_cmd_buf_alloc_info(m_vk_context->m_graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1);
    auto cmd_buf = std::move(m_vk_context->m_device.allocateCommandBuffersUnique(primary_cmd_buf_alloc_info).front());
    cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::ClearColorValue color_value(std::array{ 0.f, 0.f, 0.f, 1.f });
    vk::ClearValue clear_value(color_value);

    vk::RenderPassBeginInfo rp_begin_info(render_pass, framebuffers[image_index], { {0,0},m_vk_context->m_window_extent }, clear_value);

    cmd_buf->beginRenderPass(rp_begin_info, vk::SubpassContents::eInline);

    auto push_constants = InstanceRendererPushConstants{
        simulation_params.particle_radius
    };
    cmd_buf->pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<const InstanceRendererPushConstants>(push_constants));

    auto barriers = std::array{
            generalReadWriteBarrier(position_buffer),
    };


    cmd_buf->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);


    vk::DeviceSize offset = 0;

    cmd_buf->bindVertexBuffers(0, { triangle.getBuffer(), position_buffer.get() }, { offset, offset });

    cmd_buf->draw(3, 
    //    1,
        simulation_params.num_particles,
        0, 0);


    //cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});



    //cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, barriers, {});
 
    cmd_buf->endRenderPass();

    cmd_buf->end();

    vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submit_info(present_semaphore, waitStage, cmd_buf.get(), render_semaphore);

    m_vk_context->m_queues.graphics.submit(submit_info, finish_fence);

    m_vk_context->m_device.waitForFences(finish_fence, VK_TRUE, std::numeric_limits<u64>::max());

    vk::PresentInfoKHR present_info(render_semaphore, m_vk_context->m_swapchain, image_index);

    m_vk_context->m_queues.present.presentKHR(present_info);

    

}

void InstanceRenderer::writeBufferDescriptorSets(VulkanBuffer& position_buffer) {
    auto buffers = std::array{
        position_buffer.getDescriptorBufferInfo(),
    };

    vk::WriteDescriptorSet buffers_write(descriptor_sets.set, 0, 0, vk::DescriptorType::eStorageBuffer, {}, buffers, {});


    m_vk_context->m_device.updateDescriptorSets({ buffers_write }, {});
}





