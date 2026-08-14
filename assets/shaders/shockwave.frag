#version 330 core

uniform vec3 color;
uniform float alpha;
out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(color, alpha);
}
