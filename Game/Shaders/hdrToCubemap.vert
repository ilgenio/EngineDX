#version 460

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 coords;

//push constants block
layout( push_constant ) uniform ViewConstants
{
	mat4 view;
	mat4 proj;
} constants;

void main()
{
    coords      = position;
    gl_Position = constants.proj*vec4(mat3(constants.view)*position, 1.0); // not translation of view
}

