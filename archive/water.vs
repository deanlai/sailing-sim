#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform float time;

out vec3 fragNormal;
out vec3 fragPosition;

void main()
{
    vec3 pos = vertexPosition;
    
    // Wave displacement
    float wave1 = sin(pos.x * 0.4 + time * 2.0) * 0.6;  // ~8m wavelength
    float wave2 = cos(pos.z * 0.4 + time * 2.0) * 0.6;
    pos.y = wave1 + wave2;

    
    float dHdx = cos(pos.x * 0.4 + time * 2.0) * 0.8 * 0.6;
    float dHdz = -sin(pos.z * 0.4 + time * 2.0) * 0.6 * 0.6;
    vec3 normal = normalize(vec3(-dHdx, 1.0, -dHdz));
    
    fragNormal = normal;
    fragPosition = pos;
    gl_Position = mvp * vec4(pos, 1.0);
}