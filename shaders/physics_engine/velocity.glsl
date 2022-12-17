#version 460
#extension GL_KHR_vulkan_glsl : enable

#define pi 3.141592653589793238

layout (local_size_x = 256) in;



layout(push_constant) uniform DensityComputePushConstants {
	int num_particles;
	float kernel_radius;
	ivec3 blocks_count;
	float particle_mass;
	float dt;
};

layout(set = 0, binding = 0) readonly buffer PosBuffer{
	vec3 posBuffer[];
};

layout(set = 0, binding = 1) readonly buffer ZIndexBuffer{
	ivec3 zindexBuffer[];
};

layout(set = 0, binding = 2) readonly buffer DensityBuffer
{
	float densityBuffer[];
};

layout(set = 0, binding = 3) buffer VelocityBuffer
{
	vec3 velocityBuffer[];
};

struct BlockData {
	int number_particles;
	int first_particles_offset;
};

layout(std140,set = 0, binding = 4) readonly buffer BlockDataBuffer{
	BlockData blockDataBuffer[];
};



float grad_kernel(float r){

}

int getBlockID(ivec3 zindex){
	
}

int getNeighborBlockID(int blockid, ivec3 offset){
	int offset_x = 
	int offset_y = 
	int offset_z = 
	return block_id + offset_x + offset_y + offset_z;
}

bool isBlockInDomain(int block_id){

}

void main() {
	if (gl_GlobalInvocationID.x < num_particles){
		int block_id = getBlockID(zindexBuffer[gl_GlobalInvocationID.x]);
		for (int i = -1; i < 2; i++){
			for (int j = -1; j < 2; j++){
				for (int k = -1; k < 2; k++){
					int neighbor_block_id = getNeighborBlockID(block_id, ivec3(i,j,k));
					if( isBlockInDomain(neighbor_block_id)){

					}
				}
			}
		}
	}
}