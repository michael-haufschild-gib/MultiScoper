# Particle Texture Placeholders

This directory contains placeholder PNG images for the particle system textures referenced in the JUCE binary data configuration.

## Files

- `particle_cloud.png` - Cloud particle texture (64x64 RGBA)
- `particle_smoke_sheet.png` - Smoke particle sprite sheet (512x512 RGBA, 8x8 grid)
- `particle_fire_sheet.png` - Fire particle sprite sheet (512x512 RGBA, 8x8 grid)
- `particle_electric_sheet.png` - Electric particle sprite sheet (512x512 RGBA, 8x8 grid)
- `particle_sparkle.png` - Sparkle particle texture (64x64 RGBA)
- `particle_halo.png` - Halo particle texture (64x64 RGBA)
- `particle_shockwave.png` - Shockwave particle texture (64x64 RGBA)
- `particle_point_soft.png` - Soft point particle texture (64x64 RGBA)
- `particle_wisp.png` - Wisp particle texture (64x64 RGBA)

## Note

These are simple white placeholder textures created to satisfy the build requirements. They should be replaced with actual designed particle textures for production use.

The textures are referenced in `src/rendering/materials/TextureManager.cpp` and compiled into the binary via `CMakeLists.txt` using JUCE's `juce_add_binary_data`.
