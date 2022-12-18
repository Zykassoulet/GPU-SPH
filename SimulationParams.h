#pragma once

#include "utils.h"
#include <glm/glm.hpp>

struct SimulationParams {
    u32 block_size;     //
    u32 num_particles;  //
    u32 num_blocks;
    f32 kernel_radius;  //
    f32 particle_mass;  //
    f32 stiffness;      //
    f32 rest_density;   //
    f32 gas_gamma;      //
    f32 dt;             //
    f32 grid_unit;      //
    glm::ivec4 grid_size;//
};