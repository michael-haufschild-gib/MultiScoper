/*
    Oscil - Transport Sync Component Implementation
*/

#include "ui/components/OscilTransportSync.h"

namespace oscil
{

OscilTransportSync::OscilTransportSync()
    : beatPulseSpring_(SpringPresets::bouncy())
{
    theme_ = ThemeManager::getInstance().getCurrentTheme();
    ThemeManager::getInstance().addListener(this);

    beatPulseSpring_.position = 0.0f;

    startTimerHz(30);
}

OscilTransportSync::~OscilTransportSync()
{
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
}

void OscilTransportSync::updateTransportInfo(const juce::AudioPlayHead::PositionInfo& info)
{
    if (auto bpm = info.getBpm())
        hostBpm_ = *bpm;

    if (auto timeSig = info.getTimeSignature())
    {
        timeSignatureNumerator_ = timeSig->numerator;
        timeSignatureDenominator_ = timeSig->denominator;
    }

    if (auto ppq = info.getPpqPosition())
    {
        double newBeatPos = std::fmod(*ppq, static_cast<double>(timeSignatureNumerator_));

        // Detect beat transition for pulse animation
        if (static_cast<int>(newBeatPos) != static_cast<int>(beatPosition_))
        {
            beatPulseSpring_.position = 1.0f;
            beatPulseSpring_.setTarget(0.0f);
        }

        beatPosition_ = newBeatPos;
    }

    hostPlaying_ = info.getIsPlaying();
}

void OscilTransportSync::setManualBPM(double bpm)
{
    manualBpm_ = std::clamp(bpm, minBpm_, maxBpm_);

    if (!syncEnabled_ && onManualBPMChanged)
        onManualBPMChanged(manualBpm_);

    repaint();
}

void OscilTransportSync::setBPMRange(double min, double max)
{
    minBpm_ = min;
    maxBpm_ = max;
    manualBpm_ = std::clamp(manualBpm_, minBpm_, maxBpm_);
}

void OscilTransportSync::setSyncEnabled(bool enabled)
{
    if (syncEnabled_ != enabled)
    {
        syncEnabled_ = enabled;

        if (onSyncChanged)
            onSyncChanged(syncEnabled_);

        repaint();
    }
}

void OscilTransportSync::setShowTimeSignature(bool show)
{
    showTimeSignature_ = show;
    repaint();
}

void OscilTransportSync::setShowBeatIndicator(bool show)
{
    showBeatIndicator_ = show;
    repaint();
}

void OscilTransportSync::setCompactMode(bool compact)
{
    compactMode_ = compact;
    repaint();
}

double OscilTransportSync::getCurrentBPM() const
{
    return syncEnabled_ ? hostBpm_ : manualBpm_;
}

int OscilTransportSync::getPreferredWidth() const
{
    if (compactMode_)
        return 80;

    int width = 100;  // BPM display

    if (showTimeSignature_)
        width += 40;

    if (showBeatIndicator_)
        width += 60;

    return width;
}

int OscilTransportSync::getPreferredHeight() const
{
    return compactMode_ ? 24 : 36;
}

void OscilTransportSync::paint(juce::Graphics& g)
{
    paintBPMDisplay(g, getBPMBounds());

    if (showTimeSignature_ && !compactMode_)
        paintTimeSignature(g, getTimeSignatureBounds());

    if (showBeatIndicator_ && !compactMode_)
        paintBeatIndicator(g, getBeatIndicatorBounds());

    paintSyncButton(g, getSyncButtonBounds());
}

void OscilTransportSync::paintBPMDisplay(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    double bpm = getCurrentBPM();

    // Background
    g.setColour(theme_.backgroundSecondary);
    g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);

    // BPM value
    g.setColour(theme_.textPrimary);
    g.setFont(juce::Font(juce::FontOptions().withHeight(compactMode_ ? 14.0f : 18.0f)).boldened());

    juce::String bpmText = juce::String(bpm, 1);
    g.drawText(bpmText, bounds.reduced(4, 0), juce::Justification::centred);

    // "BPM" label
    if (!compactMode_)
    {
        g.setColour(theme_.textSecondary);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("BPM", bounds.getX(), bounds.getBottom() - 12, bounds.getWidth(), 10,
                   juce::Justification::centred);
    }

    // Border when synced
    if (syncEnabled_ && hostPlaying_)
    {
        float pulse = beatPulseSpring_.position;
        g.setColour(theme_.controlActive.withAlpha(0.3f + 0.4f * pulse));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), ComponentLayout::RADIUS_SM, 1.5f);
    }
}

void OscilTransportSync::paintTimeSignature(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(theme_.textSecondary);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));

    juce::String text = juce::String(timeSignatureNumerator_) + "/" +
                        juce::String(timeSignatureDenominator_);

    g.drawText(text, bounds, juce::Justification::centred);
}

void OscilTransportSync::paintBeatIndicator(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int numBeats = timeSignatureNumerator_;
    int currentBeat = static_cast<int>(beatPosition_);

    float dotSize = 8.0f;
    float spacing = (bounds.getWidth() - numBeats * dotSize) / (numBeats + 1);

    for (int i = 0; i < numBeats; ++i)
    {
        float x = bounds.getX() + spacing + i * (dotSize + spacing);
        float y = bounds.getCentreY() - dotSize / 2;

        bool isCurrentBeat = (i == currentBeat) && hostPlaying_;

        if (isCurrentBeat)
        {
            float scale = 1.0f + 0.3f * beatPulseSpring_.position;
            float scaledSize = dotSize * scale;
            float offset = (scaledSize - dotSize) / 2;

            g.setColour(theme_.controlActive);
            g.fillEllipse(x - offset, y - offset, scaledSize, scaledSize);
        }
        else
        {
            g.setColour(theme_.controlBorder);
            g.fillEllipse(x, y, dotSize, dotSize);
        }
    }
}

void OscilTransportSync::paintSyncButton(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Sync indicator
    auto colour = syncEnabled_ ? theme_.controlActive : theme_.textSecondary;

    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)).boldened());
    g.drawText("SYNC", bounds, juce::Justification::centred);

    if (syncEnabled_)
    {
        g.setColour(colour.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_XS);
    }
}

juce::Rectangle<int> OscilTransportSync::getBPMBounds() const
{
    if (compactMode_)
        return getLocalBounds().reduced(2);

    return juce::Rectangle<int>(4, 2, 60, getHeight() - 4);
}

juce::Rectangle<int> OscilTransportSync::getTimeSignatureBounds() const
{
    return juce::Rectangle<int>(68, 2, 36, getHeight() - 4);
}

juce::Rectangle<int> OscilTransportSync::getBeatIndicatorBounds() const
{
    int x = showTimeSignature_ ? 108 : 68;
    return juce::Rectangle<int>(x, 2, 56, getHeight() - 4);
}

juce::Rectangle<int> OscilTransportSync::getSyncButtonBounds() const
{
    return juce::Rectangle<int>(getWidth() - 36, static_cast<int>((getHeight() - 16) / 2), 32, 16);
}

void OscilTransportSync::resized()
{
    // Nothing to do
}

void OscilTransportSync::mouseDown(const juce::MouseEvent& e)
{
    // Check if clicking sync button
    if (getSyncButtonBounds().contains(e.getPosition()))
    {
        setSyncEnabled(!syncEnabled_);
        return;
    }

    // Check if clicking BPM (for manual adjustment)
    if (getBPMBounds().contains(e.getPosition()) && !syncEnabled_)
    {
        isDraggingBPM_ = true;
        dragStartBPM_ = manualBpm_;
        dragStartY_ = e.y;
    }
}

void OscilTransportSync::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingBPM_)
    {
        int deltaY = dragStartY_ - e.y;
        double newBPM = dragStartBPM_ + deltaY * 0.5;
        setManualBPM(newBPM);
    }
}

void OscilTransportSync::mouseUp(const juce::MouseEvent&)
{
    isDraggingBPM_ = false;
}

void OscilTransportSync::mouseWheelMove(const juce::MouseEvent& e,
                                         const juce::MouseWheelDetails& wheel)
{
    if (getBPMBounds().contains(e.getPosition()) && !syncEnabled_)
    {
        double delta = wheel.deltaY * 5.0;
        setManualBPM(manualBpm_ + delta);
    }
}

void OscilTransportSync::timerCallback()
{
    beatPulseSpring_.update(AnimationTiming::FRAME_DURATION_60FPS);
    repaint();
}

void OscilTransportSync::themeChanged(const ColorTheme& newTheme)
{
    theme_ = newTheme;
    repaint();
}

} // namespace oscil
