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

    /**
    * For GLSL above 3.3
    * FragColor = texture(textures[slot], v_texture_coordinate);
    */

    if (slot == 0) texColor = texture(textures[0], v_texture_coordinate);
    else if (slot == 1) texColor = texture(textures[1], v_texture_coordinate);
    else if (slot == 2) texColor = texture(textures[2], v_texture_coordinate);
    else if (slot == 3) texColor = texture(textures[3], v_texture_coordinate);
    else if (slot == 4) texColor = texture(textures[4], v_texture_coordinate);
    else if (slot == 5) texColor = texture(textures[5], v_texture_coordinate);
    else if (slot == 6) texColor = texture(textures[6], v_texture_coordinate);
    else if (slot == 7) texColor = texture(textures[7], v_texture_coordinate);
    else {
        FragColor = v_color; // fallback
        return;
    }

    // TODO: support tiniting
    FragColor = texColor;
}