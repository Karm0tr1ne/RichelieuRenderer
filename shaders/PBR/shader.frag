#version 450

layout(position = 0) out vec4 FragColor;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalMap;

uniform vec3 lightPosition;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cos, vec3 F0){
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos, 0.0, 1.0), 5.0);
}

void main(){

}