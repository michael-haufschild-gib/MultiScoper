#version 330 core
precision highp float;
// ============================================================================
// Neon Glow Fragment Shader
// Renders intense HDR neon effect with hot white core and colored glow halo
// Part of Oscil audio visualization plugin
//
// GPU COMPATIBILITY: OpenGL 3.3 Core Profile
// - Intel HD Graphics 2500+ (2012+): SUPPORTED
// - AMD/NVIDIA discrete GPUs (2008+): SUPPORTED
// - macOS 10.9+: SUPPORTED
//
// HDR HANDLING: This shader outputs HIGH DYNAMIC RANGE values up to ~8.0
// in the core for intense bloom effects. The blit shader MUST apply
// tonemapping before display, or colors will clip severely on SDR displays.
// Recommended tonemapping: Reinhard or ACES
// ============================================================================

in float vDistFromCenter;

uniform vec4 baseColor;
uniform float opacity;
uniform float glowIntensity;
uniform float geometryScale;

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
    float d = dist * geometryScale;

    // --- Hyperbolic Glow (Soft Electric) ---
    // Falloff < 1.0 makes the tail very long (gaseous)
    float glowRadius = 0.25;
    float glowFalloff = 0.9;
    // Add small epsilon to prevent singularity (division by near-zero)
    float glow = pow(glowRadius / (d + 0.05), glowFalloff);

    glow *= glowIntensity;

    // --- Hot Core ---
    // Thinner core, sharp falloff
    float coreThickness = 0.2;
    float core = 1.0 - smoothstep(0.0, coreThickness, d);
    core = pow(core, 4.0); // Sharpen the core peak

    // Core Brightness (HDR)
    // We use a very high value (8.0) so it survives the bloom threshold easily
    // WARNING: This produces values WAY above 1.0 - tonemapping is REQUIRED!
    vec3 coreColor = vec3(8.0) * core;

    // --- Color Mixing ---
    // Convert sRGB input to linear for correct color operations
    vec3 rgb = sRGBToLinear(baseColor.rgb);

    // The core should be "hot" (desaturated towards white) but still retain tint
    // Mix 30% color, 70% white for the core
    vec3 hotCore = mix(vec3(1.0), rgb, 0.3) * coreColor;

    // The glow is pure color
    vec3 glowColor = rgb * glow;

    // Combine (HDR output in linear space)
    // Final values can reach ~8.0 in the core - this is intentional for bloom
    vec3 finalColor = hotCore + glowColor;

    // --- Output ---
    // Pre-multiply opacity for GL_ONE, GL_ONE (additive) blending
    // Output in linear HDR space - blit shader will tonemap and apply gamma
    // Note: Values exceeding 1.0 are expected and necessary for proper bloom
    fragColor = vec4(finalColor * opacity, 1.0);
}
