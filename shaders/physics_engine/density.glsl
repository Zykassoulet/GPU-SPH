#version 460
#pragma shader_stage(compute)

#define pi 3.141592653589793238

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std140, push_constant) uniform DensityComputePushConstants {
	ivec4 simulation_domain; // In Z-index space (real space/grid spacing)
	uint block_size; // In Z-index space (real space/grid spacing)
	float kernel_radius;
	float particle_mass;
	float stiffness;
	float rest_density;
	float gamma;
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

layout(set = 0, binding = 3) writeonly buffer DensityBuffer {
	float density_buffer[];
};

layout(set = 0, binding = 4) writeonly buffer PressureBuffer {
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

struct ParticleInfo {
	bool valid;
	vec4 position;
};

shared ParticleInfo sParticles[256];

uint getGlobalParticleIndex(Block block, uint particle_offset) {
	uint particle_index_buffer_index = block.first_particle_index + particle_offset;
	return particle_index_buffer[particle_index_buffer_index];
}

void fetchParticle(uint source_block_index, uint particle_offset) {
	Block source_block = uncompacted_block_buffer[source_block_index];
	uint global_particle_index = getGlobalParticleIndex(source_block, particle_offset);

	sParticles[particle_offset].valid = (particle_offset < source_block.num_particles);
	if ((particle_offset < source_block.num_particles)) {
		sParticles[particle_offset].position = position_buffer[global_particle_index];
	}
}

ParticleInfo fetchCurrentParticle(Block current_block, uint particle_offset) {
	uint global_particle_index = getGlobalParticleIndex(current_block, particle_offset);

	ParticleInfo particle;

	particle.valid = (particle_offset < current_block.num_particles);
	if (particle.valid) {
		particle.position = position_buffer[global_particle_index];
	}
	return particle;
}

float poly6KernelC = 315. / (64. * pi * pow(kernel_radius, 9));
float poly6Kernel(float d_2){
	float h_2 = kernel_radius * kernel_radius;
	float a = h_2 - d_2;
	float res = poly6KernelC * a * a * a;
	return d_2 < h_2 ? res : 0;
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
	float local_density = 0;
	ParticleInfo current_particle = fetchCurrentParticle(current_block, particle_index);

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++){
			for (int k = -1; k <= 1; k++){
				int neighbor_block_index = getNeighborBlockIndex(current_block, ivec3(i,j,k));
				if (neighbor_block_index != -1) {
					fetchParticle(uint(neighbor_block_index), particle_index);
				}

				barrier();

				if (current_particle.valid) {
					for (int o = 0; o < 256; o++) {
						if (sParticles[o].valid) {
							vec4 dx = sParticles[o].position - current_particle.position;
							local_density += particle_mass * poly6Kernel(dot(dx, dx));
						}
					}
				}
			}
		}
	}

	uint global_particle_index = getGlobalParticleIndex(current_block, particle_index);
	if (current_particle.valid) {
		density_buffer[global_particle_index] = local_density;
		float pressure = (stiffness * rest_density / gamma) * (pow(local_density/rest_density, gamma) - 1.0f);
		pressure_buffer[global_particle_index] = pressure;
	}
}