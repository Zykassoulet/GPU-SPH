#version 460
#pragma shader_stage(compute)
#extension GL_EXT_debug_printf : enable

#define pi 3.141592653589793238

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std140, push_constant) uniform ForceComputePushConstants {
	ivec4 simulation_domain; // In Z-index space (real space/grid spacing)
	vec4 box_size;
	uint block_size; // In Z-index space (real space/grid spacing)
	float kernel_radius;
	float particle_mass;
	float dt;
	float particle_radius;
};

layout(set = 0, binding = 0) readonly buffer ZIndexBuffer {
	uint z_index_buffer[];
};

layout(set = 0, binding = 1) readonly buffer ParticleIndexBuffer {
	uint particle_index_buffer[];
};

layout(set = 0, binding = 2) readonly buffer PosBuffer {
	vec4 position_buffer[];
};

layout(set = 0, binding = 3) readonly buffer DensityBuffer {
	float density_buffer[];
};

layout(set = 0, binding = 4) readonly buffer VelocityBuffer {
	vec4 velocity_buffer[];
};

layout(set = 0, binding = 5) readonly buffer PressureBuffer {
	float pressure_buffer[];
};

struct Block {
	uint first_particle_index;
	uint num_particles;
};

layout(std430, set = 1, binding = 0) readonly buffer UncompactedBlockBuffer {
	Block uncompacted_block_buffer[];
};

layout(std430, set = 1, binding = 1) readonly buffer CompactedBlockBuffer {
	Block compacted_block_buffer[];
};

layout(set = 2, binding = 0) writeonly buffer OutPosBuffer {
	vec4 out_position_buffer[];
};

layout(set = 2, binding = 1) writeonly buffer OutVelocityBuffer {
	vec4 out_velocity_buffer[];
};

struct ParticleInfo {
	bool valid;
	vec4 position;
	float density;
	float pressure;
	vec4 velocity;
	uint global_id;
};

shared ParticleInfo sParticles[256];


vec3 ihash(uvec3 x) //  https://www.shadertoy.com/view/XlXcW4
{
	uint k = 1103515245U;  // GLIB C
	x = ((x >> 8U) ^ x.yzx) * k;
	x = ((x >> 8U) ^ x.yzx) * k;
	x = ((x >> 8U) ^ x.yzx) * k;

	return vec3(x) * (1.0 / float(0xffffffffU));
}

vec4 randomDir(uvec3 x) {
	vec3 rand_vec = ihash(x);
	float d = length(rand_vec);
	rand_vec = rand_vec / d;
	return vec4(rand_vec.x, rand_vec.y, rand_vec.z, 0.);
}

uint getGlobalParticleIndex(Block block, uint particle_offset) {
	uint particle_index_buffer_index = block.first_particle_index + particle_offset;
	return particle_index_buffer[particle_index_buffer_index];
}

void fetchParticle(uint source_block_index, uint particle_offset, uint block_offset) {
	Block source_block = uncompacted_block_buffer[source_block_index];
	uint particle_index = block_offset + particle_offset;
	uint global_particle_index = getGlobalParticleIndex(source_block, particle_index);

	sParticles[particle_offset].valid = (particle_index < source_block.num_particles);
	if ((particle_index < source_block.num_particles)) {
		sParticles[particle_offset].position = position_buffer[global_particle_index];
		sParticles[particle_offset].density = density_buffer[global_particle_index];
		sParticles[particle_offset].velocity = velocity_buffer[global_particle_index];
		sParticles[particle_offset].pressure = pressure_buffer[global_particle_index];
		sParticles[particle_offset].global_id = global_particle_index;

	}
}

ParticleInfo fetchCurrentParticle(Block current_block, uint particle_offset) {
	uint global_particle_index = getGlobalParticleIndex(current_block, particle_offset);

	ParticleInfo particle;

	particle.valid = (particle_offset < current_block.num_particles);
	if (particle.valid) {
		particle.position = position_buffer[global_particle_index];
		particle.density = density_buffer[global_particle_index];
		particle.velocity = velocity_buffer[global_particle_index];
		particle.pressure = pressure_buffer[global_particle_index];
		particle.global_id = global_particle_index;

	}
	return particle;
}

float gradSpikyKernelC = -45.f / (pi * pow(kernel_radius,6));
vec4 gradSpikyKernel(vec4 dx, vec4 rand_dir){
	float d = length(dx);
	float a = kernel_radius - d;
	vec4 dir = d == 0 ? vec4(0,0,0,0) : dx / d;
	//vec4 dir = d == 0 ? rand_dir : dx / d;
	return a > 0 ? gradSpikyKernelC * a * a * dir : vec4(0);
}

uint deinterleave(uint value, uint offset, uint spacing) {
	uint o = 0;
	uint i = 0;
	for (uint bit = offset; bit < 32; bit += spacing, i++) {
		o |= (((value & (1u << bit)) != 0 ? 1u : 0u) << i);
	}
	return o;
}

uint interleave(uint value, uint offset, uint spacing) {
	uint o = 0;
	uint i = 0;
	for (uint bit = offset; bit < 32; bit += spacing, i++) {
		o |= (((value & (1u << i)) != 0 ? 1u : 0u) << bit);
	}
	return o;
}


vec4 handleBoundariesPos(vec4 pos) {
	vec4 r = vec4(particle_radius, particle_radius, particle_radius, 0);
	pos = clamp(pos, r, box_size - r);
	return pos;
}


#define COLLISION_DAMPING 0.95
vec4 handleBoundariesVel(vec4 pos, vec4 vel) {
	vec4 center_offset = pos - box_size / 2.;
	vel.x = abs(center_offset.x) + particle_radius - box_size.x / 2. > 0. ? -COLLISION_DAMPING * vel.x : vel.x;
	vel.y = abs(center_offset.y) + particle_radius - box_size.y / 2. > 0. ? -COLLISION_DAMPING * vel.y : vel.y;
	vel.z = abs(center_offset.z) + particle_radius - box_size.z / 2. > 0. ? -COLLISION_DAMPING * vel.z : vel.z;

	return vel;
}

int getNeighborBlockIndex(Block block, ivec3 offset) {
	uint S3 = block_size * block_size * block_size;
	uint block_z_index = (z_index_buffer[block.first_particle_index] / S3) * S3;
	int block_x = int(deinterleave(block_z_index, 0, 3));
	int block_y = int(deinterleave(block_z_index, 1, 3));
	int block_z = int(deinterleave(block_z_index, 2, 3));

	block_x += offset.x * int(block_size);
	block_y += offset.y * int(block_size);
	block_z += offset.z * int(block_size);

	if (block_x >= 0 && block_x < simulation_domain.x
	&& block_y >= 0 && block_y < simulation_domain.y
	&& block_z >= 0 && block_z < simulation_domain.z) {
		block_z_index = interleave(block_x, 0, 3) | interleave(block_y, 1, 3) | interleave(block_z, 2, 3);
		return int(block_z_index / S3);
	} else {
		return -1;
	}
}

void main() {
	Block current_block = compacted_block_buffer[gl_WorkGroupID.x];
	uint particle_index = gl_LocalInvocationID.x;
	vec4 force = vec4(0.0);
	ParticleInfo current_particle = fetchCurrentParticle(current_block, particle_index);

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++){
			for (int k = -1; k <= 1; k++){
				int neighbor_block_index = getNeighborBlockIndex(current_block, ivec3(i,j,k));
				if (neighbor_block_index != -1) {
					uint neighbor_num_particles = uncompacted_block_buffer[neighbor_block_index].num_particles;
					for (uint block_offset = 0; block_offset < neighbor_num_particles; block_offset += 256) {
						fetchParticle(uint(neighbor_block_index), particle_index, block_offset);

						memoryBarrierShared();
						barrier();

						if (current_particle.valid) {
							for (uint o = 0; o < 256; o++) {
								if (sParticles[o].valid && (sParticles[o].global_id != current_particle.global_id)) {
									vec4 rand_dir = randomDir(uvec3(sParticles[o].global_id, current_particle.global_id, o));
									//force -= (particle_mass/sParticles[o].density)
									//* (current_particle.pressure + sParticles[o].pressure)/2.0
									//* gradSpikyKernel(current_particle.position - sParticles[o].position, rand_dir);

									force -= particle_mass
									* (current_particle.pressure / (current_particle.density * current_particle.density) + sParticles[o].pressure / (sParticles[o].density * sParticles[o].density))
									* gradSpikyKernel(current_particle.position - sParticles[o].position, rand_dir);
								}
							}
						}
					}
				}
			}
		}
	}

	vec4 gravity_acceleration = vec4(0, 0, -1., 0); // Gravity

	uint global_particle_index = getGlobalParticleIndex(current_block, particle_index);
	if (current_particle.valid) {
		//vec4 velocity = (force / current_particle.density + gravity_acceleration);
		vec4 velocity = velocity_buffer[global_particle_index] + dt * (force + gravity_acceleration);
		//vec4 velocity = velocity_buffer[global_particle_index] + dt * (force / current_particle.density + gravity_acceleration);
		vec4 position = position_buffer[global_particle_index] + dt * velocity;
		velocity = handleBoundariesVel(position, velocity);
		position = handleBoundariesPos(position);


		out_velocity_buffer[global_particle_index] = velocity;
		out_position_buffer[global_particle_index] = position;
	}
}