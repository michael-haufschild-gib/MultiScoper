#version 330 core
in float vDistFromCenter;

uniform vec4 baseColor;
uniform float opacity;
uniform float glowIntensity;

out vec4 fragColor;

// Convert sRGB to linear color space
// Input colors from juce::Colour are in sRGB, but we need linear for correct
// blending and tonemapping in the rendering pipeline
vec3 sRGBToLinear(vec3 srgb) {
    // Approximate sRGB to linear conversion (gamma 2.2)
    return pow(srgb, vec3(2.2));
}

void main()
{
    // Convert sRGB input color to linear space for correct rendering
    vec3 neonColor = sRGBToLinear(baseColor.rgb);

    float dist = abs(vDistFromCenter);

    // 80s Neon effect: thin bright core with colored glow halo
    // Core line - very thin bright center
    float core = 1.0 - smoothstep(0.0, 0.08, dist);

    // Glow falloff - smooth exponential decay for the halo
    float glow = exp(-dist * 6.0) * glowIntensity;

    // The core is slightly brighter/saturated version of the color
    // The glow keeps the color but fades out
    vec3 coreColor = neonColor * 1.5;  // Brighten core
    vec3 glowColor = neonColor;         // Glow stays true to color

    // Mix: bright core fading into colored glow
    vec3 finalColor = mix(glowColor * glow, coreColor, core);

    // Alpha: solid core, fading glow
    float alpha = opacity * (core + glow * 0.7) * baseColor.a;

    // Output in linear space - blit shader will tonemap and apply gamma
    fragColor = vec4(finalColor, alpha);
}
