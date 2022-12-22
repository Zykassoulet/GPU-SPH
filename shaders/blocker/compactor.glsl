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

struct DispatchCommand {
    uint x;
    uint y;
    uint z;
};

layout(std430, set = 0, binding = 2) buffer CompactedInfo {
    DispatchCommand dispatch_block;
};

#define max_particles_per_block 256u

void main() {
    if (gl_GlobalInvocationID.x < uncompacted_block_buffer.length()) {
        Block b = uncompacted_block_buffer[gl_GlobalInvocationID.x];
        for (uint offset = 0; offset < b.num_particles; offset += max_particles_per_block) {
            uint i = atomicAdd(dispatch_block.x, 1);
            dispatch_block.y = 1;
            dispatch_block.z = 1;
            Block out_block;
            out_block.first_particle_index = offset + b.first_particle_index;
            out_block.num_particles = min(max_particles_per_block, b.num_particles - offset);
            if (i < compacted_block_buffer.length()) {
                compacted_block_buffer[i] = out_block;
            }
        }
    }
}
