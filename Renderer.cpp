#include "Renderer.h"
#include <array>
#include "glm/glm.hpp"



Renderer::Renderer(std::shared_ptr<VulkanContext> vulkan_context) : vk_context(vulkan_context) {
	createDescriptions();
}


void Renderer::createDescriptions() {
	binding_description = vk::VertexInputBindingDescription(0, sizeof(glm::vec3));
	attribute_description = vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0);

}









