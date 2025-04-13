#version 330 core
out vec4 FragColor;

// 传入的纹理坐标
in vec2 TexCoords;

// 将之前FBO渲染的纹理传入
uniform sampler2D screenTexture;

void main() {
	// 采样纹理
	vec3 col = texture(screenTexture, TexCoords).rgb;
	
	// 输出颜色
	FragColor = vec4(col, 1.0);
}

