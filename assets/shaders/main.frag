#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(push_constant, std430) uniform pc {
    float x;
    float y;
    float time;
};

void main() {
    outColor = vec4(fragColor.xy, (sin(time) + 1.0) / 2.0, 1.0);
}
