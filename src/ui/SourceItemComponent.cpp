/*
    Oscil - Source Item Component Implementation
*/

#include "ui/SourceItemComponent.h"
#include "ui/ThemeManager.h"

namespace oscil
{

SourceItemComponent::SourceItemComponent(const SourceInfo& sourceInfo)
    : sourceId_(sourceInfo.sourceId)
    , displayName_(sourceInfo.name)
    , trackName_("")  // SourceInfo doesn't have separate track name
{
    // Register test ID with source ID
    juce::String testId = "sidebar_sources_item_" + sourceId_.id;
    OSCIL_REGISTER_TEST_ID(testId);

    // Create add-to-pane dropdown with test ID
    juce::String dropdownTestId = "sidebar_sources_item_" + sourceId_.id + "_paneDropdown";
    addToPaneDropdown_ = std::make_unique<OscilDropdown>("Add...", dropdownTestId);
    addToPaneDropdown_->onSelectionChanged = [this](int /*index*/) { handleAddToPaneSelection(); };
    addAndMakeVisible(addToPaneDropdown_.get());
}

void SourceItemComponent::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background
    if (isSelected_)
    {
        g.setColour(theme.controlActive.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(2), 4.0f);
    }
    else if (isHovered_)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.reduced(2), 4.0f);
    }

    // Activity indicator (green dot)
    auto leftArea = bounds.reduced(8, 0);
    auto dotBounds = leftArea.removeFromLeft(ACTIVITY_DOT_SIZE + 4);
    float dotY = dotBounds.getCentreY() - ACTIVITY_DOT_SIZE / 2.0f;

    if (isActive_)
    {
        g.setColour(juce::Colours::limegreen);
    }
    else
    {
        g.setColour(theme.textSecondary.withAlpha(0.3f));
    }
    g.fillEllipse(dotBounds.getX() + 2, dotY,
                  static_cast<float>(ACTIVITY_DOT_SIZE),
                  static_cast<float>(ACTIVITY_DOT_SIZE));

    // Source name
    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(13.0f));

    auto textArea = leftArea.reduced(4, 0);
    textArea.removeFromRight(DROPDOWN_WIDTH + 4);  // Leave space for dropdown

    // Display name (truncated if needed)
    juce::String text = displayName_;
    if (trackName_.isNotEmpty() && trackName_ != displayName_)
    {
        text += " (" + trackName_ + ")";
    }

    g.drawText(text, textArea, juce::Justification::centredLeft, true);
}

void SourceItemComponent::resized()
{
    auto bounds = getLocalBounds();

    // Position dropdown on right side
    auto dropdownBounds = bounds.removeFromRight(DROPDOWN_WIDTH).reduced(4, 6);
    addToPaneDropdown_->setBounds(dropdownBounds);
}

void SourceItemComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SourceItemComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

void SourceItemComponent::mouseUp(const juce::MouseEvent& e)
{
    // Don't trigger selection if clicking on dropdown
    if (!addToPaneDropdown_->getBounds().contains(e.getPosition()))
    {
        if (onSelected)
        {
            onSelected(sourceId_);
        }
    }
}

void SourceItemComponent::updateSourceInfo(const SourceInfo& info)
{
    displayName_ = info.name;
    repaint();
}

void SourceItemComponent::updateAvailablePanes(const std::vector<Pane>& panes)
{
    addToPaneDropdown_->clearItems();
    paneIds_.clear();

    // Add "New Pane" option (index 0)
    addToPaneDropdown_->addItem("New Pane");
    paneIds_.push_back(PaneId{});  // Empty ID means create new

    // Add separator (index 1)
    addToPaneDropdown_->addItem(DropdownItem::separator());
    paneIds_.push_back(PaneId{});  // Placeholder for separator

    // Add existing panes (indices 2+)
    int paneNum = 1;
    for (const auto& pane : panes)
    {
        juce::String paneName = pane.getName();
        if (paneName.isEmpty())
        {
            paneName = "Pane " + juce::String(paneNum);
        }
        addToPaneDropdown_->addItem(paneName);
        paneIds_.push_back(pane.getId());
        ++paneNum;
    }
}

void SourceItemComponent::setActive(bool isActive)
{
    if (isActive_ != isActive)
    {
        isActive_ = isActive;
        repaint();
    }
}

void SourceItemComponent::handleAddToPaneSelection()
{
    int selectedIndex = addToPaneDropdown_->getSelectedIndex();
    // Skip if nothing selected, separator selected, or out of range
    if (selectedIndex < 0 || static_cast<size_t>(selectedIndex) >= paneIds_.size())
    {
        return;
    }

    // Index 1 is the separator, skip it
    if (selectedIndex == 1)
    {
        return;
    }

    PaneId targetPaneId = paneIds_[static_cast<size_t>(selectedIndex)];

    // Use SafePointer because onAddToPane triggers refreshOscillatorPanels()
    // which may destroy this component during the callback
    juce::Component::SafePointer<SourceItemComponent> safeThis(this);

    if (onAddToPane)
    {
        onAddToPane(sourceId_, targetPaneId);
    }

    // Only reset dropdown if we're still valid (component wasn't destroyed)
    if (safeThis != nullptr)
    {
        addToPaneDropdown_->setSelectedIndex(-1, false);
    }
}

} // namespace oscil
