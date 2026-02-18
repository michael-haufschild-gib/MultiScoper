#version 430 core

// ============================================================================
// SSBO-based Waveform Vertex Shader
// Uses Shader Storage Buffer Objects for unlimited waveform count (Tier 3)
// Requires OpenGL 4.3+
//
// GPU COMPATIBILITY NOTES:
// - Intel HD Graphics 4000+ (Ivy Bridge, 2012): OpenGL 4.0 only - NOT SUPPORTED
// - Intel HD Graphics 4400+ (Haswell, 2013): OpenGL 4.3 - SUPPORTED (limited SSBO size)
// - Intel HD Graphics 5xxx+ (Broadwell, 2015+): OpenGL 4.4/4.5 - SUPPORTED
// - Intel UHD Graphics 6xx+ (2018+): OpenGL 4.6 - FULL SUPPORT
// - AMD/NVIDIA discrete GPUs (2012+): Generally OpenGL 4.3+ - SUPPORTED
//
// FALLBACK: For Intel HD 4000 and older integrated GPUs, use waveform_instanced.vert
// (Tier 2, OpenGL 3.3) or BasicShader (Tier 1, immediate upload).
// ============================================================================

// Per-vertex attributes (from VBO)
in vec2 position;
in float distFromCenter;

// Shared projection matrix
uniform mat4 projection;

// Instance index: Set by C++ code before each draw call.
// NOTE: Although this shader supports SSBO-based multi-instance data storage,
// rendering is done per-waveform (one draw call per waveform) rather than true
// hardware instancing (glDrawArraysInstanced). This design allows per-waveform
// viewport clipping and custom draw order. For true instancing, use gl_InstanceID
// with glDrawArraysInstanced and remove this uniform.
uniform int instanceIndex;

// Waveform configuration structure - must match C++ WaveformConfigSSBO
struct WaveformConfig {
    mat4 transform;      // Full 4x4 transform matrix
    vec4 color;          // RGBA color
    vec4 params;         // opacity, lineWidth, glowIntensity, shaderType
    vec4 viewport;       // x, y, width, height
};

// SSBO containing all waveform configurations
// This allows unlimited waveforms (limited only by GPU memory)
layout(std430, binding = 0) readonly buffer ConfigBuffer {
    WaveformConfig configs[];
};

// Outputs to fragment shader
out float vDistFromCenter;
out vec4 vColor;
out float vOpacity;
out float vGlowIntensity;

void main()
{
    // Get configuration for this instance
    WaveformConfig config = configs[instanceIndex];
    
    // Apply transform
    gl_Position = projection * config.transform * vec4(position, 0.0, 1.0);
    
    // Pass through to fragment shader
    vDistFromCenter = distFromCenter;
    vColor = config.color;
    vOpacity = config.params.x;
    vGlowIntensity = config.params.z;
}













