#version 120

uniform sampler2D texture;
uniform sampler2D palette;

void main()
{
	vec4 pixel = texture2D(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y));
	gl_FragColor = texture2D(palette, vec2(pixel.r, 0));
}
