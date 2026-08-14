#version 330 core

in vec4 particleColor;
out vec4 fragmentColor;

void main() {
    vec2 centered = (gl_PointCoord * 2.0) - 1.0;
    float radiusSquared = dot(centered, centered);
    if (radiusSquared > 1.0) {
        discard;
    }
    float glow = 1.0 - smoothstep(0.05, 1.0, radiusSquared);
    fragmentColor = vec4(particleColor.rgb, particleColor.a * glow);
}
