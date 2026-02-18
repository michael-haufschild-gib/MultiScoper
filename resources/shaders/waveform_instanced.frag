#version 330 core

// ============================================================================
// Instanced Waveform Fragment Shader
// Uses per-instance color and parameters from vertex shader
// Part of Tier 2 rendering (OpenGL 3.3+)
//
// GPU COMPATIBILITY: OpenGL 3.3 Core Profile
// - Intel HD Graphics 2500+ (Ivy Bridge, 2012): SUPPORTED
// - Intel HD Graphics 4400+ (Haswell, 2013+): SUPPORTED
// - AMD/NVIDIA discrete GPUs (2008+): SUPPORTED
// - macOS 10.9+: SUPPORTED
//
// HDR HANDLING: This shader outputs values in [0, ~2.0] range for the glow
// effect. The blit shader is responsible for tonemapping before display.
// ============================================================================

// Inputs from vertex shader
in float vDistFromCenter;
in vec4 vColor;
in float vOpacity;
in float vGlowIntensity;

// Output
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
    // Convert sRGB input color to linear space for correct rendering
    vec3 neonColor = sRGBToLinear(vColor.rgb);

    float dist = abs(vDistFromCenter);

    // 80s Neon effect: thin bright core with colored glow halo
    // Core line - very thin bright center
    float core = 1.0 - smoothstep(0.0, 0.08, dist);

    // Glow falloff - smooth exponential decay for the halo
    float glow = exp(-dist * 6.0) * vGlowIntensity;

    // The core is slightly brighter/saturated version of the color
    // The glow keeps the color but fades out
    // Note: coreColor can exceed 1.0 (HDR) - this is intentional for bloom
    vec3 coreColor = neonColor * 1.5;  // Brighten core (may exceed 1.0)
    vec3 glowColor = neonColor;         // Glow stays true to color

    // Mix: bright core fading into colored glow
    vec3 finalColor = mix(glowColor * glow, coreColor, core);

    // Alpha: solid core, fading glow
    float alpha = vOpacity * (core + glow * 0.7) * vColor.a;

    // Output in linear space - blit shader will tonemap and apply gamma
    // Note: finalColor may exceed 1.0 for HDR bloom effect
    fragColor = vec4(finalColor, alpha);
}













