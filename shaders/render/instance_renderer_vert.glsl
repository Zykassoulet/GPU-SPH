#version 460

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 pPosition;
layout (location = 2) in vec4 pVelocity;

layout (location = 0) out vec4 world_space_pos;
layout(location = 1) out float pVelocityAbs;


layout(push_constant) uniform RendererPushConstants {
	mat4 view_proj;
	float particle_radius;
};

void main()
{
	mat4 scale = mat4(particle_radius);
	scale[3][3] = 1.0;

	mat4 translation = mat4 (
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	pPosition.x, pPosition.y, pPosition.z, 1.0
	);

	gl_Position =  view_proj * translation * scale * vPosition;
	world_space_pos = translation * scale * vPosition;
	pVelocityAbs = length(pVelocity);

}