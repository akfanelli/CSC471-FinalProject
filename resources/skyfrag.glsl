#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
in float day_night;

uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
	vec4 tcol = texture(tex, vertex_tex);
	vec4 tcol2 = texture(tex2, vertex_tex);

	color = tcol * day_night + tcol2 * (1 - day_night);
}
