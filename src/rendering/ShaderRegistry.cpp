/*
    Oscil - Shader Registry Implementation
*/

#include "rendering/ShaderRegistry.h"
#include "rendering/shaders/BasicShader.h"
#include "rendering/shaders/NeonGlowShader.h"
#include "rendering/shaders/GradientFillShader.h"
#include "rendering/shaders/DualOutlineShader.h"

namespace oscil
{

ShaderRegistry::ShaderRegistry()
{
    registerBuiltInShaders();
}

ShaderRegistry::~ShaderRegistry() = default;

void ShaderRegistry::registerBuiltInShaders()
{
    registerShaderType<BasicShader>();
    registerShaderType<NeonGlowShader>();
    registerShaderType<GradientFillShader>();
    registerShaderType<DualOutlineShader>();
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
