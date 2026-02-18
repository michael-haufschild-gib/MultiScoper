#version 330 core
// ============================================================================
// Gradient Fill Vertex Shader
// Renders filled waveform with vertical gradient from waveform to baseline
// Part of Oscil audio visualization plugin
//
// GPU COMPATIBILITY: OpenGL 3.3 Core Profile
// - Intel HD Graphics 2500+ (2012+): SUPPORTED
// - AMD/NVIDIA discrete GPUs (2008+): SUPPORTED
// - macOS 10.9+: SUPPORTED
// ============================================================================

in vec2 position;
in float vParam; // 0 at baseline, 1 at waveform (used for gradient interpolation)
// Note: tParam (horizontal position) was removed as unused - shader only needs
// vertical gradient information, not horizontal position along wave

uniform mat4 projection;

out float vV;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vV = vParam;
}
