#version 450
#pragma shader_stage(compute)

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Block {
    uint first_particle_index;
    uint num_particles;
};

layout(std430, set = 0, binding = 0) readonly buffer BlockBuffer {
    Block uncompacted_block_buffer[];
};

layout(std430, set = 0, binding = 1) buffer CompactedBlockBuffer {
    Block compacted_block_buffer[];
};

layout(std430, set = 0, binding = 2) buffer CompactedInfo {
    uint compacted_block_count;
};

layout(std430, push_constant) uniform SimInfo {
    uint max_particles_per_block; // S
};

void main() {
    if (gl_GlobalInvocationID.x < uncompacted_block_buffer.length()) {
        Block b = uncompacted_block_buffer[gl_GlobalInvocationID.x];
        for (uint offset = 0; offset < b.num_particles; offset += max_particles_per_block) {
            uint i = atomicAdd(compacted_block_count, 1);
            Block out_block;
            out_block.first_particle_index = offset;
            out_block.num_particles = min(max_particles_per_block, b.num_particles - offset);
            compacted_block_buffer[i] = out_block;
        }
    }
}
