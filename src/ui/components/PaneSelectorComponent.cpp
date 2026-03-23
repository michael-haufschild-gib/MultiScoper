/*
    Oscil - Pane Selector Component Implementation
    Reusable dropdown for selecting an existing pane or creating a new one
*/

#include "ui/components/PaneSelectorComponent.h"

namespace oscil
{

PaneSelectorComponent::PaneSelectorComponent(IThemeService& themeService, bool allowNewPane, const juce::String& testId)
    : themeService_(themeService)
    , allowNewPane_(allowNewPane)
{
    if (testId.isNotEmpty())
        setTestId(testId);

    setupDropdown();
}

PaneSelectorComponent::~PaneSelectorComponent() = default;

void PaneSelectorComponent::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

void PaneSelectorComponent::setupDropdown()
{
    juce::String dropdownTestId = testId_.isNotEmpty() ? testId_ + "_dropdown" : "";
    dropdown_ = std::make_unique<OscilDropdown>(themeService_, "Select pane...", dropdownTestId);
    dropdown_->onSelectionChanged = [this](int index) { handleSelectionChange(index); };
    addAndMakeVisible(*dropdown_);

    populateDropdown();
}

void PaneSelectorComponent::resized() { dropdown_->setBounds(getLocalBounds()); }

void PaneSelectorComponent::setAvailablePanes(const std::vector<std::pair<PaneId, juce::String>>& panes)
{
    panes_ = panes;
    populateDropdown();
}

void PaneSelectorComponent::setAvailablePanes(const std::vector<Pane>& panes)
{
    panes_.clear();
    panes_.reserve(panes.size());
    for (const auto& pane : panes)
    {
        panes_.emplace_back(pane.getId(), pane.getName());
    }
    populateDropdown();
}

void PaneSelectorComponent::populateDropdown()
{
    dropdown_->clearItems();

    // Add "New pane" option first if enabled
    if (allowNewPane_)
    {
        dropdown_->addItem("New pane", NEW_PANE_VALUE);
    }

    // Add existing panes
    for (const auto& pane : panes_)
    {
        dropdown_->addItem(pane.second, pane.first.id);
    }

    // Default selection
    if (allowNewPane_ && dropdown_->getNumItems() > 0)
    {
        dropdown_->setSelectedIndex(NEW_PANE_OPTION_INDEX, false);
    }
}

PaneId PaneSelectorComponent::getSelectedPaneId() const
{
    if (isNewPaneSelected())
        return PaneId::invalid();

    int index = dropdown_->getSelectedIndex();
    if (index < 0)
        return PaneId::invalid();

    // Adjust for "New pane" option
    int paneIndex = allowNewPane_ ? index - 1 : index;
    if (paneIndex >= 0 && static_cast<size_t>(paneIndex) < panes_.size())
    {
        return panes_[static_cast<size_t>(paneIndex)].first;
    }

    return PaneId::invalid();
}

bool PaneSelectorComponent::isNewPaneSelected() const
{
    if (!allowNewPane_)
        return false;

    return dropdown_->getSelectedIndex() == NEW_PANE_OPTION_INDEX;
}

bool PaneSelectorComponent::hasValidSelection() const
{
    int index = dropdown_->getSelectedIndex();
    if (index < 0)
        return false;

    // "New pane" is a valid selection if allowed
    if (allowNewPane_ && index == NEW_PANE_OPTION_INDEX)
        return true;

    // Check if an existing pane is selected
    int paneIndex = allowNewPane_ ? index - 1 : index;
    return paneIndex >= 0 && static_cast<size_t>(paneIndex) < panes_.size();
}

void PaneSelectorComponent::setSelectedPaneId(const PaneId& paneId, bool notify)
{
    if (!paneId.isValid())
    {
        if (allowNewPane_)
        {
            dropdown_->setSelectedIndex(NEW_PANE_OPTION_INDEX, notify);
        }
        else
        {
            dropdown_->setSelectedIndex(-1, notify);
        }
        return;
    }

    // Find the pane in our list
    for (size_t i = 0; i < panes_.size(); ++i)
    {
        if (panes_[i].first == paneId)
        {
            int dropdownIndex = allowNewPane_ ? static_cast<int>(i) + 1 : static_cast<int>(i);
            dropdown_->setSelectedIndex(dropdownIndex, notify);
            return;
        }
    }

    // Pane not found - select "New pane" or nothing
    if (allowNewPane_)
    {
        dropdown_->setSelectedIndex(NEW_PANE_OPTION_INDEX, notify);
    }
    else
    {
        dropdown_->setSelectedIndex(-1, notify);
    }
}

void PaneSelectorComponent::selectNewPane(bool notify)
{
    if (allowNewPane_)
    {
        dropdown_->setSelectedIndex(NEW_PANE_OPTION_INDEX, notify);
    }
}

void PaneSelectorComponent::handleSelectionChange(int index)
{
    if (!onSelectionChanged)
        return;

    if (allowNewPane_ && index == NEW_PANE_OPTION_INDEX)
    {
        onSelectionChanged(PaneId::invalid(), true);
    }
    else
    {
        int paneIndex = allowNewPane_ ? index - 1 : index;
        if (paneIndex >= 0 && static_cast<size_t>(paneIndex) < panes_.size())
        {
            onSelectionChanged(panes_[static_cast<size_t>(paneIndex)].first, false);
        }
        else
        {
            onSelectionChanged(PaneId::invalid(), false);
        }
    }
}

} // namespace oscil
