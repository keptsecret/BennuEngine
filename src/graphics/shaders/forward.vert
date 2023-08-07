#version 450

layout (set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    vec4 camPos;
    float camNear;
    float camFar;
} ubo;

layout (push_constant) uniform PushConstants {
    mat4 model;
} meshConstants;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragTexCoord;
layout (location = 3) out vec3 camPos;
layout (location = 4) out float camNear;
layout (location = 5) out float camFar;
layout (location = 6) out vec4 alt_FragCoord;
layout (location = 7) out mat3 TBN;

void main() {
    gl_Position = ubo.projection * ubo.view * meshConstants.model * vec4(inPos, 1.0);
    fragPos = vec3(meshConstants.model * vec4(inPos, 1.0));
    fragNormal = vec3(mat4(mat3(meshConstants.model)) * vec4(inNormal, 1.0));
    fragTexCoord = inTexCoord;

    camPos = ubo.camPos.xyz;
    camNear = ubo.camNear;
    camFar = ubo.camFar;

    alt_FragCoord = ubo.projection * ubo.view * meshConstants.model * vec4(inPos, 1.0);
    // Vertex in NDC space
    alt_FragCoord.xyz /= alt_FragCoord.w;
    alt_FragCoord.w = 1 / alt_FragCoord.w;
    // Vertex in window space, multiply by screen dims in fragment shader
    alt_FragCoord.xyz *= vec3(0.5);
    alt_FragCoord.xyz += vec3(0.5);

    vec3 T = normalize(vec3(meshConstants.model * vec4(inTangent, 0.0)));
    vec3 N = normalize(vec3(meshConstants.model * vec4(inNormal, 0.0)));

    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);
}