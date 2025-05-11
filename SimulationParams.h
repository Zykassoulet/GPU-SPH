#pragma once

#include "utils.h"
#include <glm/glm.hpp>

struct SimulationParams {
    u32 num_particles;  //
    u32 num_blocks;
    u32 max_num_compacted_blocks;
    f32 kernel_radius;  //
    f32 particle_mass;  //
    f32 stiffness;     //
    f32 rest_density;  //
    f32 gas_gamma;     //
    f32 dt;            //
    u32 block_size;
    f32 grid_unit;     //
    glm::ivec4 grid_size;//
    f32 particle_radius;  //
    glm::vec4 box_size; //
};