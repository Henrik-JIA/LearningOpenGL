#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 viewPos;

void main()
{
	
	if(distance(TexCoords,vec2(0.5,0.5))<0.2)
		FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	else
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}

