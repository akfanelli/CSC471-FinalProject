#version 330 core 
in vec3 fragNor;
out vec4 color;

uniform vec3 solidColor;

void main()
{
	color = vec4(solidColor, 1.0);
}
