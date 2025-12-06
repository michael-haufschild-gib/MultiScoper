#version 330 core
in float vDistFromCenter;

uniform vec4 baseColor;
uniform float opacity;
uniform float glowIntensity;
uniform float geometryScale;

out vec4 fragColor;

// Convert sRGB to linear color space
// Input colors from juce::Colour are in sRGB, but we need linear for correct
// blending and tonemapping in the rendering pipeline
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
    // Add small epsilon to prevent singularity
    float glow = pow(glowRadius / (d + 0.05), glowFalloff);

    glow *= glowIntensity;

    // --- Hot Core ---
    // Thinner core, sharp falloff
    float coreThickness = 0.2;
    float core = 1.0 - smoothstep(0.0, coreThickness, d);
    core = pow(core, 4.0); // Sharpen the core peak

    // Core Brightness (HDR)
    // We use a very high value so it survives the bloom threshold easily
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
    vec3 finalColor = hotCore + glowColor;

    // --- Output ---
    // Pre-multiply opacity for GL_ONE, GL_ONE blending
    // Output in linear space - blit shader will tonemap and apply gamma
    fragColor = vec4(finalColor * opacity, 1.0);
}
