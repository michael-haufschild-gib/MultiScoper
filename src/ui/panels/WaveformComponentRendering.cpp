/*
    Oscil - Waveform Component Software Rendering
    Path building and shader-based drawing for CPU rendering path
*/

#include "ui/panels/WaveformComponent.h"

#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"

namespace oscil
{

void WaveformComponent::drawWaveformWithShader(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto* shader = shaderRegistry_.getShader(shaderId_);
    if (!shader)
        shader = shaderRegistry_.getShader(shaderRegistry_.getDefaultShaderId());

    if (!shader)
    {
        drawWaveform(g, bounds);
        return;
    }

    if (!presenter_)
        return;

    ShaderRenderParams params;
    params.colour = colour_;
    params.opacity = opacity_;
    params.lineWidth = lineWidth_;
    params.bounds = bounds.toFloat();
    params.isStereo = presenter_->isStereo();
    params.verticalScale = presenter_->getEffectiveScale();

    const auto& buffer2 = presenter_->getDisplayBuffer2();
    const std::vector<float>* channel2Ptr = params.isStereo ? &buffer2 : nullptr;
    shader->renderSoftware(g, presenter_->getDisplayBuffer1(), channel2Ptr, params);
}

void WaveformComponent::drawWaveform(juce::Graphics& g, juce::Rectangle<int> /*unused*/)
{
    if (!presenter_ || !presenter_->hasData())
        return;

    if (!waveformPath1_.isEmpty())
    {
        g.setColour(colour_.withAlpha(opacity_));
        g.strokePath(waveformPath1_, juce::PathStrokeType(lineWidth_));
    }

    if (presenter_->isStereo() && !waveformPath2_.isEmpty())
    {
        g.setColour(colour_.withAlpha(opacity_));
        g.strokePath(waveformPath2_, juce::PathStrokeType(lineWidth_));
    }
}

static void buildChannelPath(juce::Path& path, const std::vector<float>& buffer, int width, float centerY,
                             float amplitude, float scale, float yMin, float yMax)
{
    if (buffer.size() < 2)
        return;

    float const xScale = static_cast<float>(width) / static_cast<float>(buffer.size() - 1);
    float const yStart = juce::jlimit(yMin, yMax, centerY - (buffer[0] * amplitude * scale));
    path.startNewSubPath(0, yStart);

    for (size_t i = 1; i < buffer.size(); ++i)
    {
        float const x = static_cast<float>(i) * xScale;
        float const y = juce::jlimit(yMin, yMax, centerY - (buffer[i] * amplitude * scale));
        path.lineTo(x, y);
    }
}

void WaveformComponent::updateWaveformPath()
{
    processAudioData();

    if (holdDisplay_ || !presenter_)
        return;

    waveformPath1_.clear();
    waveformPath2_.clear();

    const auto& displayBuffer1 = presenter_->getDisplayBuffer1();
    const auto& displayBuffer2 = presenter_->getDisplayBuffer2();

    if (displayBuffer1.empty())
        return;

    int const width = getWidth();
    int const height = getHeight();
    if (width <= 0 || height <= 0)
        return;

    bool const isStereo = presenter_->isStereo();
    float const effectiveScale = presenter_->getEffectiveScale();
    auto const h = static_cast<float>(height);

    if (isStereo)
    {
        float const halfH = h * 0.5f;
        buildChannelPath(waveformPath1_, displayBuffer1, width, halfH * 0.5f, halfH * 0.5f, effectiveScale, 0.0f,
                         halfH);
        buildChannelPath(waveformPath2_, displayBuffer2, width, halfH * 1.5f, halfH * 0.5f, effectiveScale, halfH, h);
    }
    else
    {
        buildChannelPath(waveformPath1_, displayBuffer1, width, h * 0.5f, h * 0.5f, effectiveScale, 0.0f, h);
    }
}

} // namespace oscil
