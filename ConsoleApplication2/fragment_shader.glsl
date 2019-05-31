#version 330 core

out vec4 outColor;
in vec3 color;
in vec2 vuv;
uniform sampler2D texture;

void main(void)
{
    outColor = vec4(color,1.0) * texture2D(texture, vuv);
}