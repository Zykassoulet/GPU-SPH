#version 460
#extension GL_KHR_vulkan_glsl : enable

#define pi 3.141592653589793238

layout (local_size_x = 256) in;



layout(push_constant) uniform DensityComputePushConstants {
	int num_particles;
	float kernel_radius;
};

layout(set = 0, binding = 0) readonly buffer PosBuffer{
	vec3 posBuffer[];
};

layout(set = 0, binding = 1) readonly buffer ZIndexBuffer{
	ivec3 zindexBuffer[];
};

layout(set = 0, binding = 2) buffer DensityBuffer
{
	float densityBuffer[];
};

struct BlockData {
	int number_particles;
	int first_particles_offset;
};

layout(std140,set = 0, binding = 3) readonly buffer BlockDataBuffer{
	BlockData blockDataBuffer[];
};



float kernel(float r){
	float h_2 = kernel_radius * kernel_radius;
	float h_3 = h_2 * kernel_radius;
	float r_2 = r * r;
	float a = (h_2 - r_2) / h_3;
	float res = 315./(64.*pi) * a * a * a;
	return r < kernel_radius ? res : 0;
}

int getBlockID(ivec3 zindex){
	
}

void main() {
	if (gl_GlobalInvocationID.x < num_particles){
		int block_id = getBlockID(zindexBuffer[gl_GlobalInvocationID.x]);
		for (int i = 0; i < 27
	}
}