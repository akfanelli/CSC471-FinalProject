#version 330 core
uniform sampler2D Texture0;
uniform int flip;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 lightDir;
out vec4 Outcolor;

void main() {
	vec4 texColor0 = texture(Texture0, vTexCoord);

	vec3 normal = normalize(fragNor);
	if (flip == 1) {
		normal = normal * -1.0f;
	}
	vec3 light = normalize(lightDir);
	float dC = max(0, dot(normal, light));
	float lightScale = 0.3 + dC;
	//to set the out color as the texture color
	Outcolor = vec4(texColor0.rgb * lightScale, texColor0.a);
}

