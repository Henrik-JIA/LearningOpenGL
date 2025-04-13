#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	mat4 mvp = projection * view * model;
	// mat4 mvp = model;

	TexCoords = aTexCoords;

	// 这里依然是三维坐标，但是z坐标为0，w坐标为1
	gl_Position = mvp * vec4(aPos, 0.0, 1.0);
}

