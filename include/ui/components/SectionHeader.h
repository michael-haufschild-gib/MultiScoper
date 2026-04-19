/*
    MultiScoper - Section Header

    Reusable section-title row: small-caps label + optional left/right
    chevron navigator + 1px hairline divider beneath. The visual idiom is
    used inside panels where a subsection title introduces a cluster of
    controls and (optionally) cycles presets.

    Implementation lives in SectionHeader.cpp.
*/

#pragma once

#include "ui/components/ThemedComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace multiscoper
{

class SectionHeader : public ThemedComponent
{
public:
    explicit SectionHeader(IThemeService& themeService, juce::String title = {});
    ~SectionHeader() override = default;

    void setTitle(const juce::String& title);
    juce::String getTitle() const { return title_; }

    /** Enable left/right chevrons. When enabled, click callbacks fire. */
    void setChevronsVisible(bool visible);

    /** Optional accent colour used for chevrons / divider tint. When left
        transparent, theme.divider and theme.textSecondary are used. */
    void setAccentColour(juce::Colour accent);
    [[nodiscard]] juce::Colour getAccentColour() const { return accent_; }

    std::function<void()> onPrev;
    std::function<void()> onNext;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    static constexpr int PREFERRED_HEIGHT = 22;

private:
    juce::String title_;
    bool chevronsVisible_ = false;
    juce::Colour accent_ = juce::Colours::transparentBlack;

    juce::Rectangle<int> prevBounds_;
    juce::Rectangle<int> nextBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionHeader)
};

} // namespace multiscoper
