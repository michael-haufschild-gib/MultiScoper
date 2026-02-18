/*
    Oscil - Shader Registry Implementation
*/

#include "rendering/ShaderRegistry.h"
#include "rendering/shaders/BasicShader.h"
#include "rendering/shaders/NeonGlowShader.h"
#include "rendering/shaders/GradientFillShader.h"

namespace oscil
{

ShaderRegistry::ShaderRegistry()
{
    registerBuiltInShaders();
}

ShaderRegistry::~ShaderRegistry() = default;

void ShaderRegistry::registerBuiltInShaders()
{
    // Register 2D shaders
    registerShaderType<BasicShader>();
    registerShaderType<NeonGlowShader>();
    registerShaderType<GradientFillShader>();
}

WaveformShader* ShaderRegistry::getShader(const juce::String& shaderId)
{
    // H13 FIX: Use shared lock for concurrent read access
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = shaders_.find(shaderId.toStdString());
    return (it != shaders_.end()) ? it->second.get() : nullptr;
}

const WaveformShader* ShaderRegistry::getShader(const juce::String& shaderId) const
{
    // H13 FIX: Use shared lock for concurrent read access
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = shaders_.find(shaderId.toStdString());
    return (it != shaders_.end()) ? it->second.get() : nullptr;
}

std::unique_ptr<WaveformShader> ShaderRegistry::createShader(const juce::String& shaderId) const
{
    // H13 FIX: Use shared lock for concurrent read access
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = factories_.find(shaderId.toStdString());
    if (it != factories_.end())
    {
        return it->second();
    }
    return nullptr;
}

std::vector<ShaderInfo> ShaderRegistry::getAvailableShaders() const
{
    // H13 FIX: Use shared lock for concurrent read access
    std::shared_lock<std::shared_mutex> lock(mutex_);
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
    // H13 FIX: Use shared lock for concurrent read access
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return shaders_.find(shaderId.toStdString()) != shaders_.end();
}

} // namespace oscil
