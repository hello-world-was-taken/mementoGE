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

    // NOTE:
    // We intentionally avoid indexing like:
    //     texture(textures[v_tex_slot], uv)
    // Because on some hardware the sampler index must be a compile-time
    // constant or "dynamically uniform". See: https://stackoverflow.com/a/74729081

    switch (v_tex_slot)
    {
        case -1: texColor = v_color; break; // sprites with no texture
        case 0:  texColor = texture(textures[0], v_texture_coordinate); break;
        case 1:  texColor = texture(textures[1], v_texture_coordinate); break;
        case 2:  texColor = texture(textures[2], v_texture_coordinate); break;
        case 3:  texColor = texture(textures[3], v_texture_coordinate); break;
        case 4:  texColor = texture(textures[4], v_texture_coordinate); break;
        case 5:  texColor = texture(textures[5], v_texture_coordinate); break;
        case 6:  texColor = texture(textures[6], v_texture_coordinate); break;
        case 7:  texColor = texture(textures[7], v_texture_coordinate); break;
        case 8:  texColor = texture(textures[8], v_texture_coordinate); break;
        case 9:  texColor = texture(textures[9], v_texture_coordinate); break;
        case 10: texColor = texture(textures[10], v_texture_coordinate); break;
        case 11: texColor = texture(textures[11], v_texture_coordinate); break;
        case 12: texColor = texture(textures[12], v_texture_coordinate); break;
        case 13: texColor = texture(textures[13], v_texture_coordinate); break;
        case 14: texColor = texture(textures[14], v_texture_coordinate); break;
        case 15: texColor = texture(textures[15], v_texture_coordinate); break;
        default:
            texColor = v_color;
            break;
    }

    // discard transparent textures
    if(texColor.a < 0.1)
        discard;

    FragColor = texColor * v_color;
}