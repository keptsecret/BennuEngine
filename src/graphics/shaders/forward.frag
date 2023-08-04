#version 450

layout (location = 0) out vec4 outColor;

struct PointLight {
    vec4 posr;      // position + radius
    vec4 colori;    // color + intensity
};

struct LightGrid {
    uint count;
    uint offset;
};

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragTexCoord;
layout (location = 3) in vec3 camPos;
layout (location = 4) in float camNear;
layout (location = 5) in float camFar;
layout (location = 6) in mat3 TBN;

layout (set = 0, binding = 1) uniform DirectionalLight {
    vec4 direction;
    vec4 colori;
    vec4 worldData;
} directionalLight;

layout (std430, set = 0, binding = 2) buffer PointLightsBuffer {
    PointLight lights[];
};

layout (std430, set = 0, binding = 3) buffer ClusterGenDataBuffer {
    mat4 inverseProjection;
    uvec4 tileSizes;
    uvec2 screenDims;
    float scale;
    float bias;
};

layout (std430, set = 0, binding = 4) buffer LightIndicesBuffer {
    uint globalLightIndexList[];
};

layout (std430, set = 0, binding = 5) buffer LightGridBuffer {
    LightGrid lightGrid[];
};

layout (set = 1, binding = 0) uniform MaterialAux {
    uint normalMapMode;
    uint roughnessGlossyMode;
} aux;

layout (set = 1, binding = 1) uniform sampler2D albedoSampler;
layout (set = 1, binding = 2) uniform sampler2D metallicSampler;
layout (set = 1, binding = 3) uniform sampler2D roughnessSampler;
layout (set = 1, binding = 4) uniform sampler2D ambientSampler;
layout (set = 1, binding = 5) uniform sampler2D normalSampler;

const float PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness);
float geometrySchlickGGX(float NdotV, float roughness);
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

vec3 linear_to_srgb(vec3 color) {
    return max(vec3(1.055) * pow(color, vec3(0.416666667)) - vec3(0.055), vec3(0.0));
}

float linearDepth(float depth) {
    float range = 2.0 * depth - 1.0;
    return 2.0 * camNear * camFar / (camFar + camNear - range * (camFar - camNear));
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camPos - fragPos);

    vec3 albedo = texture(albedoSampler, fragTexCoord).rgb;
    float metallic = texture(metallicSampler, fragTexCoord).b;
    float roughness = texture(roughnessSampler, fragTexCoord).g;
    roughness = aux.roughnessGlossyMode == 1 ? 1 - roughness : roughness;
    float ao = texture(ambientSampler, fragTexCoord).r;

    if (aux.normalMapMode == 1) {
        N = texture(normalSampler, fragTexCoord).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(TBN * N);
    }

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Directional light
    {
        vec3 L = normalize(-directionalLight.direction.xyz);
        vec3 H = normalize(V + L);
        // skip attenuation
        vec3 radiance = directionalLight.colori.rgb * directionalLight.colori.a;

        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Find cluster grid of fragment
    uint zTile = uint(max(log2(linearDepth(gl_FragCoord.z)) * scale + bias, 0.0));
    uvec3 tile = uvec3(uvec2(gl_FragCoord.xy / tileSizes[3]), zTile);
    uint tileIndex = tile.x + tileSizes.x * tile.y + (tileSizes.x * tileSizes.y) * tile.z;

    uint lightCount = lightGrid[tileIndex].count;
    uint lightIndexOffset = lightGrid[tileIndex].offset;

    // Point lights
    for (int i = 0; i < lightCount; i++) {
        uint lightIdx = globalLightIndexList[lightIndexOffset + i];

        vec3 L = normalize(lights[lightIdx].posr.xyz - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lights[lightIdx].posr.xyz - fragPos);
        float dmr = distance / lights[lightIdx].posr.w;
        float s = clamp(1 - (dmr * dmr * dmr * dmr), 0.0, 1.0);
        float s2 = s * s;
        float attenuation = s2 / (distance * distance + 1);
        vec3 radiance = lights[lightIdx].colori.rgb * lights[lightIdx].colori.a * attenuation;

        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = clamp(color, 0.0, 1.0);
    color = linear_to_srgb(color);

    outColor = vec4(color, 1.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}