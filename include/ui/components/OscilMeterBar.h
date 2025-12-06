/*
    Oscil - Meter Bar Component
    Audio level meter with peak hold and RMS display
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/ThemedComponent.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/ComponentTypes.h"
#include "ui/components/TestId.h"

namespace oscil
{

/**
 * Audio level meter bar
 *
 * Features:
 * - Peak and RMS level display
 * - Peak hold with decay
 * - Configurable orientation
 * - Clip indicator
 * - Stereo or mono mode
 * - Theme integration
 */
class OscilMeterBar : public ThemedComponent,
                      public TestIdSupport,
                      private juce::Timer
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    enum class MeterType
    {
        Peak,
        RMS,
        PeakWithRMS
    };

    explicit OscilMeterBar(IThemeService& themeService);
    OscilMeterBar(IThemeService& themeService, const juce::String& testId);
    ~OscilMeterBar() override;

    // Level input (0.0 to 1.0, can exceed for clip detection)
    void setLevel(float level);
    void setLevels(float left, float right);  // Stereo
    void setRMSLevel(float rms);
    void setRMSLevels(float leftRMS, float rightRMS);

    float getLevel() const { return leftLevel_; }
    float getRMSLevel() const { return leftRMS_; }

    // Configuration
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return orientation_; }

    void setMeterType(MeterType type);
    MeterType getMeterType() const { return meterType_; }

    void setStereo(bool stereo);
    bool isStereo() const { return stereo_; }

    void setPeakHoldTime(int milliseconds);
    int getPeakHoldTime() const { return peakHoldTimeMs_; }

    void setPeakDecayRate(float decayPerSecond);
    float getPeakDecayRate() const { return peakDecayRate_; }

    void setMinDb(float db);
    float getMinDb() const { return minDb_; }

    void setMaxDb(float db);
    float getMaxDb() const { return maxDb_; }

    void setShowScale(bool show);
    bool getShowScale() const { return showScale_; }

    // State
    bool isClipping() const { return leftClip_ || rightClip_; }
    void resetClip();
    void resetPeakHold();

    // Size hints
    int getPreferredWidth() const;
    int getPreferredHeight() const;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;


private:
    void timerCallback() override;

    float levelToPosition(float level) const;
    float dbToLevel(float db) const;
    juce::Colour getLevelColour(float level) const;

    void paintMeter(juce::Graphics& g, juce::Rectangle<int> bounds,
                    float level, float rms, float peakHold, bool clip);
    void paintScale(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Current levels
    float leftLevel_ = 0.0f;
    float rightLevel_ = 0.0f;
    float leftRMS_ = 0.0f;
    float rightRMS_ = 0.0f;

    // Peak hold
    float leftPeakHold_ = 0.0f;
    float rightPeakHold_ = 0.0f;
    int leftPeakHoldCounter_ = 0;
    int rightPeakHoldCounter_ = 0;

    // Clip indicators
    bool leftClip_ = false;
    bool rightClip_ = false;

    // Smoothed values for display
    float leftSmooth_ = 0.0f;
    float rightSmooth_ = 0.0f;

    // Configuration
    Orientation orientation_ = Orientation::Vertical;
    MeterType meterType_ = MeterType::PeakWithRMS;
    bool stereo_ = false;
    int peakHoldTimeMs_ = 2000;
    float peakDecayRate_ = 0.5f;  // Per second
    float minDb_ = -60.0f;
    float maxDb_ = 6.0f;
    bool showScale_ = false;


    static constexpr int MONO_WIDTH = 12;
    static constexpr int STEREO_WIDTH = 24;
    static constexpr int SCALE_WIDTH = 24;
    static constexpr float SMOOTH_FACTOR = 0.3f;
    static constexpr int TIMER_HZ = 30;
    static constexpr float FRAME_TIME_SEC = 1.0f / static_cast<float>(TIMER_HZ);

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilMeterBar)
};

} // namespace oscil
