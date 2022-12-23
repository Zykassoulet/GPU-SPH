#pragma once
#include "utils.h"
#include "VulkanContext.h"
#include <vector>
#include <array>
#include "glm/glm.hpp"
#include "VulkanHelpers.h"


struct Vertex {
	glm::vec3 position;
	static std::vector<vk::VertexInputAttributeDescription> getVertexInputDescriptions(u32 binding);

    Vertex(const glm::vec3& position) : position(position) {}
};


class Mesh {
public:

	Mesh(std::shared_ptr<VulkanContext> vk_context, std::vector<Vertex> vertices);
	Mesh() = default;
	static Mesh triangleMesh(std::shared_ptr<VulkanContext> vk_context);
    static Mesh unitIcosahedronMesh(std::shared_ptr<VulkanContext> vk_context);

	void store_mesh();
    usize vertex_count() { return vertices.size(); }

	inline vk::Buffer getBuffer() { return vertex_buffer.get(); }

private:

	std::vector<Vertex> vertices;
	VulkanBuffer vertex_buffer;
	std::shared_ptr<VulkanContext> vk_context;


};


