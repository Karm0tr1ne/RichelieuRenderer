#version 450

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 positionWS;
layout (location = 2) in vec3 normalWS;
layout (location = 3) in vec4 tangentWS;

layout (binding = 0) uniform UniformBufferObjects {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 camPos;
};
layout (binding = 1) uniform UBOParams {
    vec4 lightPos;
    float exposure;
    float gamma;
} uboParams;
layout (binding = 2) uniform sampler2D mainTex;
layout (binding = 3) uniform sampler2D normalMap;
layout (binding = 4) uniform sampler2D metallicMap;
layout (binding = 5) uniform sampler2D roughnessMap;
layout (binding = 6) uniform sampler2D occlusionMap;
layout (binding = 7) uniform sampler2D emissionMap;

const float PI = 3.14159265359;

vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 fresnelSchlick(float cos, vec3 F0){
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos, 0.0, 1.0), 5.0);
}

float distribution(vec3 normal, vec3 halfVector, float roughness) {
    float a2 = roughness * roughness;
    float nh = max(dot(normal, halfVector), 0.0);
    float nh2 = nh * nh;
    float down = nh2 * (a2 - 1.0) + 1;
    down = PI * down * down;

    return a2 / down;
}

float geometryGGX(float nv, float k) {
    float down = nv * (1.0 - k) + k;
    return nv / down;
}

float geometry(float nv, float nl, float k) {
    float ggx1 = geometryGGX(nv, k);
    float ggx2 = geometryGGX(nl, k);
    return ggx1 * ggx2;
}

void main(){
    vec3 albedo = texture(mainTex, uv).rgb;
    float metallic = texture(metallicMap, uv).r;
    float roughness = texture(roughnessMap, uv).r;
    float occlusion = texture(occlusionMap, uv).r;
    vec3 emission = texture(emissionMap, uv).rgb;
    emission *= 0.5f;
    // vec3 light = normalize(uboParams.lightPos.xyz - positionWS);
    vec3 light = normalize(vec3(-15.0f, -7.5f, 15.0f) - positionWS);
    vec3 normalTS = texture(normalMap, uv).rgb;
    vec3 q1 = dFdx(positionWS);
    vec3 q2 = dFdy(positionWS);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);
    vec3 n = normalize(normalTS);
    vec3 t = normalize(q1 * st2.t - q2 * st1.t);
    vec3 b = normalize(cross(n, t));
    mat3 tbn = mat3(t, b, n);
    vec3 normal = normalize(tbn * normalTS);

    vec3 viewPos = normalize(camPos - positionWS);
    vec3 h = normalize(viewPos + light);
    float nv = max(dot(normal, viewPos), 0.0);
    float nl = max(dot(normal, light), 0.0);
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    float NDF = distribution(normal, h, roughness);
    float k = (roughness + 1) * (roughness + 1) / 8;
    float G = geometry(nv, nl, k);
    vec3 F = fresnelSchlick(max(dot(h, viewPos), 0.0), f0);

    float down = 4 * nv * nl + 0.001;
    vec3 specular = NDF * G * F / down;

    vec3 kd = 1.0 - F;
    kd *= 1.0 - metallic;
    float distance = length(uboParams.lightPos.xyz - positionWS);
    float attenuation = 1.0 / (distance * distance);
    vec3 lightColor = vec3(0.0f, 0.0f, 0.0f);
    vec3 radiance = lightColor * attenuation;
    vec3 Lo = (kd * albedo / PI + specular) * nl;// * radiance;
    vec3 ambient = vec3(0.03) * albedo * occlusion;
    vec3 color = emission + Lo + ambient;

    color = Uncharted2Tonemap(color * uboParams.exposure);
    // gamma correction
    color = pow(color, vec3(1.0 / uboParams.gamma));
    FragColor = vec4(color, 1.0f);
}