#version 450

layout(binding = 0) uniform UniformBufferObjects {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(position = 0) in vec3 inPosition;
layout(position = 2) in vec2 texCoord;

layout(position = 0) out vec2 uv;
layout(position = 1) out vec3 positionWS;
layout(position = 2) out vec3 normalWS;

void main(){
    uv = texCoord;
    positionWS = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * positionWS;
}