/*
    Oscil - Transport Sync Component
    BPM display and DAW transport sync controls
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ui/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/AnimationSettings.h"

namespace oscil
{

/**
 * Transport sync display and controls
 *
 * Features:
 * - BPM display from host
 * - Time signature display
 * - Sync enable toggle
 * - Beat position indicator
 * - Animated beat pulse
 */
class OscilTransportSync : public juce::Component,
                           public ThemeManagerListener,
                           private juce::Timer
{
public:
    OscilTransportSync();
    ~OscilTransportSync() override;

    // Transport info update (call from processBlock)
    void updateTransportInfo(const juce::AudioPlayHead::PositionInfo& info);

    // Manual BPM (when not synced to host)
    void setManualBPM(double bpm);
    double getManualBPM() const { return manualBpm_; }

    void setBPMRange(double min, double max);
    double getMinBPM() const { return minBpm_; }
    double getMaxBPM() const { return maxBpm_; }

    // Sync control
    void setSyncEnabled(bool enabled);
    bool isSyncEnabled() const { return syncEnabled_; }

    // Display options
    void setShowTimeSignature(bool show);
    bool getShowTimeSignature() const { return showTimeSignature_; }

    void setShowBeatIndicator(bool show);
    bool getShowBeatIndicator() const { return showBeatIndicator_; }

    void setCompactMode(bool compact);
    bool isCompactMode() const { return compactMode_; }

    // Current values
    double getCurrentBPM() const;
    int getNumerator() const { return timeSignatureNumerator_; }
    int getDenominator() const { return timeSignatureDenominator_; }
    bool isHostPlaying() const { return hostPlaying_; }

    // Callbacks
    std::function<void(bool)> onSyncChanged;
    std::function<void(double)> onManualBPMChanged;

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    void timerCallback() override;

    void paintBPMDisplay(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintTimeSignature(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintBeatIndicator(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintSyncButton(juce::Graphics& g, juce::Rectangle<int> bounds);

    juce::Rectangle<int> getBPMBounds() const;
    juce::Rectangle<int> getTimeSignatureBounds() const;
    juce::Rectangle<int> getBeatIndicatorBounds() const;
    juce::Rectangle<int> getSyncButtonBounds() const;

    // Host transport info
    double hostBpm_ = 120.0;
    int timeSignatureNumerator_ = 4;
    int timeSignatureDenominator_ = 4;
    double beatPosition_ = 0.0;
    bool hostPlaying_ = false;

    // Manual/fallback
    double manualBpm_ = 120.0;
    double minBpm_ = 20.0;
    double maxBpm_ = 300.0;
    bool syncEnabled_ = true;

    // Display options
    bool showTimeSignature_ = true;
    bool showBeatIndicator_ = true;
    bool compactMode_ = false;

    // Interaction
    bool isDraggingBPM_ = false;
    double dragStartBPM_ = 0.0;
    int dragStartY_ = 0;

    // Animation
    SpringAnimation beatPulseSpring_;

    ColorTheme theme_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilTransportSync)
};

} // namespace oscil
