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
     * Get a shader by ID
     * @param shaderId The shader identifier
     * @return Pointer to shader, or nullptr if not found
     */
    [[nodiscard]] WaveformShader* getShader(const juce::String& shaderId);

    /**
     * Get a shader by ID (const version)
     */
    [[nodiscard]] const WaveformShader* getShader(const juce::String& shaderId) const;

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

#if OSCIL_ENABLE_OPENGL
    /**
     * Compile all registered shaders
     * Call this when OpenGL context becomes available
     */
    void compileAll(juce::OpenGLContext& context);

    /**
     * Release all shader resources
     * Call this before OpenGL context is destroyed
     */
    void releaseAll();
#endif

    // Prevent copying
    ShaderRegistry(const ShaderRegistry&) = delete;
    ShaderRegistry& operator=(const ShaderRegistry&) = delete;

private:
    ShaderRegistry();
    ~ShaderRegistry();

    /**
     * Register built-in shaders
     */
    void registerBuiltInShaders();

    std::unordered_map<std::string, std::unique_ptr<WaveformShader>> shaders_;
    juce::String defaultShaderId_ = "neon_glow";
};

} // namespace oscil
