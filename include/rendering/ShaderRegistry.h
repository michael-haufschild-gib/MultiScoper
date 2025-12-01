/*
    Oscil - Shader Registry
    Singleton managing shader registration and retrieval
*/

#pragma once

#include "WaveformShader.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace oscil
{

/**
 * Singleton registry for waveform shaders.
 * Manages shader lifecycle and provides access for rendering.
 */
class ShaderRegistry
{
public:
    /**
     * Get the singleton instance
     */
    [[nodiscard]] static ShaderRegistry& getInstance();

    /**
     * Register a shader
     * @param shader The shader to register (takes ownership)
     */
    void registerShader(std::unique_ptr<WaveformShader> shader);

    /**
     * Get a shader by ID (prototype access for software rendering)
     * @param shaderId The shader identifier
     * @return Pointer to shader prototype, or nullptr if not found
     */
    [[nodiscard]] WaveformShader* getShader(const juce::String& shaderId);

    /**
     * Get a shader by ID (const version)
     */
    [[nodiscard]] const WaveformShader* getShader(const juce::String& shaderId) const;

    /**
     * Create a new instance of a shader by ID.
     * This is used by RenderEngine to create per-instance shaders that own their GL state.
     * @param shaderId The shader identifier
     * @return Unique pointer to new shader instance, or nullptr if not found
     */
    [[nodiscard]] std::unique_ptr<WaveformShader> createShader(const juce::String& shaderId) const;

    /**
     * Get list of all available shaders
     * @return Vector of shader info for UI display
     */
    [[nodiscard]] std::vector<ShaderInfo> getAvailableShaders() const;

    /**
     * Get the default shader ID
     */
    [[nodiscard]] juce::String getDefaultShaderId() const { return defaultShaderId_; }

    /**
     * Check if a shader ID is valid
     */
    [[nodiscard]] bool hasShader(const juce::String& shaderId) const;

    ShaderRegistry();
    ~ShaderRegistry();

    // Prevent copying
    ShaderRegistry(const ShaderRegistry&) = delete;
    ShaderRegistry& operator=(const ShaderRegistry&) = delete;

private:
    /**
     * Register built-in shaders
     */
    void registerBuiltInShaders();

    // Prototype storage
    std::unordered_map<std::string, std::unique_ptr<WaveformShader>> shaders_;
    // Factory storage
    std::unordered_map<std::string, std::function<std::unique_ptr<WaveformShader>()>> factories_;
    
    juce::String defaultShaderId_ = "basic";
    
    // Helper to register a shader type with both prototype and factory
    template<typename T>
    void registerShaderType()
    {
        auto prototype = std::make_unique<T>();
        auto id = prototype->getId().toStdString();
        shaders_[id] = std::move(prototype);
        factories_[id] = []() { return std::make_unique<T>(); };
    }
};

} // namespace oscil
