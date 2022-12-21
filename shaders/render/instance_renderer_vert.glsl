#version 460

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 pPosition;


layout(push_constant) uniform RendererPushConstants {
	float particle_radius;
};

mat4 view_matrix = mat4(1, 0, 0, 0, 
				0, 1, 0, 0, 
				0, 0, 1, 0, 
				0, 0, 0, 1);


void main()
{
	mat4 translation = mat4 (
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	pPosition.x, pPosition.y, pPosition.z, 1.0
	);

	gl_Position =  view_matrix * translation * particle_radius * vPosition;
}