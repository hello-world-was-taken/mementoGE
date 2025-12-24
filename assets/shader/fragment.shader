#version 330 core

layout (location = 0) out vec4 FragColor;

in vec4 v_color;
in vec2 v_texture_coordinate;
in float v_tex_index;

uniform vec4 our_color;
uniform sampler2D textures[16];

void main()
{
    vec4 texColor;
    int slot = int(v_tex_index);
    if (slot == -1) {
        // Use only vertex color (for colliders or non-textured geometry)
        FragColor = v_color;
        return;
    } else if (slot > 15) {
        FragColor = v_color; // fallback
        return;
    }

    texColor = texture(textures[slot], v_texture_coordinate);
    // discard transparent textures
    if(texColor.a < 0.1)
        discard;

    FragColor = texColor * v_color;
}