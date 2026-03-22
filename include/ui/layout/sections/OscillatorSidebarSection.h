/*
    Oscil - Oscillator Sidebar Section
    Collapsible section containing the Add Oscillator button and oscillator list
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "core/ServiceContext.h"
#include "ui/layout/sections/SectionConstants.h"
#include "ui/layout/sections/DynamicHeightContent.h"
#include "ui/components/OscilButton.h"
#include "ui/panels/OscillatorListComponent.h"
#include "ui/components/TestId.h"
#include "core/Oscillator.h"
#include <functional>

namespace oscil
{

/**
 * Oscillator section component for the sidebar
 * Contains the Add Oscillator button and the oscillator list with filter tabs
 * Designed to be used inside an OscilAccordion for collapsibility
 */
class OscillatorSidebarSection : public juce::Component,
                                  public ThemeManagerListener,
                                  public DynamicHeightContent,
                                  public OscillatorListComponent::Listener
{
public:
    /**
     * Listener interface for oscillator section events
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void addOscillatorDialogRequested() {}
        virtual void oscillatorSelected(const OscillatorId& /*id*/) {}
        virtual void oscillatorVisibilityChanged(const OscillatorId& /*id*/, bool /*visible*/) {}
        virtual void oscillatorModeChanged(const OscillatorId& /*id*/, ProcessingMode /*mode*/) {}
        virtual void oscillatorConfigRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorColorConfigRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorsReordered(int /*fromIndex*/, int /*toIndex*/) {}
        virtual void oscillatorPaneSelectionRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorNameChanged(const OscillatorId& /*id*/, const juce::String& /*newName*/) {}
    };

    /// Construct the oscillator management section with the given service context.
    explicit OscillatorSidebarSection(ServiceContext& context);
    ~OscillatorSidebarSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // OscillatorListComponent::Listener overrides
    void oscillatorSelected(const OscillatorId& id) override;
    void oscillatorVisibilityChanged(const OscillatorId& id, bool visible) override;
    void oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode) override;
    void oscillatorConfigRequested(const OscillatorId& id) override;
    void oscillatorColorConfigRequested(const OscillatorId& id) override;
    void oscillatorDeleteRequested(const OscillatorId& id) override;
    void oscillatorsReordered(int fromIndex, int toIndex) override;
    void oscillatorPaneSelectionRequested(const OscillatorId& id) override;
    void oscillatorNameChanged(const OscillatorId& id, const juce::String& newName) override;

    /// Refresh the oscillator list component with the current oscillator snapshot.
    void refreshList(const std::vector<Oscillator>& oscillators);
    void setSelectedOscillator(const OscillatorId& oscillatorId);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout (DynamicHeightContent)
    int getPreferredHeight() const override;

    // Layout constants
    static constexpr int ADD_BUTTON_HEIGHT = 36;
    static constexpr int MAX_LIST_HEIGHT = 300;

private:
    void setupComponents();

    IThemeService& themeService_;

    std::unique_ptr<OscilButton> addOscillatorButton_;
    std::unique_ptr<OscillatorListComponent> oscillatorList_;

    juce::ListenerList<Listener> listeners_;

    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorSidebarSection)
};

} // namespace oscil
