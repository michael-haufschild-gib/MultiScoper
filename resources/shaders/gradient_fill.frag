#version 330 core
precision highp float;
// ============================================================================
// Gradient Fill Fragment Shader
// Renders filled waveform with vertical gradient from solid to transparent
// Part of Oscil audio visualization plugin
//
// GPU COMPATIBILITY: OpenGL 3.3 Core Profile
// - Intel HD Graphics 2500+ (2012+): SUPPORTED
// - AMD/NVIDIA discrete GPUs (2008+): SUPPORTED
// - macOS 10.9+: SUPPORTED
//
// COLOR SPACE: Outputs linear color values in [0, 1] range (no HDR).
// The blit shader applies gamma correction for display.
// ============================================================================

in float vV;

uniform vec4 baseColor;
uniform float opacity;

out vec4 fragColor;

// Convert sRGB to linear color space for physically correct blending
// This uses the gamma 2.2 approximation which is faster than the full sRGB
// transfer function but slightly less accurate in the toe region (dark values).
// For real-time rendering, this trade-off is acceptable.
//
// Full sRGB formula (not used for performance):
//   if (srgb <= 0.04045) linear = srgb / 12.92;
//   else linear = pow((srgb + 0.055) / 1.055, 2.4);
vec3 sRGBToLinear(vec3 srgb) {
    return pow(srgb, vec3(2.2));
}

void main()
{
    // Convert sRGB input to linear for correct rendering
    vec3 linearColor = sRGBToLinear(baseColor.rgb);

    // Gradient from linearColor (at top) to transparent (at bottom)
    vec4 topColor = vec4(linearColor, baseColor.a);
    vec4 bottomColor = vec4(linearColor, 0.0);

    // Apply opacity
    topColor.a *= opacity;
    bottomColor.a *= opacity;

    // Simple linear interpolation (in linear space)
    // Note: Output is in [0, 1] range - no HDR values produced
    fragColor = mix(bottomColor, topColor, vV);
}
