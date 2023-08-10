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

void main() {
    gl_Position = ubo.projection * ubo.view * meshConstants.model * vec4(inPos, 1.0);
}