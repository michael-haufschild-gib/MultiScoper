#version 330 core
precision highp float;
// ============================================================================
// Dual Outline Fragment Shader
// Renders waveform with double-line outline effect (inner + outer)
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

in float vDistFromCenter;

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
    float dist = abs(vDistFromCenter);

    // Inner line: Center to 0.3
    float inner = 1.0 - smoothstep(0.2, 0.3, dist);

    // Outer line: 0.6 to 0.9
    float outer = smoothstep(0.6, 0.7, dist) * (1.0 - smoothstep(0.8, 0.9, dist));

    // Combine - alpha stays in [0, 1] range
    float alpha = (inner + outer * 0.5) * opacity * baseColor.a;

    // Convert sRGB input to linear for correct rendering
    vec3 linearColor = sRGBToLinear(baseColor.rgb);

    // Note: Output is in [0, 1] range - no HDR values produced
    fragColor = vec4(linearColor, alpha);
}
