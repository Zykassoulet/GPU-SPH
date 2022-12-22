#version 430
#pragma shader_stage(compute)

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


layout(std430, set = 0, binding = 0) readonly buffer ParticleBuffer {
    vec4 particle_buffer[];
};

layout(std430, set = 0, binding = 1) writeonly buffer ZIndexBuffer {
    uint z_index_buffer[];
};

layout(std430, set = 0, binding = 2) writeonly buffer ParticleIndexBuffer {
    uint particle_index_buffer[];
};

layout(std430, set = 1, binding = 0) readonly buffer XInterleaveBuffer {
    uint x_interleave_buffer[];
};

layout(std430, set = 1, binding = 1) readonly buffer YInterleaveBuffer {
    uint y_interleave_buffer[];
};

layout(std430, set = 1, binding = 2) readonly buffer ZInterleaveBuffer {
    uint z_interleave_buffer[];
};

layout(std430, push_constant) uniform GridInfo {
    uint grid_size_x;
    uint grid_size_y;
    uint grid_size_z;
    float grid_unit_size;
};

void main() {
    uint particle_index = gl_GlobalInvocationID.x;
    uint num_particles = particle_buffer.length();
    if (particle_index < num_particles) {
        particle_index_buffer[particle_index] = particle_index;
        vec4 particle = particle_buffer[particle_index];

        uint z_index = 0;
        z_index |= x_interleave_buffer[min(uint(floor(particle.x / grid_unit_size)), grid_size_x - 1)];
        z_index |= y_interleave_buffer[min(uint(floor(particle.y / grid_unit_size)), grid_size_y - 1)];
        z_index |= z_interleave_buffer[min(uint(floor(particle.z / grid_unit_size)), grid_size_z - 1)];

        z_index_buffer[particle_index] = z_index;
    }
}