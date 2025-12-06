#version 330 core
in vec2 position;
in float distFromCenter;
in float tParam;

uniform mat4 projection;
uniform float time;

out float vDistFromCenter;

float rand(float n) { return fract(sin(n) * 43758.5453123); }

void main()
{
    vec2 pos = position;
    
    // Glitch logic
    // Create random blocky offsets
    float blockIdx = floor(tParam * 30.0);
    float r = rand(blockIdx + floor(time * 10.0));
    
    if (r > 0.9) {
        // Y offset
        pos.y += (rand(r) - 0.5) * 100.0;
        // X offset
        pos.x += (rand(r + 1.0) - 0.5) * 20.0;
    }

    gl_Position = projection * vec4(pos, 0.0, 1.0);
    vDistFromCenter = distFromCenter;
}
