#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// 声明一个uniform sampler2D把一个纹理添加到片段着色器中，稍后我们会把纹理赋值给这个uniform
uniform sampler2D ourTexture;

void main()
{
    // FragColor = vec4(ourColor, 1.0);
    FragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0);
}