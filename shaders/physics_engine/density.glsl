#version 460
#extension GL_KHR_vulkan_glsl : enable

#define pi 3.141592653589793238

layout (local_size_x = 256) in;



layout(push_constant) uniform DensityComputePushConstants {
	int num_particles;
	float kernel_radius;
	ivec3 blocks_count;
	float particle_mass;
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

ivec3 zindexToBlockPos(ivec3 zindex){
	
}

int blockPosToBlockID(ivec3 block_pos){

}

ivec3 blockIDToBlockPos(int block_id){

}

bool isInBlockDomain(ivec3 block_pos){
	return 0 <= block_pos.x && block_pos.x < blocks_count.x &&
		0 <= block_pos.y && block_pos.y < blocks_count.y && 
		0 <= block_pos.z && block_pos.z < blocks_count.z;
}

void main() {
	uint cur_part_id = gl_GlobalInvocationID.x;
	if (cur_part_id < num_particles){
		float density = 0;
		ivec3 block_pos = zindexToBlockPos(zindexBuffer[cur_part_id]);
		for (int i = -1; i < 2; i++){
			for (int j = -1; j < 2; j++){
				for (int k = -1; k < 2; k++){
					ivec3 neighbor_block_pos = block_pos + ivec3(i,j,k);
					if( isInBlockDomain(neighbor_block_pos)){
						int neighbor_block_id = blockPosToBlockID(neighbor_block_pos);
						int first_particle_id = blockDataBuffer[neighbor_block_id].first_particles_offset;
						int num_particles = blockDataBuffer[neighbor_block_id].number_particles;
						for (int p = first_particle_id; p < first_particle_id + num_particles; p++){
							if (p != cur_part_id){
								float dist = length(posBuffer[p] - posBuffer[cur_part_id]);
								density += particle_mass * kernel(dist);
							}
						}
					}
				}
			}
		}
		densityBuffer[cur_part_id] = density;
	}
}