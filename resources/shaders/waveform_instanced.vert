#version 330 core

// ============================================================================
// Instanced Waveform Vertex Shader
// Renders multiple waveforms with a single draw call using gl_InstanceID
// Part of Tier 2 rendering (OpenGL 3.3+)
//
// GPU COMPATIBILITY NOTES:
// - Intel HD Graphics 2500/4000 (Ivy Bridge, 2012): OpenGL 4.0 - SUPPORTED
// - Intel HD Graphics 4400+ (Haswell, 2013+): OpenGL 4.3+ - SUPPORTED
// - Intel GMA (pre-2010): OpenGL 2.1 only - NOT SUPPORTED (use software fallback)
// - AMD/NVIDIA discrete GPUs (2008+): Generally OpenGL 3.3+ - SUPPORTED
// - macOS: OpenGL 3.3 Core Profile available on 10.9+ - SUPPORTED
//
// This shader is the recommended fallback for systems that don't support
// OpenGL 4.3+ (required for SSBO-based Tier 3 rendering).
// ============================================================================

// Maximum instances supported (must match kMaxInstancedWaveforms in C++)
#define MAX_INSTANCES 64

// Per-vertex attributes (from VBO, same as basic shader)
in vec2 position;
in float distFromCenter;

// Projection matrix (shared across all instances)
uniform mat4 projection;

// Per-instance data from Uniform Buffer Object (UBO)
// UBO layout must match InstanceDataUBO struct in C++
//
// std140 LAYOUT ALIGNMENT (for C++ struct):
//   - vec4 aligns to 16 bytes
//   - vec4[N] has 16-byte stride (each element aligned to 16 bytes)
//   - Total size: 4 arrays * 64 elements * 16 bytes = 4096 bytes
//   - Guaranteed within GL_MAX_UNIFORM_BLOCK_SIZE (min 16KB per OpenGL 3.1 spec)
//
// C++ struct must match:
//   struct alignas(16) InstanceDataUBO {
//       float transforms[64][4];  // 1024 bytes at offset 0
//       float colors[64][4];      // 1024 bytes at offset 1024
//       float params[64][4];      // 1024 bytes at offset 2048
//       float viewports[64][4];   // 1024 bytes at offset 3072
//   };  // Total: 4096 bytes
//
layout(std140) uniform InstanceData {
    // Transform: x-offset, y-offset, x-scale, y-scale (16 bytes each, 1024 total)
    vec4 transforms[MAX_INSTANCES];

    // Color: r, g, b, a (16 bytes each, 1024 total)
    vec4 colors[MAX_INSTANCES];

    // Params: opacity, lineWidth, glowIntensity, reserved (16 bytes each, 1024 total)
    vec4 params[MAX_INSTANCES];

    // Viewport: x, y, width, height normalized 0-1 (16 bytes each, 1024 total)
    vec4 viewports[MAX_INSTANCES];
};

// Outputs to fragment shader
out float vDistFromCenter;
out vec4 vColor;
out float vOpacity;
out float vGlowIntensity;

void main()
{
    // Bounds check to prevent out-of-bounds UBO array access
    // If gl_InstanceID exceeds MAX_INSTANCES, cull the vertex
    if (gl_InstanceID >= MAX_INSTANCES) {
        gl_Position = vec4(0.0);
        vDistFromCenter = 0.0;
        vColor = vec4(0.0);
        vOpacity = 0.0;
        vGlowIntensity = 0.0;
        return;
    }

    // Get per-instance data using gl_InstanceID
    vec4 transform = transforms[gl_InstanceID];
    vec4 color = colors[gl_InstanceID];
    vec4 param = params[gl_InstanceID];
    vec4 viewport = viewports[gl_InstanceID];
    
    // Apply per-instance transform to position
    // transform.xy = offset, transform.zw = scale
    vec2 transformedPos = position * transform.zw + transform.xy;
    
    // Apply viewport transform (if viewport is non-zero)
    if (viewport.z > 0.0 && viewport.w > 0.0) {
        // Map position to viewport
        transformedPos.x = viewport.x + transformedPos.x * viewport.z;
        transformedPos.y = viewport.y + transformedPos.y * viewport.w;
    }
    
    // Apply projection
    gl_Position = projection * vec4(transformedPos, 0.0, 1.0);
    
    // Pass through to fragment shader
    vDistFromCenter = distFromCenter;
    vColor = color;
    vOpacity = param.x;
    vGlowIntensity = param.z;
}













