/*
    Oscil - Shader Registry Implementation
*/

#include "rendering/ShaderRegistry.h"
#include "rendering/shaders/NeonGlowShader.h"
#include "rendering/shaders/VectorFlowShader.h"

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
    registerShader(std::make_unique<NeonGlowShader>());
    registerShader(std::make_unique<VectorFlowShader>());
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
