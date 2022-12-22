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

layout(std140, push_constant) uniform GridInfo {
    uint block_size; // S
};

void main() {
    uint block_index = gl_GlobalInvocationID.x;
    if (block_index < block_buffer.length()) {
        block_buffer[block_index].first_particle_index = 0xFFFFFFFF;
        block_buffer[block_index].num_particles = 0;
    }
}