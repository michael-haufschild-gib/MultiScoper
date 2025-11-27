/*
    Oscil - Oscillator List Item
    Compact list item that expands when selected to show controls
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "core/Oscillator.h"
#include <functional>

namespace oscil
{

/**
 * Oscillator list item component
 * Shows compact view for non-selected items, expands when selected to show mode/visibility controls
 */
class OscillatorListItemComponent : public juce::Component,
                                     public ThemeManagerListener
{
public:
    /**
     * Listener interface for item actions
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void oscillatorSelected(const OscillatorId& /*id*/) {}
        virtual void oscillatorVisibilityChanged(const OscillatorId& /*id*/, bool /*visible*/) {}
        virtual void oscillatorModeChanged(const OscillatorId& /*id*/, ProcessingMode /*mode*/) {}
        virtual void oscillatorConfigRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorDeleteRequested(const OscillatorId& /*id*/) {}
        virtual void oscillatorDragStarted(const OscillatorId& /*id*/) {}
        virtual void oscillatorMoveRequested(const OscillatorId& /*id*/, int /*direction*/) {}
    };

    explicit OscillatorListItemComponent(const Oscillator& oscillator);
    ~OscillatorListItemComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State management
    void setSelected(bool selected);
    bool isSelected() const { return selected_; }

    void updateFromOscillator(const Oscillator& oscillator);

    OscillatorId getOscillatorId() const { return oscillatorId_; }
    bool isOscillatorVisible() const { return isVisible_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height depends on selected state
    int getPreferredHeight() const { return selected_ ? EXPANDED_HEIGHT : COMPACT_HEIGHT; }

    static constexpr int COMPACT_HEIGHT = 56;
    static constexpr int EXPANDED_HEIGHT = 100;
    static constexpr int PREFERRED_HEIGHT = COMPACT_HEIGHT;  // Default for layout calculations

private:
    void paintCompact(juce::Graphics& g, const ColorTheme& theme);
    void paintExpanded(juce::Graphics& g, const ColorTheme& theme);
    void paintModeButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                         const juce::String& text, bool isActive, const ColorTheme& theme);
    void paintVisibilityToggle(juce::Graphics& g, juce::Rectangle<float> bounds, const ColorTheme& theme);
    void paintIconButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                         const juce::String& icon, bool isHovered, const ColorTheme& theme, bool isDanger = false);

    bool isInDragZone(const juce::Point<int>& pos) const;
    bool isInSettingsButton(const juce::Point<int>& pos) const;
    bool isInDeleteButton(const juce::Point<int>& pos) const;
    bool isInVisibilityToggle(const juce::Point<int>& pos) const;
    bool isInModeButton(const juce::Point<int>& pos, ProcessingMode& outMode) const;

    // Data
    OscillatorId oscillatorId_;
    juce::String displayName_;
    juce::String trackName_;
    juce::Colour colour_;
    ProcessingMode processingMode_ = ProcessingMode::FullStereo;
    bool isVisible_ = true;

    // UI state
    bool selected_ = false;
    bool isHovered_ = false;
    bool isDragging_ = false;
    bool hasFocus_ = false;
    bool dragHandleHovered_ = false;
    bool settingsHovered_ = false;
    bool deleteHovered_ = false;
    bool visibilityHovered_ = false;
    int hoveredModeButton_ = -1;  // -1 = none, 0-3 = mode index
    juce::Point<int> dragStartPos_;

    juce::ListenerList<Listener> listeners_;

    // Layout constants
    static constexpr int DRAG_HANDLE_WIDTH = 24;
    static constexpr int COLOR_INDICATOR_SIZE = 14;
    static constexpr int ICON_BUTTON_SIZE = 24;
    static constexpr int ROW_PADDING = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorListItemComponent)
};

} // namespace oscil
