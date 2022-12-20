#include "Mesh.h"


std::vector<vk::VertexInputAttributeDescription> Vertex::getVertexInputDescriptions(u32 binding) {
	std::vector< vk::VertexInputAttributeDescription> res;
	
	res.push_back(vk::VertexInputAttributeDescription(0, binding, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)));
	return res;
}


Mesh::Mesh(std::shared_ptr<VulkanContext> _vk_context, std::vector<Vertex> _vertices) : vertices(_vertices), vk_context(_vk_context){


	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	vertex_buffer = vk_context->createBuffer(usage, sizeof(Vertex), vertices.size());
	store_mesh();
}

void Mesh::store_mesh() {

	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc;
	VulkanBuffer temp_buffer = vk_context->createCPUAccessibleBuffer(usage, sizeof(Vertex), vertices.size());
	temp_buffer.store_data(vertices.data(), vertices.size());

	vk::CommandBufferAllocateInfo cmd_buf_alloc_info(vk_context->m_compute_command_pool, vk::CommandBufferLevel::ePrimary, 1);
	auto cmd_buf = std::move(vk_context->m_device.allocateCommandBuffersUnique(cmd_buf_alloc_info).front());
	cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	cmd_buf->copyBuffer(
		temp_buffer.get(),
		vertex_buffer.get(),
		vk::BufferCopy{ 0, 0, temp_buffer.get_size() }
	);
	cmd_buf->end();
	vk::SubmitInfo submit_info({}, {}, cmd_buf.get(), {});
	vk::Fence finish_fence = vk_context->m_device.createFence({});
	vk_context->m_queues.compute.submit(submit_info, finish_fence);

	auto _ = vk_context->m_device.waitForFences(finish_fence, VK_TRUE, std::numeric_limits<u64>::max());

}

Mesh Mesh::triangleMesh(std::shared_ptr<VulkanContext> _vk_context) {

	std::vector<Vertex> vertices;
	vertices.push_back(Vertex(glm::vec3(0.f, -1.f, 0.f)));
	vertices.push_back(Vertex(glm::vec3(0.f, 1.f, 0.f)));
	vertices.push_back(Vertex(glm::vec3(1.f, 0.f, 1.f)));
	return Mesh(_vk_context, vertices);
}
