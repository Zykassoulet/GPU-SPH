#version 460

layout (location = 0) in vec4 world_space_pos;

layout(location = 1) in flat float velocity_abs;



//output write
layout (location = 0) out vec4 outFragColor;

vec3 hsv2rgb(vec3 c) {
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 hsv_green = vec3(0.3, 1, 0.5);
vec3 hsv_red = vec3(0., 1, 0.5);



void main()
{
	float kinetic_energy_div_mass = 0.5 * velocity_abs * velocity_abs;
	float gravity_potential_energy_div_mass = 1. * world_space_pos.z;
	
	vec3 hsv = tanh(kinetic_energy_div_mass + gravity_potential_energy_div_mass) * (hsv_red - hsv_green) + hsv_green;
	vec3 rgb = hsv2rgb(hsv);

	outFragColor = vec4(rgb.r, rgb.g, rgb.b,0);
}