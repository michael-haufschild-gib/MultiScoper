/*
    Oscil - Bloom Effect GLSL Shader Sources
    Embedded GLSL 3.30 source strings for the dual-filter bloom chain.
    Split from BloomEffect.cpp to isolate shader text from compile/bind logic.
*/

#include "rendering/effects/BloomEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil::detail
{

// Prefilter: Extracts bright pixels and performs the first downsample (13-tap Dual Filter)
// Includes Karis Average to reduce fireflies (temporal instability)
const char* kBloomPrefilterFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform vec2 srcResolution;
    uniform float threshold;
    uniform float softKnee;

    in vec2 vTexCoord;
    out vec4 fragColor;

    // Partial Karis Average: weighting by 1/(1+luma)
    float karisWeight(vec3 color) {
        float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
        return 1.0 / (1.0 + luma);
    }

    vec3 applyThreshold(vec3 color) {
        float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
        float knee = threshold * softKnee;

        float contribution = max(0.0, brightness - threshold);

        if (softKnee > 0.001) {
            float soft = brightness - threshold + knee;
            soft = clamp(soft, 0.0, 2.0 * knee);
            contribution = soft * soft / (4.0 * knee + 0.00001);
            contribution += max(0.0, brightness - threshold - knee);
        }

        contribution /= max(brightness, 0.00001);
        return color * contribution;
    }

    void main()
    {
        vec2 texelSize = 1.0 / srcResolution;
        float x = texelSize.x;
        float y = texelSize.y;

        // 13-tap Jimenez Filter with Karis Average
        // A B C
        // D E F
        // G H I
        // J K L
        // M N O

        // Groups:
        // Center: [F, G, J, K] (Wait, standard dual filter uses 5 groups)
        // Let's use the standard Jimenez 13-tap locations:
        // (0,0), (-2,-2), (-2,2), (2,-2), (2,2) ...

        // Sample locations from "Next Generation Post Processing"
        vec3 a = texture(sourceTexture, vTexCoord + vec2(-2*x, 2*y)).rgb;
        vec3 b = texture(sourceTexture, vTexCoord + vec2( 0,   2*y)).rgb;
        vec3 c = texture(sourceTexture, vTexCoord + vec2( 2*x, 2*y)).rgb;

        vec3 d = texture(sourceTexture, vTexCoord + vec2(-1*x, 1*y)).rgb;
        vec3 e = texture(sourceTexture, vTexCoord + vec2( 1*x, 1*y)).rgb;

        vec3 f = texture(sourceTexture, vTexCoord + vec2(-2*x, 0)).rgb;
        vec3 g = texture(sourceTexture, vTexCoord).rgb;
        vec3 h = texture(sourceTexture, vTexCoord + vec2( 2*x, 0)).rgb;

        vec3 i = texture(sourceTexture, vTexCoord + vec2(-1*x, -1*y)).rgb;
        vec3 j = texture(sourceTexture, vTexCoord + vec2( 1*x, -1*y)).rgb;

        vec3 k = texture(sourceTexture, vTexCoord + vec2(-2*x, -2*y)).rgb;
        vec3 l = texture(sourceTexture, vTexCoord + vec2( 0,   -2*y)).rgb;
        vec3 m = texture(sourceTexture, vTexCoord + vec2( 2*x, -2*y)).rgb;

        // Weighted average groups with Karis weight
        // Group 0 (Center)
        vec3 g0 = (d + e + i + j) * 0.25;
        // Group 1 (Top Left)
        vec3 g1 = (a + b + g + f) * 0.25;
        // Group 2 (Top Right)
        vec3 g2 = (b + c + h + g) * 0.25;
        // Group 3 (Bottom Left)
        vec3 g3 = (f + g + l + k) * 0.25;
        // Group 4 (Bottom Right)
        vec3 g4 = (g + h + m + l) * 0.25;

        // Apply Karis weights to groups to suppress fireflies
        // Note: We apply thresholding *after* downsampling to preserve energy of thin lines,
        // but we might want to threshold *before* to prevent dimming?
        // Standard approach: Downsample -> Threshold.
        // Karis approach: Weighted Average -> Threshold.

        float w0 = karisWeight(g0);
        float w1 = karisWeight(g1);
        float w2 = karisWeight(g2);
        float w3 = karisWeight(g3);
        float w4 = karisWeight(g4);

        vec3 sum = g0 * w0 * 0.5;
        sum += g1 * w1 * 0.125;
        sum += g2 * w2 * 0.125;
        sum += g3 * w3 * 0.125;
        sum += g4 * w4 * 0.125;

        float totalWeight = w0 * 0.5 + (w1 + w2 + w3 + w4) * 0.125;

        vec3 avgColor = sum / max(totalWeight, 0.0001);

        // Finally apply threshold
        vec3 result = applyThreshold(avgColor);

        fragColor = vec4(result, 1.0);
    }
)";

// Downsample: Dual Filter (13-tap) - Preserves shape better than Gaussian
// Based on "Next Generation Post Processing in Call of Duty: Advanced Warfare" (Jimenez 2014)
const char* kBloomDownsampleFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform vec2 srcResolution;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec2 texelSize = 1.0 / srcResolution;
        float x = texelSize.x;
        float y = texelSize.y;

        // Take 13 samples
        // a - b - c
        // - d - e -
        // f - g - h
        // - i - j -
        // k - l - m

        vec3 a = texture(sourceTexture, vTexCoord + vec2(-2*x, 2*y)).rgb;
        vec3 b = texture(sourceTexture, vTexCoord + vec2( 0,   2*y)).rgb;
        vec3 c = texture(sourceTexture, vTexCoord + vec2( 2*x, 2*y)).rgb;

        vec3 d = texture(sourceTexture, vTexCoord + vec2(-1*x, 1*y)).rgb;
        vec3 e = texture(sourceTexture, vTexCoord + vec2( 1*x, 1*y)).rgb;

        vec3 f = texture(sourceTexture, vTexCoord + vec2(-2*x, 0)).rgb;
        vec3 g = texture(sourceTexture, vTexCoord).rgb;
        vec3 h = texture(sourceTexture, vTexCoord + vec2( 2*x, 0)).rgb;

        vec3 i = texture(sourceTexture, vTexCoord + vec2(-1*x, -1*y)).rgb;
        vec3 j = texture(sourceTexture, vTexCoord + vec2( 1*x, -1*y)).rgb;

        vec3 k = texture(sourceTexture, vTexCoord + vec2(-2*x, -2*y)).rgb;
        vec3 l = texture(sourceTexture, vTexCoord + vec2( 0,   -2*y)).rgb;
        vec3 m = texture(sourceTexture, vTexCoord + vec2( 2*x, -2*y)).rgb;

        // Weighted average
        vec3 result = vec3(0.0);
        result += (d + e + i + j) * 0.5;      // Inner box
        result += (a + b + g + f) * 0.125;    // Top-left box
        result += (b + c + h + g) * 0.125;    // Top-right box
        result += (f + g + l + k) * 0.125;    // Bottom-left box
        result += (g + h + m + l) * 0.125;    // Bottom-right box

        fragColor = vec4(result * 0.25, 1.0); // Normalize (sum of weights = 4, so divide by 4)
    }
)";

// Upsample: Tent Filter (3x3) - Smooth reconstruction
const char* kBloomUpsampleFragmentShader = R"(
    #version 330 core
    uniform sampler2D sourceTexture;
    uniform float filterRadius;
    uniform vec2 texelSize;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        // Scale offset by filterRadius (user setting) and actual texel size
        // This ensures we sample neighboring pixels correctly at any resolution
        vec2 offset = texelSize * filterRadius;

        vec3 result = vec3(0.0);

        // 9-tap tent filter
        // Center
        result += texture(sourceTexture, vTexCoord).rgb * 4.0;

        // Direct neighbors (Cross)
        result += texture(sourceTexture, vTexCoord + vec2(-offset.x, 0.0)).rgb * 2.0;
        result += texture(sourceTexture, vTexCoord + vec2( offset.x, 0.0)).rgb * 2.0;
        result += texture(sourceTexture, vTexCoord + vec2( 0.0, -offset.y)).rgb * 2.0;
        result += texture(sourceTexture, vTexCoord + vec2( 0.0,  offset.y)).rgb * 2.0;

        // Diagonals (Corners)
        result += texture(sourceTexture, vTexCoord + vec2(-offset.x, -offset.y)).rgb * 1.0;
        result += texture(sourceTexture, vTexCoord + vec2( offset.x, -offset.y)).rgb * 1.0;
        result += texture(sourceTexture, vTexCoord + vec2(-offset.x,  offset.y)).rgb * 1.0;
        result += texture(sourceTexture, vTexCoord + vec2( offset.x,  offset.y)).rgb * 1.0;

        fragColor = vec4(result * 0.0625, 1.0); // Divide by 16
    }
)";

// Final Combine
const char* kBloomCombineFragmentShader = R"(
    #version 330 core
    uniform sampler2D originalTexture;
    uniform sampler2D bloomTexture;
    uniform float intensity;

    in vec2 vTexCoord;
    out vec4 fragColor;

    void main()
    {
        vec4 original = texture(originalTexture, vTexCoord);
        vec3 bloom = texture(bloomTexture, vTexCoord).rgb;

        // Additive combine of Scene + Bloom in LINEAR space
        // Tone mapping and gamma correction will happen in the final blit
        vec3 hdrColor = original.rgb + bloom * intensity;

        // Calculate alpha for compositing over transparent background
        float bloomAlpha = length(bloom * intensity);
        float finalAlpha = clamp(original.a + bloomAlpha, 0.0, 1.0);

        fragColor = vec4(hdrColor, finalAlpha);
    }
)";

} // namespace oscil::detail

#endif // OSCIL_ENABLE_OPENGL
