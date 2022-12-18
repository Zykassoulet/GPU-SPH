#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (local_size_x = 256) in;

layout(push_constant) uniform DensityComputePushConstants {
	int num_particles;
	float dt;
	float particle_radius;
	vec3 box_size;
};

layout(set = 0, binding = 0) buffer PosBuffer{
	vec3 posBuffer[];
};

layout(set = 0, binding = 1) buffer VelocityBuffer
{
	vec3 velocityBuffer[];
};


vec3 handleBoundariesPos(vec3 pos){
	
	pos.x = particle_radius + abs(pos.x - particle_radius);
	pos.x = box_size.x - particle_radius - abs(box_size.x - particle_radius - pos.x);
	pos.y = particle_radius + abs(pos.y - particle_radius);
	pos.y = box_size.y - particle_radius - abs(box_size.y - particle_radius - pos.y);
	pos.z = particle_radius + abs(pos.z - particle_radius);
	pos.z = box_size.z - particle_radius - abs(box_size.z - particle_radius - pos.z);
	return pos;
}

vec3 handleBoundariesVel(vec3 pos, vec3 vel){
	vel.x = sign(pos.x - particle_radius) * vel.x;
	vel.x = sign(box_size.x - particle_radius - pos.x) * vel.x;
	vel.y = sign(pos.y - particle_radius) * vel.y;
	vel.y = sign(box_size.y - particle_radius - pos.y) * vel.y;
	vel.z = sign(pos.z - particle_radius) * vel.z;
	vel.z = sign(box_size.z - particle_radius - pos.z) * vel.z;
	return vel;
}


void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i < num_particles){
		vec3 vel = velocityBuffer[i];
		vec3 new_pos = posBuffer[i] + dt * vel;
		velocityBuffer[i] = handleBoundariesVel(new_pos, vel);
		posBuffer[i] = handleBoundariesPos(new_pos);
	}
}