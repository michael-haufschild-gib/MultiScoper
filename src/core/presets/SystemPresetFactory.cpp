#include "core/presets/SystemPresetFactory.h"

namespace oscil
{

std::vector<SystemPresetFactory::PresetDefinition> SystemPresetFactory::getSystemPresets()
{
    return {
        {
            "default",
            "Default",
            "Clean waveform visualization with no effects",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Basic2D;
            }
        },
        {
            "vector_scope",
            "Vector Scope",
            "Classic oscilloscope look with scanlines and phosphor glow",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Basic2D;
                c.scanlines.enabled = true;
                c.scanlines.intensity = 0.4f;
                c.scanlines.density = 2.0f;
                c.scanlines.phosphorGlow = true;
                c.bloom.enabled = true;
                c.bloom.intensity = 1.2f;
                c.bloom.threshold = 0.1f;
                c.bloom.spread = 1.5f;
                c.filmGrain.enabled = true;
                c.filmGrain.intensity = 0.15f;
                c.colorGrade.enabled = true;
                c.colorGrade.tint = 0.1f;
                c.trails.enabled = true;
                c.trails.decay = 0.2f;
                c.trails.opacity = 0.6f;
            }
        },
        {
            "string_theory",
            "String Theory",
            "3D visualization with rotating strings",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::StringTheory;
                c.settings3D.enabled = true;
                c.settings3D.cameraDistance = 4.5f;
                c.settings3D.cameraAngleX = 15.0f;
                c.settings3D.autoRotate = true;
                c.settings3D.rotateSpeed = 5.0f;
                c.bloom.enabled = true;
                c.bloom.intensity = 1.5f;
            }
        },
        {
            "crystalline",
            "Crystalline",
            "Crystal material shader with high refraction",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Crystalline;
                c.settings3D.enabled = true;
                c.settings3D.cameraDistance = 4.0f;
                c.settings3D.autoRotate = true;
                c.settings3D.rotateSpeed = 4.0f;
                c.material.enabled = true;
                c.material.refractiveIndex = 2.4f;
                c.material.reflectivity = 0.8f;
                c.material.roughness = 0.0f;
                c.material.useEnvironmentMap = true;
                c.material.environmentMapId = "sunset";
                c.bloom.enabled = true;
                c.bloom.intensity = 1.8f;
            }
        },
        {
            "neon_glow",
            "Neon Glow",
            "Intense neon glow effect with additive blending",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::NeonGlow;
                c.compositeBlendMode = BlendMode::Additive;
                c.bloom.enabled = true;
                c.bloom.intensity = 2.0f;
                c.bloom.threshold = 0.2f;
                c.bloom.spread = 1.5f;
            }
        },
        {
            "synthwave",
            "Synthwave",
            "Retro synthwave aesthetic with wireframe grid",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::WireframeMesh;
                c.settings3D.enabled = true;
                c.settings3D.cameraDistance = 5.0f;
                c.settings3D.cameraAngleX = 25.0f;
                c.scanlines.enabled = true;
                c.scanlines.intensity = 0.3f;
                c.chromaticAberration.enabled = true;
                c.chromaticAberration.intensity = 0.008f;
                c.bloom.enabled = true;
                c.bloom.intensity = 1.2f;
                c.colorGrade.enabled = true;
                c.colorGrade.saturation = 1.3f;
            }
        },
        {
            "liquid_metal",
            "Liquid Metal",
            "Flowing liquid chrome with reflections",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::LiquidChrome;
                c.settings3D.enabled = true;
                c.settings3D.cameraDistance = 4.5f;
                c.settings3D.autoRotate = true;
                c.settings3D.rotateSpeed = 3.0f;
                c.material.enabled = true;
                c.material.reflectivity = 0.9f;
                c.material.roughness = 0.05f;
                c.material.useEnvironmentMap = true;
                c.bloom.enabled = true;
                c.bloom.intensity = 1.0f;
            }
        },
        {
            "electric",
            "Electric",
            "Lightning noise effect with trails and bloom",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::ElectricFiligree;
                c.settings3D.enabled = true;
                c.settings3D.cameraDistance = 4.0f;
                c.bloom.enabled = true;
                c.bloom.intensity = 1.6f;
                c.bloom.threshold = 0.3f;
                c.trails.enabled = true;
                c.trails.decay = 0.15f;
                c.trails.opacity = 0.7f;
            }
        },
        {
            "plasma",
            "Plasma",
            "Animated procedural plasma fill with color grading",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::PlasmaSine;
                c.colorGrade.enabled = true;
                c.colorGrade.saturation = 1.5f;
                c.colorGrade.contrast = 1.2f;
                c.bloom.enabled = true;
                c.bloom.intensity = 0.8f;
            }
        },
        {
            "minimal",
            "Minimal",
            "Clean and simple visualization with reduced opacity",
            [](VisualConfiguration& c) {
                c.shaderType = ShaderType::Basic2D;
                c.compositeOpacity = 0.7f;
            }
        }
    };
}

}
