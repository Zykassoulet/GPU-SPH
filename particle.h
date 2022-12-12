#ifndef _PARTICLE_H_
#define _PARTICLE_H_
#ifndef GL_core_profile
// C++ defintion of particle struct
#include "utils.h"

struct Particle {
    f32 pos_x;
    f32 pos_y;
    f32 pos_z;
};

#else
// GLSL definition of particle struct

struct Particle {
    float pos_x;
    float pos_y;
    float pos_z;
};

#endif // GL_core_profile
#endif // _PARTICLE_H_