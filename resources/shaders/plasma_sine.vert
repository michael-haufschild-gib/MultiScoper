#version 330 core
in vec2 position;
in float vParam; // -1 to 1 across width
in float tParam; // 0 to 1 along length

uniform mat4 projection;
uniform float time;
uniform float jitterAmount;
uniform float jitterSpeed;

out vec2 vPos;
out float vV;
out float vT;

// Simple pseudo-random noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), f.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

void main()
{
    vec2 pos = position;
    vPos = pos;
    vV = vParam;
    vT = tParam;

    // Apply Jitter (Vertex Displacement)
    // We displace based on the 'tParam' (position along wave) so the whole
    // cross-section moves together (keeping the ribbon intact but wavy).
    if (jitterAmount > 0.0) {
        float n = noise(vec2(tParam * 10.0, time * jitterSpeed));
        // Map 0..1 to -1..1
        float displacement = (n - 0.5) * 2.0;
        
        // Displace perpendicular to the wave? 
        // For simplicity, just vertical displacement is often enough for audio waves,
        // but 2D displacement is better.
        // Let's just add to Y for now as it aligns with the audio amplitude.
        pos.y += displacement * jitterAmount;
    }

    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
