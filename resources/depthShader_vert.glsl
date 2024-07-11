#version 330 core

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNor;
layout (location = 2) in vec2 verTex;

out vec2 texCoords;

out VS_OUT {
    vec3 fragPos;
    vec3 nor;
    vec2 texCoords;
    vec4 fragPosLightSpace;
} vs_out;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 lightSpaceMat;

void main()
{
    vs_out.fragPos = vec3(M * vec4(vertPos, 1.0));
    vs_out.nor = transpose(inverse(mat3(M))) * vertNor;
    //vs_out.nor = (V * M * vec4(vertNor, 0.0)).xyz;
    vs_out.texCoords = verTex;
    vs_out.fragPosLightSpace = lightSpaceMat * vec4(vs_out.fragPos, 1.0);
    gl_Position = P * V * M * vec4(vertPos, 1.0);
}
