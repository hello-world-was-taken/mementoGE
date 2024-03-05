#version 330 core

layout (location = 0) out vec4 FragColor;

in vec2 v_texture_coordinate;
in float v_tex_index;

uniform vec4 our_color;
uniform sampler2D textures[8];

void main()
{
    // FragColor = our_color;
    int slot = int(v_tex_index);
    FragColor = texture(textures[slot], v_texture_coordinate);
}