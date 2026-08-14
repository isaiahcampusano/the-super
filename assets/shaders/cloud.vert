#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in float intensity;
layout (location = 2) in float group;

uniform mat4 view;
uniform mat4 projection;
uniform float pointSize;
uniform float intensityCeiling;

out float pointIntensity;
out float pointGroup;

void main() {
    gl_Position = projection * view * vec4(position, 1.0);
    gl_PointSize = pointSize;
    pointIntensity = clamp(
        log(1.0 + 99.0 * max(intensity, 0.0) / max(intensityCeiling, 1.0e-6))
            / log(100.0),
        0.0,
        1.0
    );
    pointGroup = group;
}
