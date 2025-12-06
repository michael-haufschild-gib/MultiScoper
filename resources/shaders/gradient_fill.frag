#version 330 core
in float vV;

uniform vec4 baseColor;
uniform float opacity;

out vec4 fragColor;

// Convert sRGB to linear color space
// Input colors from juce::Colour are in sRGB, but we need linear for correct
// blending and tonemapping in the rendering pipeline
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
    fragColor = mix(bottomColor, topColor, vV);
}
