#version 330 core
layout(location = 0) in vec2 aPos;

uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;

void main() {
    gl_Position = u_projection_matrix * u_view_matrix * vec4(aPos, 0.0, 1.0);
}
