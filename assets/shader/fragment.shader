#version 330 core

layout (location = 0) out vec4 FragColor;

in vec4 v_color;
in vec2 v_texture_coordinate;
in float v_tex_index;

uniform vec4 our_color;
uniform sampler2D textures[8];

void main()
{
    vec4 texColor;
    int slot = int(v_tex_index);
    if (slot == -1) {
        // Use only vertex color (for colliders or non-textured geometry)
        FragColor = v_color;
        return;
    }

    if (slot == 0) {
        FragColor = texture(textures[0], v_texture_coordinate);
    } else if (slot == 1) {
        FragColor = texture(textures[1], v_texture_coordinate);
    } else if (slot == 2) {
        FragColor = texture(textures[2], v_texture_coordinate);
    } else if (slot == 3) {
        FragColor = texture(textures[3], v_texture_coordinate);
    } else if (slot == 4) {
        FragColor = texture(textures[4], v_texture_coordinate);
    } else if (slot == 5) {
        FragColor = texture(textures[5], v_texture_coordinate);
    } else if (slot == 6) {
        FragColor = texture(textures[6], v_texture_coordinate);
    } else {
        FragColor = texture(textures[7], v_texture_coordinate);
    }

    // Modulate texture color by vertex color for tinting
    // FragColor = texColor * v_color;
}