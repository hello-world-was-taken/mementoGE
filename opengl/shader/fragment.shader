#version 330 core

layout (location = 0) out vec4 FragColor;
uniform vec4 our_color;

void main()
{
    FragColor = our_color;
}