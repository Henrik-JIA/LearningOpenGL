#version 330 core
out vec4 FragColor;
in vec3 outTexCoord;

uniform samplerCube skyboxTexture;

void main() {
  FragColor = vec4(texture(skyboxTexture, outTexCoord).rgb, 1.0);
  // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}