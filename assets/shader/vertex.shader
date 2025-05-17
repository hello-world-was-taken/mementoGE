#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texture_coordinate;
layout(location = 3) in float tex_index;

uniform mat4 u_projection_matrix;
uniform mat4 u_view_matrix;

out vec2 v_texture_coordinate;
out float v_tex_index;

void main() {
    // We can think of 'vec4(position, 1.0)' as the model_matrix, since we are passing in the
    // world coordinates of sprites in our implementation.
    gl_Position = u_projection_matrix * u_view_matrix  * vec4(position, 1.0);
    v_texture_coordinate = texture_coordinate;
    v_tex_index = tex_index;
}