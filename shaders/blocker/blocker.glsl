#version 450
#pragma shader_stage(compute)

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, set = 0, binding = 0) readonly buffer ZIndexBuffer {
    uint z_index_buffer[];
};

struct Block {
    uint first_particle_index;
    uint num_particles;
};

layout(std430, set = 0, binding = 1) buffer BlockBuffer {
    Block block_buffer[];
};

layout(std430, push_constant) uniform GridInfo {
    uint block_size; // S
};

void main() {
    // Take a particle from ZIndexBuffer
    uint particle_index = gl_GlobalInvocationID.x;
    uint num_particles = z_index_buffer.length();
    if (particle_index < num_particles) {
        uint z_index = z_index_buffer[particle_index];

        // Calculate block index of particle (based on s and S^3)
        uint S3 = block_size * block_size * block_size;
        uint block_index = z_index / S3;

        // Adjust block data
        atomicMin(block_buffer[block_index].first_particle_index, particle_index);
        atomicAdd(block_buffer[block_index].num_particles, 1);
    }
}