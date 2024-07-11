#version 330 core

layout (location = 0) in vec3 vertPos;

uniform mat4 lightSpaceMat;
uniform mat4 M;

void main()
{
    gl_Position = lightSpaceMat * M * vec4(vertPos, 1.0);
}
