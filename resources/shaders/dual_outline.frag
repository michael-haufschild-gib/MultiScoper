#version 330 core
in float vDistFromCenter;

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
    float dist = abs(vDistFromCenter);

    // Inner line: Center to 0.3
    float inner = 1.0 - smoothstep(0.2, 0.3, dist);

    // Outer line: 0.6 to 0.9
    float outer = smoothstep(0.6, 0.7, dist) * (1.0 - smoothstep(0.8, 0.9, dist));

    // Combine
    float alpha = (inner + outer * 0.5) * opacity * baseColor.a;

    // Convert sRGB input to linear for correct rendering
    vec3 linearColor = sRGBToLinear(baseColor.rgb);

    fragColor = vec4(linearColor, alpha);
}
