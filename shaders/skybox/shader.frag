#version 450

layout (binding = 1) uniform UBOParams{
    vec4 lightPos;
    float exposure;
    float gamma;
} uboParams;

layout (binding = 2) uniform samplerCube envCube;

layout (location = 0) in vec3 uv;

layout (location = 0) out vec4 FragColor;

vec3 Uncharted2Tonemap(vec3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

void main() {
    vec3 color = texture(envCube, uv).rgb;

    color = Uncharted2Tonemap(color * uboParams.exposure);
    color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    color = pow(color, vec3(1.0f / uboParams.gamma));

    FragColor = vec4(240.0f / 255.0f, 1.0f, 1.0f, 1.0f);
    // FragColor = vec4(color, 1.0f);
}