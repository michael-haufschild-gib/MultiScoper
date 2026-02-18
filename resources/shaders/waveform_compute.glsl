#version 430 core

// ============================================================================
// Waveform Compute Shader
// Processes audio samples and generates vertex positions on the GPU (Tier 4)
// Requires OpenGL 4.3+
// ============================================================================

layout(local_size_x = 256) in;

// Input: Audio samples (interleaved stereo)
// Layout: samples[waveformId * samplesPerWaveform * 2 + sampleIndex * 2 + channel]
layout(std430, binding = 0) readonly buffer AudioData {
    float samples[];
};

// Output: Vertex positions for rendering
// Layout: vertices[waveformId * verticesPerWaveform + vertexIndex]
// Each vertex is: x, y, distFromCenter, padding
layout(std430, binding = 1) writeonly buffer VertexOutput {
    vec4 vertices[];
};

// Waveform configuration structure
struct WaveformConfig {
    vec4 bounds;         // x, y, width, height (in pixels)
    vec4 color;          // RGBA
    vec4 params;         // opacity, lineWidth, glowIntensity, verticalScale
    vec4 viewport;       // viewportWidth, viewportHeight, reserved, reserved
};

// Configuration buffer
layout(std430, binding = 2) readonly buffer ConfigBuffer {
    WaveformConfig configs[];
};

// Uniforms
uniform int samplesPerWaveform;
uniform int verticesPerWaveform;  // Usually samplesPerWaveform * 2 (for ribbon geometry)
uniform int totalWaveforms;
uniform float lineWidth;

void main()
{
    // Each work group processes one waveform
    uint waveformId = gl_WorkGroupID.x;
    // Each invocation processes multiple samples
    uint localId = gl_LocalInvocationID.x;
    
    if (waveformId >= totalWaveforms)
        return;
    
    WaveformConfig config = configs[waveformId];
    
    // Get bounds and transform info
    float boundsX = config.bounds.x;
    float boundsY = config.bounds.y;
    float boundsWidth = config.bounds.z;
    float boundsHeight = config.bounds.w;
    float verticalScale = config.params.w;
    float viewportWidth = config.viewport.x;
    float viewportHeight = config.viewport.y;
    
    // Process samples in chunks based on local invocation
    uint samplesPerInvocation = (samplesPerWaveform + 255) / 256;
    uint startSample = localId * samplesPerInvocation;
    uint endSample = min(startSample + samplesPerInvocation, uint(samplesPerWaveform));
    
    for (uint sampleIdx = startSample; sampleIdx < endSample; ++sampleIdx)
    {
        // Read sample (left channel only for now)
        uint audioIdx = waveformId * samplesPerWaveform + sampleIdx;
        float amplitude = samples[audioIdx];
        
        // Calculate normalized position within waveform bounds
        // Guard against division by zero when samplesPerWaveform <= 1
        float normalizedX = (samplesPerWaveform > 1u)
            ? float(sampleIdx) / float(samplesPerWaveform - 1u)
            : 0.0;
        
        // Convert to pixel coordinates
        float pixelX = boundsX + normalizedX * boundsWidth;
        float pixelY = boundsY + boundsHeight * 0.5 + amplitude * verticalScale * boundsHeight * 0.5;
        
        // Convert to normalized device coordinates (-1 to 1)
        float ndcX = (pixelX / viewportWidth) * 2.0 - 1.0;
        float ndcY = 1.0 - (pixelY / viewportHeight) * 2.0;  // Flip Y
        
        // For ribbon geometry, we create top and bottom vertices
        // Vertex 0: top of ribbon
        // Vertex 1: bottom of ribbon
        uint vertexBaseIdx = waveformId * verticesPerWaveform + sampleIdx * 2;
        
        float halfWidth = lineWidth / viewportHeight;  // Ribbon half-width in NDC
        
        // Top vertex (+ distFromCenter for gradient)
        vertices[vertexBaseIdx] = vec4(ndcX, ndcY + halfWidth, 1.0, 0.0);
        
        // Bottom vertex (- distFromCenter for gradient)
        vertices[vertexBaseIdx + 1] = vec4(ndcX, ndcY - halfWidth, -1.0, 0.0);
    }
}













