/*
    Oscil - Framebuffer Implementation
*/

#include "rendering/Framebuffer.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

bool Framebuffer::initFbo(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    ext.glGenFramebuffers(1, &fbo);
    if (fbo == 0)
    {
        DBG("Framebuffer: Failed to generate FBO");
        return false;
    }

    ext.glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    auto unbindAndFail = [&ext]() {
        ext.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    };

    if (numSamples > 0)
    {
        DBG("Framebuffer: MSAA requested (numSamples=" << numSamples << ") but not implemented. Use numSamples=0.");
        jassertfalse;
        return unbindAndFail();
    }

    if (!createColorTexture(context))
        return unbindAndFail();

    if (hasDepth && !createDepthBuffer(context))
        return unbindAndFail();

    if (!checkFramebufferComplete())
    {
        DBG("Framebuffer: Framebuffer is not complete");
        return unbindAndFail();
    }

    ext.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool Framebuffer::create(juce::OpenGLContext& context, int w, int h, int samples, GLenum fmt, bool withDepth,
                         bool useDepthTexture)
{
    if (w <= 0 || h <= 0)
        return false;

    width = w;
    height = h;
    numSamples = samples;
    format = fmt;
    hasDepth = withDepth;
    hasDepthTexture = useDepthTexture && withDepth;

    if (!initFbo(context))
    {
        destroy(context);
        return false;
    }

    return true;
}

void Framebuffer::resize(juce::OpenGLContext& context, int w, int h)
{
    if (w == width && h == height)
        return;

    GLenum savedFormat = format;
    bool savedHasDepth = hasDepth;
    bool savedHasDepthTexture = hasDepthTexture;
    int savedSamples = numSamples;

    destroy(context);
    if (!create(context, w, h, savedSamples, savedFormat, savedHasDepth, savedHasDepthTexture))
    {
        DBG("Framebuffer::resize() failed to recreate FBO at " << w << "x" << h);
        // Framebuffer is now invalid - caller should check isValid() before use
    }
}

void Framebuffer::bind()
{
    if (fbo != 0)
    {
        auto* context = juce::OpenGLContext::getCurrentContext();
        if (context != nullptr)
        {
            context->extensions.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, width, height);
        }
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
    // Guard against invalid texture unit (GL supports 0-15 typically)
    if (colorTexture != 0 && textureUnit >= 0 && textureUnit < 16)
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
        glBindTexture(GL_TEXTURE_2D, colorTexture);
    }
}

void Framebuffer::bindDepthTexture(int textureUnit)
{
    // Guard against invalid texture unit (GL supports 0-15 typically)
    if (depthTexture != 0 && textureUnit >= 0 && textureUnit < 16)
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
        glBindTexture(GL_TEXTURE_2D, depthTexture);
    }
}

void Framebuffer::clear(juce::Colour colour, bool clearDepth)
{
    glClearColor(colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());

    GLbitfield clearMask = GL_COLOR_BUFFER_BIT;
    if (clearDepth && hasDepth)
    {
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear(clearMask);
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

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), width, height, 0, GL_RGBA, dataType, nullptr);

    // Set texture parameters for FBO usage
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach to framebuffer
    ext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Framebuffer::createDepthBuffer(juce::OpenGLContext& context)
{
    auto& ext = context.extensions;

    if (hasDepthTexture)
    {
        // Create sampleable depth texture
        glGenTextures(1, &depthTexture);
        if (depthTexture == 0)
        {
            DBG("Framebuffer: Failed to generate depth texture");
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        ext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
    else
    {
        // Create renderbuffer (faster if sampling not needed)
        ext.glGenRenderbuffers(1, &depthBuffer);
        if (depthBuffer == 0)
        {
            DBG("Framebuffer: Failed to generate depth renderbuffer");
            return false;
        }

        ext.glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        ext.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        ext.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        ext.glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return true;
    }
}

bool Framebuffer::checkFramebufferComplete()
{
    auto* ctx = juce::OpenGLContext::getCurrentContext();
    if (ctx == nullptr)
        return false;

    GLenum status = ctx->extensions.glCheckFramebufferStatus(GL_FRAMEBUFFER);

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
            DBG("Framebuffer: Unknown error " << (int) status);
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

    if (depthTexture != 0)
    {
        glDeleteTextures(1, &depthTexture);
        depthTexture = 0;
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

    if (colorRenderbuffer != 0)
    {
        ext.glDeleteRenderbuffers(1, &colorRenderbuffer);
        colorRenderbuffer = 0;
    }

    width = 0;
    height = 0;
    numSamples = 0;
    format = juce::gl::GL_RGBA8;
    hasDepth = false;
    hasDepthTexture = false;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL