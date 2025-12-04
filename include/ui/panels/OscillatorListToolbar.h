/*
    Oscil - Oscillator List Toolbar
    Filter and count controls for the oscillator list
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "core/ServiceContext.h"
#include "ui/components/OscilButton.h"
#include "ui/components/OscilDropdown.h"
#include "ui/components/OscilTextField.h"
#include "ui/components/SegmentedButtonBar.h"
#include "core/Oscillator.h"
#include "ui/components/TestId.h"
#include <functional>

namespace oscil
{

/**
 * Filter options for oscillator list
 */
enum class OscillatorFilterMode
{
    All,
    Visible,
    Hidden
};

/**
 * Toolbar component for oscillator list
 * Provides filter and count display
 */
class OscillatorListToolbar : public juce::Component,
                               public ThemeManagerListener,
                               public TestIdSupport
{
public:
    /**
     * Listener interface for toolbar actions
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void filterModeChanged(OscillatorFilterMode /*mode*/) {}
    };

    explicit OscillatorListToolbar(ServiceContext& context);
    explicit OscillatorListToolbar(IThemeService& themeService);
    ~OscillatorListToolbar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters
    void setFilterMode(OscillatorFilterMode mode);
    void setOscillatorCount(int total, int visible);

    // State getters
    OscillatorFilterMode getFilterMode() const { return currentFilterMode_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    static constexpr int PREFERRED_HEIGHT = 36;  // Reduced since we removed a row

private:
    void setupComponents();
    void updateCountLabel();

    // Filter tabs
    std::unique_ptr<SegmentedButtonBar> filterTabs_;

    // State
    OscillatorFilterMode currentFilterMode_ = OscillatorFilterMode::All;
    int totalCount_ = 0;
    int visibleCount_ = 0;

    juce::ListenerList<Listener> listeners_;

    IThemeService& themeService_;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorListToolbar)
};

} // namespace oscil
