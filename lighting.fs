#version 330

in vec3 fragNormal;
in vec3 fragPos;

out vec4 finalColor;

uniform vec3 lightDir = vec3(-0.5, -1.0, -0.3);
uniform vec4 colDiffuse;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 light = normalize(-lightDir);  // Negate to get direction TO light
    
    float diff = max(dot(normal, light), 0.0);  // 0.3 = ambient
    
    vec3 color = colDiffuse.rgb * diff * 2.0;
    finalColor = vec4(color, colDiffuse.a);
}