#version 330 core

layout (location = 0) out vec4 FragColor;

in vec4 v_color;
in vec2 v_texture_coordinate;
flat in int v_tex_slot;

uniform vec4 our_color;
uniform sampler2D textures[16];

void main()
{
    vec4 texColor;
    if (v_tex_slot == -1) {
        // Use only vertex color (for colliders or non-textured geometry)
        FragColor = v_color;
        return;
    } else if (v_tex_slot > 15) {
        FragColor = v_color; // fallback
        return;
    }

    texColor = texture(textures[v_tex_slot], v_texture_coordinate);
    // discard transparent textures
    if(texColor.a < 0.1)
        discard;

    FragColor = texColor * v_color;
}