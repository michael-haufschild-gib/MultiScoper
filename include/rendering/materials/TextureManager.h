/*
    Oscil - Texture Manager Header
    Manages loading and binding of static textures from BinaryData
*/

#pragma once

#include <juce_opengl/juce_opengl.h>
#include <map>
#include <string>

namespace oscil
{

struct TextureMetadata
{
    int width = 0;
    int height = 0;
    int rows = 1;
    int cols = 1;
    bool isSpriteSheet = false;
};

class TextureManager
{
public:
    TextureManager();
    ~TextureManager();

    // Initialize and upload all textures to GPU
    void initialize(juce::OpenGLContext& context);

    // Release OpenGL resources
    void release(juce::OpenGLContext& context);

    // Bind a texture to a specific texture unit
    void bindTexture(const juce::String& name, int textureUnit);

    // Get the GL texture ID
    GLuint getTextureID(const juce::String& name) const;

    // Get metadata (dimensions, spritesheet info)
    TextureMetadata getMetadata(const juce::String& name) const;

private:
    void loadTexture(juce::OpenGLContext& context, const juce::String& name, const char* data, int dataSize, int rows = 1, int cols = 1);

    std::map<juce::String, GLuint> textures_;
    std::map<juce::String, TextureMetadata> metadata_;
    bool initialized_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureManager)
};

} // namespace oscil
