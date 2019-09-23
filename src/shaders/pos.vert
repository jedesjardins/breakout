#version 450

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 transform;
} constants;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
} ubo;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = ubo.view * constants.transform * vec4(inPosition, 1.0);
}