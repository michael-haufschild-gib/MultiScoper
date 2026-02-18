#version 330 core
// ============================================================================
// Neon Glow Vertex Shader
// Renders waveform with HDR neon glow effect (hot white core + colored halo)
// Part of Oscil audio visualization plugin
//
// GPU COMPATIBILITY: OpenGL 3.3 Core Profile
// - Intel HD Graphics 2500+ (2012+): SUPPORTED
// - AMD/NVIDIA discrete GPUs (2008+): SUPPORTED
// - macOS 10.9+: SUPPORTED
// ============================================================================

in vec2 position;
in float distFromCenter;
// Note: 't' (horizontal position along wave) was removed as unused - this shader
// only needs distance from center for radial glow falloff calculation

uniform mat4 projection;

out float vDistFromCenter;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vDistFromCenter = distFromCenter;
}
