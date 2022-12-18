#version 460
#extension GL_KHR_vulkan_glsl : enable

#define pi 3.141592653589793238
#define g 9.81

layout (local_size_x = 256) in;



layout(push_constant) uniform DensityComputePushConstants {
	int num_particles;
	float kernel_radius;
	ivec3 blocks_count;
	float particle_mass;
	float dt;
	float rest_density;
	float k;
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


float pressure(float density){
	return max(0, k * (density - rest_density));
}


float gradSpikyKernelC = - 45. / (pi * pow(kernel_radius,6));

vec3 gradSpikyKernel(vec3 dx){
	float d = length(dx);
	float a = kernel_radius - d;
	return a > 0 ? gradSpikyKernelC * a * a * dx / d : vec3(0);
}

ivec3 zindexToBlockPos(ivec3 zindex){
	
}

int blockPosToBlockID(ivec3 block_pos){

}


bool isInBlockDomain(ivec3 block_pos){
	return 0 <= block_pos.x && block_pos.x < blocks_count.x &&
		0 <= block_pos.y && block_pos.y < blocks_count.y && 
		0 <= block_pos.z && block_pos.z < blocks_count.z;
}

void main() {
	uint cur_part_id = gl_GlobalInvocationID.x;
	if (cur_part_id < num_particles){
		vec3 pressure_force_acceleration = vec3(0);
		float cur_pressure = pressure(densityBuffer[cur_part_id]);
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
								float neighbor_pressure = pressure(densityBuffer[p]);
								
								vec3 grad_kernel = gradSpikyKernel(posBuffer[cur_part_id] - posBuffer[p]);

								pressure_force_acceleration += -particle_mass / (2.f * densityBuffer[cur_part_id] * densityBuffer[p]) * (cur_pressure + neighbor_pressure) * grad_kernel;
							}
						}
					}
				}
			}
		}
		vec3 gravity_acceleration = vec3(0.f, 0.f, -g);
		velocityBuffer[cur_part_id] += dt * (pressure_force_acceleration + gravity_acceleration);
	}
}