#version 330

in vec3 fragNormal;
in vec3 fragPosition;

out vec4 finalColor;

uniform vec3 lightDir = vec3(1.0, 2.0, 0.5);

void main()
{
    // Simple diffuse lighting
    vec3 norm = normalize(fragNormal);
    vec3 light = normalize(lightDir);
    float diff = max(dot(norm, light), 0.2);  // 0.3 = ambient
    
    vec3 waterColor = vec3(0.15, 0.35, 0.6);  // Dark blue
    vec3 color = waterColor * diff;
    
    finalColor = vec4(color, 1.0);
}