#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;

uniform vec3 cameraPosition;
uniform vec3 baseColor;

out vec4 fragmentColor;

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(vec3(0.8, 1.2, 1.0));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);
    vec3 halfVector = normalize(lightDirection + viewDirection);
    float specular = pow(max(dot(normal, halfVector), 0.0), 32.0);
    vec3 color = baseColor * (0.22 + 0.78 * diffuse) + vec3(1.0, 0.65, 0.35) * specular;
    fragmentColor = vec4(color, 1.0);
}
