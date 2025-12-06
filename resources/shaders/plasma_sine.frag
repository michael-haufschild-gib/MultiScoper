#version 330 core
in vec2 vPos;
in float vV;
in float vT;

uniform vec4 baseColor;
uniform float opacity;
uniform float time;
uniform int passIndex; // 0=Haze, 1=Arcs, 2=Core

out vec4 fragColor;

// Convert sRGB to linear color space
// Input colors from juce::Colour are in sRGB, but we need linear for correct
// blending and tonemapping in the rendering pipeline
vec3 sRGBToLinear(vec3 srgb) {
    return pow(srgb, vec3(2.2));
}

// High Quality Noise for Texture
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
             -0.577350269189626, 0.024390243902439);
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod289(i);
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
    + i.x + vec3(0.0, i1.x, 1.0 ));
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float fbm(vec2 p) {
    float f = 0.0;
    float w = 0.5;
    for (int i = 0; i < 3; i++) {
        f += w * snoise(p);
        p *= 2.0;
        w *= 0.5;
    }
    return f;
}

void main()
{
    float dist = abs(vV);

    // Soft edge for all layers to avoid hard polygon look
    float edgeFade = pow(max(1.0 - dist, 0.0), 2.0);

    // Waveform end fades
    float endFade = smoothstep(0.0, 0.05, vT) * (1.0 - smoothstep(0.95, 1.0, vT));

    // Convert sRGB input to linear for correct color operations
    vec3 linearBaseColor = sRGBToLinear(baseColor.rgb);

    vec4 finalColor = vec4(0.0);

    // --- PASS 0: HAZE (Volumetric Glow) ---
    if (passIndex == 0) {
        // Slow flowing smoke
        float n = fbm(vec2(vT * 5.0 - time * 0.2, vV * 2.0));
        n = n * 0.5 + 0.5; // 0..1

        vec3 col = linearBaseColor * 0.8; // Deep base color
        float a = 0.1 * n * edgeFade; // Low opacity
        finalColor = vec4(col * a, a);
    }

    // --- PASS 1: ARCS (Electric Jitter) ---
    else if (passIndex == 1) {
        // High contrast electric texture
        float n = snoise(vec2(vT * 20.0 - time * 1.0, vV * 5.0));
        float spark = 1.0 - abs(n);
        spark = pow(spark, 4.0); // Sharp lines

        vec3 col = mix(linearBaseColor, vec3(1.0), 0.5); // Brighter
        float a = 0.6 * spark * edgeFade;
        finalColor = vec4(col * a, a);
    }

    // --- PASS 2: CORE (Solid Beam) ---
    else if (passIndex == 2) {
        // Solid core with glow
        float core = smoothstep(0.6, 0.0, dist); // Sharp center
        vec3 col = vec3(1.0); // White (already linear)
        float a = core * 0.9;
        finalColor = vec4(col * a, a);
    }

    fragColor = finalColor * opacity * endFade;
}
