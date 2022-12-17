#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (local_size_x = 256) in;

layout(push_constant) uniform DensityComputePushConstants {
	int num_particles;
	float dt;
};

layout(set = 0, binding = 0) buffer PosBuffer{
	vec3 posBuffer[];
};

layout(set = 0, binding = 1) readonly buffer VelocityBuffer
{
	vec3 velocityBuffer[];
};


void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i < num_particles){
		posBuffer[i] = dt * velocityBuffer[i];
	}
}