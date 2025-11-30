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
    registerShader(std::make_unique<BasicShader>());

    // Register 2D shaders
    registerShader(std::make_unique<NeonGlowShader>());
    registerShader(std::make_unique<GradientFillShader>());
    registerShader(std::make_unique<DualOutlineShader>());
    registerShader(std::make_unique<PlasmaSineShader>());
    registerShader(std::make_unique<DigitalGlitchShader>());

    // Register 3D shaders
    registerShader(std::make_unique<VolumetricRibbonShader>());
    registerShader(std::make_unique<WireframeMeshShader>());
    registerShader(std::make_unique<VectorFlowShader>());
    registerShader(std::make_unique<StringTheoryShader>());
    registerShader(std::make_unique<ElectricFlowerShader>());

    // Register Material shaders
    registerShader(std::make_unique<GlassRefractionShader>());
    registerShader(std::make_unique<LiquidChromeShader>());
    registerShader(std::make_unique<CrystallineShader>());
}

void ShaderRegistry::registerShader(std::unique_ptr<WaveformShader> shader)
{
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

#if OSCIL_ENABLE_OPENGL
void ShaderRegistry::compileAll(juce::OpenGLContext& context)
{
    for (auto& [id, shader] : shaders_)
    {
        if (shader && !shader->isCompiled())
        {
            if (!shader->compile(context))
            {
                DBG("Failed to compile shader: " << juce::String(id));
            }
        }
    }
}

void ShaderRegistry::releaseAll(juce::OpenGLContext& context)
{
    for (auto& [id, shader] : shaders_)
    {
        if (shader && shader->isCompiled())
        {
            shader->release(context);
        }
    }
}
#endif

} // namespace oscil
