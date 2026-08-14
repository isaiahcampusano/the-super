#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;
layout (location = 2) in float size;

uniform mat4 view;
uniform mat4 projection;

out vec4 particleColor;

void main() {
    particleColor = color;
    gl_PointSize = size;
    gl_Position = projection * view * vec4(position, 1.0);
}
