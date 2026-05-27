#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 in_tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec2 out_tex_coord;

void main()
{
    out_tex_coord = in_tex_coord;
    gl_Position = proj * view * model * vec4(vertex_pos, 1.0);
}