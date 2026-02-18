#version 330 core
// ============================================================================
// Basic Waveform Vertex Shader
// Simple passthrough for position and distance-from-center for glow effects
// Part of Oscil audio visualization plugin
//
// GPU COMPATIBILITY: OpenGL 3.3 Core Profile
// - Intel HD Graphics 2500+ (2012+): SUPPORTED
// - AMD/NVIDIA discrete GPUs (2008+): SUPPORTED
// - macOS 10.9+: SUPPORTED
// ============================================================================

in vec2 position;
in float distFromCenter;

uniform mat4 projection;

out float vDistFromCenter;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vDistFromCenter = distFromCenter;
}
