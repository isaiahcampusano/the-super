#version 330 core

in float pointIntensity;
in float pointGroup;

out vec4 fragmentColor;

void main() {
    vec2 centered = (gl_PointCoord * 2.0) - 1.0;
    float radiusSquared = dot(centered, centered);
    if (radiusSquared > 1.0) {
        discard;
    }

    float softEdge = 1.0 - smoothstep(0.20, 1.0, radiusSquared);
    vec3 proton = vec3(0.96, 0.18, 0.12);
    vec3 neutron = vec3(0.18, 0.48, 1.00);
    vec3 color = mix(proton, neutron, step(0.5, pointGroup));
    color = mix(color * 0.45, color, pointIntensity);
    fragmentColor = vec4(color, 0.72 * softEdge);
}
