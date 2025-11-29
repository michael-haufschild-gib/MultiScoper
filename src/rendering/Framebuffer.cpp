/*
    Oscil - Framebuffer Implementation
*/

#include "rendering/Framebuffer.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : fbo(other.fbo)
    , colorTexture(other.colorTexture)
    , depthBuffer(other.depthBuffer)
    , width(other.width)
    , height(other.height)
    , format(other.format)
    , hasDepth(other.hasDepth)
{
    // Reset the source object to prevent double-free
    other.fbo = 0;
    other.colorTexture = 0;
    other.depthBuffer = 0;
    other.width = 0;
    other.height = 0;
    other.hasDepth = false;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        // Note: Resources should be released via destroy() before assignment
        // We don't release here because we don't have the OpenGL context

        fbo = other.fbo;
        colorTexture = other.colorTexture;
        depthBuffer = other.depthBuffer;
        width = other.width;
        height = other.height;
        format = other.format;
        hasDepth = other.hasDepth;

        // Reset the source object to prevent double-free
        other.fbo = 0;
        other.colorTexture = 0;
        other.depthBuffer = 0;
        other.width = 0;
        other.height = 0;
        other.hasDepth = false;
    }
    return *this;
}

bool Framebuffer::create(juce::OpenGLContext& context, int w, int h, GLenum fmt, bool withDepth)
{
    if (w <= 0 || h <= 0)
        return false;

    width = w;
    height = h;
    format = fmt;
    hasDepth = withDepth;

    auto& ext = context.extensions;

    // Create framebuffer object
    ext.glGenFramebuffers(1, &fbo);
    if (fbo == 0)
    {
        DBG("Framebuffer: Failed to generate FBO");
        return false;
    }

    ext.glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create color texture
    if (!createColorTexture(context))
    {
        destroy(context);
        return false;
    }

    // Create depth buffer if requested
    if (withDepth && !createDepthBuffer(context))
    {
        destroy(context);
        return false;
    }

    // Check completeness
    if (!checkFramebufferComplete())
    {
        DBG("Framebuffer: Framebuffer is not complete");
        destroy(context);
        return false;
    }

    // Unbind
    ext.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    DBG("Framebuffer: Created " << width << "x" << height
        << " FBO=" << static_cast<int>(fbo) << " Texture=" << static_cast<int>(colorTexture)
        << " Depth=" << (hasDepth ? static_cast<int>(depthBuffer) : 0));

    return true;
}

bool Framebuffer::createColorTexture(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    glGenTextures(1, &colorTexture);
    if (colorTexture == 0)
    {
        DBG("Framebuffer: Failed to generate color texture");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, colorTexture);

    // Determine internal format and data type based on format parameter
    GLenum internalFormat = format;
    GLenum dataType = GL_UNSIGNED_BYTE;

    if (format == GL_RGBA16F)
    {
        internalFormat = GL_RGBA16F;
        dataType = GL_FLOAT;
    }
    else if (format == GL_RGBA32F)
    {
        internalFormat = GL_RGBA32F;
        dataType = GL_FLOAT;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                 width, height, 0, GL_RGBA, dataType, nullptr);

    // Set texture parameters for FBO usage
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach to framebuffer
    ext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, colorTexture, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Framebuffer::createDepthBuffer(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    ext.glGenRenderbuffers(1, &depthBuffer);
    if (depthBuffer == 0)
    {
        DBG("Framebuffer: Failed to generate depth renderbuffer");
        return false;
    }

    ext.glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    ext.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    ext.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depthBuffer);
    ext.glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return true;
}

bool Framebuffer::checkFramebufferComplete()
{
    auto* context = juce::OpenGLContext::getCurrentContext();
    if (context == nullptr)
    {
        DBG("Framebuffer: No OpenGL context available");
        return false;
    }

    GLenum status = context->extensions.glCheckFramebufferStatus(GL_FRAMEBUFFER);

    switch (status)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            DBG("Framebuffer: Incomplete attachment");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            DBG("Framebuffer: Missing attachment");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            DBG("Framebuffer: Unsupported format combination");
            break;
        default:
            DBG("Framebuffer: Unknown error " << static_cast<int>(status));
            break;
    }
    return false;
}

void Framebuffer::destroy(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    if (depthBuffer != 0)
    {
        ext.glDeleteRenderbuffers(1, &depthBuffer);
        depthBuffer = 0;
    }

    if (colorTexture != 0)
    {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }

    if (fbo != 0)
    {
        ext.glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }

    width = 0;
    height = 0;
    hasDepth = false;
}

void Framebuffer::resize(juce::OpenGLContext& context, int w, int h)
{
    if (w == width && h == height)
        return;

    GLenum savedFormat = format;
    bool savedHasDepth = hasDepth;

    destroy(context);
    create(context, w, h, savedFormat, savedHasDepth);
}

void Framebuffer::bind()
{
    if (fbo == 0)
        return;

    auto* context = juce::OpenGLContext::getCurrentContext();
    if (context != nullptr)
    {
        context->extensions.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }
}

void Framebuffer::unbind()
{
    auto* context = juce::OpenGLContext::getCurrentContext();
    if (context != nullptr)
    {
        context->extensions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Framebuffer::bindTexture(int textureUnit)
{
    if (colorTexture == 0)
        return;

    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
    glBindTexture(GL_TEXTURE_2D, colorTexture);
}

void Framebuffer::clear(juce::Colour colour, bool clearDepth)
{
    glClearColor(colour.getFloatRed(),
                 colour.getFloatGreen(),
                 colour.getFloatBlue(),
                 colour.getFloatAlpha());

    GLbitfield clearMask = GL_COLOR_BUFFER_BIT;
    if (clearDepth && hasDepth)
    {
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear(clearMask);
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
