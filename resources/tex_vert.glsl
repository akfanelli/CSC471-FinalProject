#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;
uniform vec3 lightPos;

out vec2 vTexCoord;
out vec3 fragNor;
out vec3 lightDir;

void main() {

    vec4 vPosition;

    /* First model transforms */
    gl_Position = P * V *M * vec4(vertPos.xyz, 1.0);

    fragNor = (V*M * vec4(vertNor, 0.0)).xyz;
    lightDir = vec3(V*(vec4(lightPos - (M*vertPos).xyz, 0.0)));

    /* pass through the texture coordinates to be interpolated */
    vTexCoord = vertTex;
}
