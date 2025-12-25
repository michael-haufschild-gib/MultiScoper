/*
    Oscil - Texture Manager Implementation
*/

#include "rendering/materials/TextureManager.h"
#include "BinaryData.h"

namespace oscil
{

using namespace juce::gl;

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
}

void TextureManager::initialize([[maybe_unused]] juce::OpenGLContext& context)
{
    if (initialized_)
        return;

    // Textures can be loaded on demand using loadTexture() method
    // No default textures are loaded since particle system was removed

    initialized_ = true;
}

void TextureManager::release(juce::OpenGLContext&)
{
    if (!initialized_)
        return;

    for (auto& pair : textures_)
    {
        GLuint textureID = pair.second;
        if (textureID != 0)
        {
            glDeleteTextures(1, &textureID);
        }
    }

    textures_.clear();
    metadata_.clear();
    initialized_ = false;
}

void TextureManager::bindTexture(const juce::String& name, int textureUnit)
{
    auto it = textures_.find(name);
    if (it != textures_.end())
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
        glBindTexture(GL_TEXTURE_2D, it->second);
    }
}

GLuint TextureManager::getTextureID(const juce::String& name) const
{
    auto it = textures_.find(name);
    if (it != textures_.end())
        return it->second;
    return 0;
}

TextureMetadata TextureManager::getMetadata(const juce::String& name) const
{
    auto it = metadata_.find(name);
    if (it != metadata_.end())
        return it->second;
    return TextureMetadata();
}

void TextureManager::loadTexture(juce::OpenGLContext&, const juce::String& name, const char* data, int dataSize, int rows, int cols)
{
    juce::MemoryInputStream stream(data, static_cast<size_t>(dataSize), false);
    juce::Image image = juce::ImageFileFormat::loadFrom(stream);

    if (!image.isValid())
    {
        DBG("TextureManager: Failed to load image " << name);
        return;
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Flip image vertically for OpenGL if necessary (JUCE images are top-left, OpenGL is bottom-left)
    // However, usually it's easier to flip UVs in shader. Let's stick to standard upload.

    juce::Image::BitmapData bitmapData(image, juce::Image::BitmapData::readOnly);
    
    // JUCE images are typically ARGB or RGB, but OpenGL expects RGBA.
    // We might need to swizzle or ensure the format is correct.
    // JUCE's BitmapData is usually ARGB (little endian -> BGRA in memory) or RGBA.
    // Let's assume standard RGBA for now, but might need GL_BGRA if colors are swapped.
    
    // Note: JUCE Image uses premultiplied alpha by default. 
    
    GLenum format = GL_RGBA;
    if (bitmapData.pixelFormat == juce::Image::ARGB)
    {
#ifdef GL_BGRA
        format = GL_BGRA;
#elif defined(GL_BGRA_EXT)
        format = GL_BGRA_EXT;
#endif
    }
    else if (bitmapData.pixelFormat == juce::Image::RGB)
    {
        format = GL_RGB;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getWidth(), image.getHeight(), 0, 
                 format, GL_UNSIGNED_BYTE, bitmapData.data);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    textures_[name] = textureID;
    
    TextureMetadata meta;
    meta.width = image.getWidth();
    meta.height = image.getHeight();
    meta.rows = rows;
    meta.cols = cols;
    meta.isSpriteSheet = (rows > 1 || cols > 1);
    metadata_[name] = meta;
}

} // namespace oscil
