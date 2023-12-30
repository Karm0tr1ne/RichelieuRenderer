#version 450

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform sampler2D mainTex;

void main(){
    FragColor = texture(mainTex, uv);
}