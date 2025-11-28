/*
    Oscil - Source Selector Component Header
    Rich dropdown for selecting signal sources with search, icons, badges
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/InstanceRegistry.h"
#include "ThemeManager.h"
#include "ui/components/OscilTextField.h"
#include <functional>

namespace oscil
{

/**
 * Individual source item in the selector popup
 */
class SourceListItem : public juce::Component
{
public:
    SourceListItem(const SourceInfo& source);

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setSelected(bool selected);
    void setFilterMatch(bool matches);
    bool matchesFilter(const juce::String& filter) const;

    const SourceId& getSourceId() const { return sourceId_; }
    std::function<void(const SourceId&)> onClick;

    static constexpr int ITEM_HEIGHT = 36;

private:
    SourceId sourceId_;
    juce::String name_;
    int channelCount_;
    bool isActive_;
    bool selected_ = false;
    bool hovered_ = false;
    bool matchesFilter_ = true;

    void drawSourceIcon(juce::Graphics& g, juce::Rectangle<float> bounds);
    juce::String getIconForSource() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceListItem)
};

/**
 * "No Source (Disconnect)" option item
 */
class NoSourceItem : public juce::Component
{
public:
    NoSourceItem();

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void()> onClick;

    static constexpr int ITEM_HEIGHT = 32;

private:
    bool hovered_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoSourceItem)
};

/**
 * Popup content for source selection with search and rich list
 */
class SourceSelectorPopup : public juce::Component,
                             public InstanceRegistryListener,
                             public ThemeManagerListener
{
public:
    SourceSelectorPopup();
    ~SourceSelectorPopup() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelectedSourceId(const SourceId& sourceId);
    void refreshSources();

    std::function<void(const SourceId&)> onSourceSelected;
    std::function<void()> onDisconnect;

    // InstanceRegistryListener
    void sourceAdded(const SourceId& sourceId) override;
    void sourceRemoved(const SourceId& sourceId) override;
    void sourceUpdated(const SourceId& sourceId) override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    int getPreferredHeight() const;

private:
    void handleFilterChange();
    void applyFilter();

    std::unique_ptr<OscilTextField> searchInput_;
    std::unique_ptr<juce::Label> sectionHeader_;
    std::unique_ptr<juce::Viewport> listViewport_;
    std::unique_ptr<juce::Component> listContainer_;
    std::vector<std::unique_ptr<SourceListItem>> sourceItems_;
    std::unique_ptr<NoSourceItem> noSourceItem_;

    SourceId selectedSourceId_;
    juce::String currentFilter_;

    static constexpr int SEARCH_HEIGHT = 32;
    static constexpr int HEADER_HEIGHT = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceSelectorPopup)
};

/**
 * Dropdown button component for selecting a signal source.
 * Shows current selection and opens rich popup on click.
 */
class SourceSelectorComponent : public juce::Component,
                                 public InstanceRegistryListener
{
public:
    SourceSelectorComponent();
    ~SourceSelectorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    /**
     * Get the currently selected source ID
     */
    SourceId getSelectedSourceId() const { return selectedSourceId_; }

    /**
     * Set the selected source by ID
     */
    void setSelectedSourceId(const SourceId& sourceId);

    /**
     * Set callback for when selection changes
     */
    void onSelectionChanged(std::function<void(const SourceId&)> callback)
    {
        selectionChangedCallback_ = std::move(callback);
    }

    /**
     * Refresh the source list from registry
     */
    void refreshSources();

    // InstanceRegistryListener interface
    void sourceAdded(const SourceId& sourceId) override;
    void sourceRemoved(const SourceId& sourceId) override;
    void sourceUpdated(const SourceId& sourceId) override;

    // Preferred height for layout
    static constexpr int PREFERRED_HEIGHT = 28;

private:
    void showPopup();
    void updateDisplayText();

    SourceId selectedSourceId_;
    juce::String displayText_;
    int channelCount_ = 0;
    std::function<void(const SourceId&)> selectionChangedCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceSelectorComponent)
};

} // namespace oscil
