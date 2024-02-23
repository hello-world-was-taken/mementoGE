#version 330 core

layout (location = 0) out vec4 FragColor;

in vec2 v_texture_coordinate;

uniform vec4 our_color;
uniform sampler2D our_texture;

void main()
{
    // FragColor = our_color;
    FragColor = texture(our_texture, v_texture_coordinate);
}