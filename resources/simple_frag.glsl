#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	vec3 halfV = normalize(EPos + lightDir);
	float dC = clamp(dot(normal, light), 0, 1);
	float sC = pow(max(dot(halfV, normal), 0), MatShine);
	color = vec4(MatAmb + dC*MatDif + sC*MatSpec, 1.0);
}
