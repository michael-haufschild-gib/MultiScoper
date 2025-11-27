# Oscil OpenGL Shader Waveforms - Image Prompt

Optimized prompt for Google Nano Banana (Gemini Image Generation) to create a showcase of OpenGL shader-based waveform visualization styles for the Oscil audio plugin.

---

## Shader Waveform Variety Showcase Prompt

A showcase grid of twelve OpenGL shader-rendered audio waveforms demonstrates the visual diversity achievable through GPU-accelerated rendering, each waveform occupying its own dark panel against a unified deep charcoal (#1A1A2E) background. The collection presents distinct shader techniques applied to identical audio data, revealing how fragment shaders, bloom effects, and procedural rendering transform simple oscilloscope traces into stunning visual experiences.

**Panel 1 - Neon Bloom:**
An electric cyan (#00D9FF) waveform pulses with intense bloom shader effect, the core trace rendered as a crisp 2px line while multiple Gaussian blur passes create expanding halos of light at 80%, 40%, and 20% opacity. Peak amplitudes intensify the glow radius, creating breathing luminosity that responds to signal dynamics. The bloom extends 24 pixels from the core, fading into the darkness with smooth falloff.

**Panel 2 - Phosphor CRT:**
A vintage green (#39FF14) waveform mimics classic analog oscilloscope phosphor persistence, featuring subtle scan line overlay at 8% opacity and characteristic phosphor decay trails. Previous waveform positions ghost behind the current trace in diminishing brightness, creating motion blur temporal effect. Slight chromatic aberration separates RGB channels at the edges, and rounded screen corners with vignette darkening complete the retro aesthetic.

**Panel 3 - Gradient Fill:**
A waveform with vertical gradient fill transitions from hot magenta (#FF006E) at peaks through purple (#A855F7) at the centerline to deep violet (#4C1D95) in the troughs. The fill area beneath the waveform renders as smooth color interpolation, with the upper edge maintaining a bright 1px stroke. Subtle dithering eliminates banding in the gradient transitions.

**Panel 4 - Glass Refraction:**
A crystalline waveform appears to bend light passing through it, rendered with refractive shader simulating thick glass material. The trace in cool white (#F0F9FF) features subtle rainbow chromatic dispersion along curved sections, internal caustic highlights, and soft shadows cast onto the panel background. Fresnel effect brightens edges where the viewing angle becomes acute.

**Panel 5 - Volumetric Ribbon:**
A three-dimensional ribbon waveform twists through space with Z-depth variation tied to frequency content. The ribbon surface in coral (#FB7185) catches simulated lighting from above-left, creating darker shadows on the underside and specular highlights along the peaks. Ambient occlusion darkens the valleys where the ribbon folds close together. Subtle depth-of-field blur affects portions further from the focal plane.

**Panel 6 - Particle Stream:**
Thousands of tiny luminous particles in amber (#F59E0B) flow along the waveform path, their density increasing at high amplitude regions. Each particle renders as a 2px soft circle with additive blending, creating bright accumulation where particles cluster. Motion trails extend behind each particle suggesting velocity, and occasional spark particles escape the main stream at transient peaks.

**Panel 7 - Heat Signature:**
A thermal imaging style waveform maps amplitude to a heat gradient colormap: deep blue (#1E40AF) in quiet passages through cyan (#06B6D4), green (#22C55E), yellow (#FBBF24), to intense white-hot (#FFFBEB) at maximum levels. The surrounding area shows subtle heat diffusion bleeding outward from the trace, with cooler dark purple (#312E81) ambient temperature in silent regions.

**Panel 8 - Wireframe Mesh:**
A three-dimensional mesh surface represents the waveform as terrain, with amplitude as height displacement on a grid of vertices. Sky blue (#38BDF8) wireframe lines connect vertices, perspective projection creating depth as the mesh recedes. Hidden line removal ensures proper occlusion, and vertex lighting brightens peaks while valleys fall into shadow.

**Panel 9 - Liquid Chrome:**
A metallic mercury-like waveform reflects an imaginary environment map, the highly reflective surface in silver (#E5E7EB) capturing distorted reflections of colored studio lights. The trace appears to have physical thickness and weight, bulging at peaks with convincing specular highlights. Subtle motion blur smooths rapid transitions, maintaining the liquid illusion.

**Panel 10 - Aurora Borealis:**
A flowing, ethereal waveform mimics northern lights with translucent overlapping curtains of color. Layers of green (#4ADE80), teal (#2DD4BF), and purple (#C084FC) blend with soft additive transparency, undulating with gentle secondary animation. Subtle noise texture breaks up the colors organically, and faint star points glimmer in the dark background above.

**Panel 11 - Digital Glitch:**
An intentionally corrupted aesthetic renders the waveform in cyan (#00D9FF) with periodic horizontal displacement glitches, RGB channel separation creating red and blue ghost traces offset by 4-8 pixels. Scanline artifacts and brief hold frames suggest data corruption. Block artifacts occasionally fragment sections, and subtle static noise overlays the entire panel.

**Panel 12 - Dual Outline Glow:**
A sophisticated stroke treatment renders the waveform with concentric outlines: innermost core in pure white (#FFFFFF), surrounded by electric cyan (#00D9FF) at 4px width, then hot magenta (#FF006E) at 8px width, finally a soft outer glow in purple (#A855F7) extending 16px. Each layer uses decreasing opacity, creating depth through color separation.

Every panel maintains consistent 16:9 aspect ratio with subtle grid lines at 5% opacity in slate (#334155). Panel labels in small monospace text identify each shader style. The overall composition demonstrates professional audio visualization software capabilities, GPU-accelerated real-time rendering, OpenGL fragment shader artistry, 4K resolution quality, production-ready oscilloscope aesthetics.

---

## Prompt Analysis

- **Subject**: Twelve distinct OpenGL shader waveform rendering styles for audio visualization
- **Style**: Professional audio software UI, GPU shader art, real-time visualization aesthetics
- **Key Enhancements**: Each panel demonstrates a specific shader technique (bloom, refraction, particles, volumetric, thermal mapping), uses established plugin color palette, includes technical shader terminology for accuracy
- **Quality Modifiers**: "GPU-accelerated real-time rendering", "OpenGL fragment shader artistry", "4K resolution quality", "production-ready oscilloscope aesthetics"

---

## Shader Style Reference

| Style | Primary Color | Shader Technique | Visual Character |
|-------|---------------|------------------|------------------|
| Neon Bloom | #00D9FF | Multi-pass Gaussian blur, additive blending | Intense, modern, club aesthetic |
| Phosphor CRT | #39FF14 | Temporal persistence, scan lines, vignette | Vintage, analog, nostalgic |
| Gradient Fill | #FF006E | Vertex color interpolation, dithering | Smooth, artistic, colorful |
| Glass Refraction | #F0F9FF | Refraction mapping, Fresnel, caustics | Elegant, premium, dimensional |
| Volumetric Ribbon | #FB7185 | 3D geometry, Phong lighting, AO | Dynamic, dimensional, organic |
| Particle Stream | #F59E0B | Point sprites, additive blend, trails | Energetic, alive, detailed |
| Heat Signature | Gradient | Color LUT mapping, blur diffusion | Technical, data-driven, scientific |
| Wireframe Mesh | #38BDF8 | Vertex displacement, perspective, wireframe | Technical, retro-3D, analytical |
| Liquid Chrome | #E5E7EB | Environment mapping, specular highlights | Luxurious, fluid, metallic |
| Aurora Borealis | Multi | Alpha blending, noise, layering | Ethereal, dreamy, natural |
| Digital Glitch | #00D9FF | Channel separation, displacement, noise | Edgy, modern, chaotic |
| Dual Outline Glow | Multi | Multi-pass stroke, opacity falloff | Bold, stylized, impactful |

---

## Implementation Notes for Developers

### Shader Techniques by Complexity

**Beginner (Fragment shader only):**
- Neon Bloom (post-process blur)
- Gradient Fill (interpolated vertex colors)
- Heat Signature (color LUT)

**Intermediate (Multiple passes):**
- Phosphor CRT (temporal buffer + post-process)
- Dual Outline Glow (multi-pass rendering)
- Digital Glitch (channel manipulation)

**Advanced (Geometry + Lighting):**
- Volumetric Ribbon (3D mesh generation)
- Wireframe Mesh (vertex displacement)
- Particle Stream (instanced rendering)

**Expert (Complex materials):**
- Glass Refraction (environment mapping, Fresnel)
- Liquid Chrome (PBR materials, reflections)
- Aurora Borealis (layered transparency, procedural animation)

### Performance Considerations

| Style | GPU Load | Suitable For |
|-------|----------|--------------|
| Gradient Fill | Low | All systems |
| Heat Signature | Low | All systems |
| Neon Bloom | Medium | Mid-range+ |
| Phosphor CRT | Medium | Mid-range+ |
| Dual Outline Glow | Medium | Mid-range+ |
| Digital Glitch | Medium | Mid-range+ |
| Particle Stream | High | Dedicated GPU |
| Wireframe Mesh | High | Dedicated GPU |
| Volumetric Ribbon | High | Dedicated GPU |
| Aurora Borealis | High | Dedicated GPU |
| Liquid Chrome | Very High | Gaming GPU |
| Glass Refraction | Very High | Gaming GPU |
