#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec3 ourPos;
in vec2 TexCoord;

uniform float factor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    // FragColor = vec4(ourColor, 1.0);
    // FragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0);
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), abs(sin(factor)));

    // float xy = length(ourPos.xy);
    // FragColor = mix(texture(texture1, TexCoord), vec4(ourColor, 1.0 - xy * 2.0) * texture(texture2, TexCoord), 0.1);
}