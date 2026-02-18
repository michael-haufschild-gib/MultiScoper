#version 430 core

// ============================================================================
// SSBO-based Waveform Fragment Shader
// Same as instanced fragment shader, uses per-instance data from vertex shader
// Part of Tier 3 rendering (OpenGL 4.3+)
//
// GPU COMPATIBILITY: OpenGL 4.3+ (SSBO support required)
// - Intel HD Graphics 4400+ (Haswell, 2013): SUPPORTED
// - Intel HD Graphics 5xxx+ (Broadwell, 2015+): SUPPORTED
// - Intel UHD Graphics 6xx+ (2018+): FULL SUPPORT
// - AMD/NVIDIA discrete GPUs (2012+): SUPPORTED
//
// FALLBACK: For Intel HD 4000 and older, use waveform_instanced.frag (Tier 2)
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
    // Convert sRGB input color to linear space
    vec3 neonColor = sRGBToLinear(vColor.rgb);

    float dist = abs(vDistFromCenter);

    // 80s Neon effect
    float core = 1.0 - smoothstep(0.0, 0.08, dist);
    float glow = exp(-dist * 6.0) * vGlowIntensity;

    // Note: coreColor can exceed 1.0 (HDR) - this is intentional for bloom
    vec3 coreColor = neonColor * 1.5;  // Brighten core (may exceed 1.0)
    vec3 glowColor = neonColor;

    vec3 finalColor = mix(glowColor * glow, coreColor, core);
    float alpha = vOpacity * (core + glow * 0.7) * vColor.a;

    // Output in linear space - blit shader will tonemap and apply gamma
    // Note: finalColor may exceed 1.0 for HDR bloom effect
    fragColor = vec4(finalColor, alpha);
}













