/*
    Oscil - Shader Registry Implementation
*/

#include "rendering/ShaderRegistry.h"
#include "rendering/shaders/BasicShader.h"
#include "rendering/shaders/NeonGlowShader.h"
#include "rendering/shaders/GradientFillShader.h"
#include "rendering/shaders/DualOutlineShader.h"
#include "rendering/shaders/PlasmaSineShader.h"
#include "rendering/shaders/DigitalGlitchShader.h"
#include "rendering/shaders3d/VolumetricRibbonShader.h"
#include "rendering/shaders3d/WireframeMeshShader.h"
#include "rendering/shaders3d/VectorFlowShader.h"
#include "rendering/shaders3d/StringTheoryShader.h"
#include "rendering/shaders3d/ElectricFlowerShader.h"
#include "rendering/shaders3d/ElectricFiligreeShader.h"
#include "rendering/materials/GlassRefractionShader.h"
#include "rendering/materials/LiquidChromeShader.h"
#include "rendering/materials/CrystallineShader.h"

namespace oscil
{

ShaderRegistry& ShaderRegistry::getInstance()
{
    static ShaderRegistry instance;
    return instance;
}

ShaderRegistry::ShaderRegistry()
{
    registerBuiltInShaders();
}

ShaderRegistry::~ShaderRegistry() = default;

void ShaderRegistry::registerBuiltInShaders()
{
    // Register default shaders
    registerShaderType<BasicShader>();

    // Register 2D shaders
    registerShaderType<NeonGlowShader>();
    registerShaderType<GradientFillShader>();
    registerShaderType<DualOutlineShader>();
    registerShaderType<PlasmaSineShader>();
    registerShaderType<DigitalGlitchShader>();

    // Register 3D shaders
    registerShaderType<VolumetricRibbonShader>();
    registerShaderType<WireframeMeshShader>();
    registerShaderType<VectorFlowShader>();
    registerShaderType<StringTheoryShader>();
    registerShaderType<ElectricFlowerShader>();
    registerShaderType<ElectricFiligreeShader>();

    // Register Material shaders
    registerShaderType<GlassRefractionShader>();
    registerShaderType<LiquidChromeShader>();
    registerShaderType<CrystallineShader>();
}

void ShaderRegistry::registerShader(std::unique_ptr<WaveformShader> shader)
{
    // Deprecated: prefer registerShaderType for factory support
    // This overload only stores a prototype and cannot support per-instance GL compilation
    // Kept for compatibility if needed, but createShader will fail for these
    if (shader)
    {
        auto id = shader->getId().toStdString();
        shaders_[id] = std::move(shader);
    }
}

WaveformShader* ShaderRegistry::getShader(const juce::String& shaderId)
{
    auto it = shaders_.find(shaderId.toStdString());
    return (it != shaders_.end()) ? it->second.get() : nullptr;
}

const WaveformShader* ShaderRegistry::getShader(const juce::String& shaderId) const
{
    auto it = shaders_.find(shaderId.toStdString());
    return (it != shaders_.end()) ? it->second.get() : nullptr;
}

std::unique_ptr<WaveformShader> ShaderRegistry::createShader(const juce::String& shaderId) const
{
    auto it = factories_.find(shaderId.toStdString());
    if (it != factories_.end())
    {
        return it->second();
    }
    return nullptr;
}

std::vector<ShaderInfo> ShaderRegistry::getAvailableShaders() const
{
    std::vector<ShaderInfo> result;
    result.reserve(shaders_.size());

    for (const auto& [id, shader] : shaders_)
    {
        result.push_back(shader->getInfo());
    }

    return result;
}

bool ShaderRegistry::hasShader(const juce::String& shaderId) const
{
    return shaders_.find(shaderId.toStdString()) != shaders_.end();
}

} // namespace oscil
