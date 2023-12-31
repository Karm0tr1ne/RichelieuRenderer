#version 450

layout (binding = 0) uniform UniformBufferObjects {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 camPos;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec2 uv;
layout (location = 1) out vec3 positionWS;
layout (location = 2) out vec3 normalWS;
layout (location = 3) out vec4 tangentWS;

void main(){
    uv = texCoord;
    positionWS = vec3(ubo.model * vec4(inPosition, 1.0));
    normalWS = mat3(ubo.model) * normal;
    tangentWS = vec4(mat3(ubo.model) * tangent.xyz, tangent.w);
    gl_Position = ubo.proj * ubo.view * vec4(positionWS, 1.0);
}