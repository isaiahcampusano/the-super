#version 330 core

uniform float intensity;
out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(1.0, 0.98, 0.90, intensity);
}
