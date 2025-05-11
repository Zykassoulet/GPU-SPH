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

Mesh Mesh::unitIcosahedronMesh(std::shared_ptr<VulkanContext> vk_context) {

    float phi = (1.0f + sqrt(5.0f)) * 0.5f; // golden ratio
    float a = 1.0f;
    float b = 1.0f / phi;
    float d = glm::length(glm::vec2(a, b));
    a = a / d;
    b = b / d;

    // add vertices
    auto v1  = glm::vec3(0, b, -a);
    auto v2  = glm::vec3(b, a, 0);
    auto v3  = glm::vec3(-b, a, 0);
    auto v4  = glm::vec3(0, b, a);
    auto v5  = glm::vec3(0, -b, a);
    auto v6  = glm::vec3(-a, 0, b);
    auto v7  = glm::vec3(0, -b, -a);
    auto v8  = glm::vec3(a, 0, -b);
    auto v9  = glm::vec3(a, 0, b);
    auto v10 = glm::vec3(-a, 0, -b);
    auto v11 = glm::vec3(b, -a, 0);
    auto v12 = glm::vec3(-b, -a, 0);



    std::vector<Vertex> vertices;
    auto add_triangle = [&](glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
        vertices.push_back(Vertex(v1));
        vertices.push_back(Vertex(v2));
        vertices.push_back(Vertex(v3));
    };

    add_triangle(v2, v3, v4);
    add_triangle(v3, v2, v1);
    add_triangle(v6, v5, v4);
    add_triangle(v5, v9, v4);
    add_triangle(v8, v7, v1);
    add_triangle(v7, v10, v1);
    add_triangle(v12, v11, v5);
    add_triangle(v11, v12, v7);
    add_triangle(v10, v6, v3);
    add_triangle(v6, v10, v12);
    add_triangle(v9, v8, v2);
    add_triangle(v8, v9, v11);
    add_triangle(v3, v6, v4);
    add_triangle(v9, v2, v4);
    add_triangle(v10, v3, v1);
    add_triangle(v2, v8, v1);
    add_triangle(v12, v10, v7);
    add_triangle(v8, v11, v7);
    add_triangle(v6, v12, v5);
    add_triangle(v11, v9, v5);
    return Mesh(vk_context, vertices);
}
