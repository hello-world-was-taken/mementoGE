#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texture_coordinate;
layout(location = 3) in int tex_index;

uniform mat4 u_projection_matrix;
uniform mat4 u_view_matrix;

out vec2 v_texture_coordinate;

void main() {
    gl_Position = u_projection_matrix * u_view_matrix  * vec4(position, 1.0);
    v_texture_coordinate = texture_coordinate;
}