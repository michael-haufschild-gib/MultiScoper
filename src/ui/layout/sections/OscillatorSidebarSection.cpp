/*
    Oscil - Oscillator Sidebar Section Implementation
    Collapsible section containing the Add Oscillator button and oscillator list
*/

#include "ui/layout/sections/OscillatorSidebarSection.h"

namespace oscil
{

OscillatorSidebarSection::OscillatorSidebarSection(ServiceContext& context) : themeService_(context.themeService)
{
    themeService_.addListener(this);
    setupComponents();

    // Create oscillator list
    oscillatorList_ = std::make_unique<OscillatorListComponent>(context);
    oscillatorList_->addListener(this);
    addAndMakeVisible(oscillatorList_.get());
}

OscillatorSidebarSection::~OscillatorSidebarSection()
{
    themeService_.removeListener(this);
    if (oscillatorList_)
        oscillatorList_->removeListener(this);
}

void OscillatorSidebarSection::setupComponents()
{
    // Add Oscillator button
    addOscillatorButton_ = std::make_unique<OscilButton>(themeService_, "+ Add Oscillator", "sidebar_addOscillator");
    addOscillatorButton_->setVariant(ButtonVariant::Primary);
    addOscillatorButton_->onClick = [this]() {
        listeners_.call([](Listener& l) { l.addOscillatorDialogRequested(); });
    };
    addAndMakeVisible(addOscillatorButton_.get());
}

void OscillatorSidebarSection::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
    // Background handled by accordion section
}

void OscillatorSidebarSection::resized()
{
    auto bounds = getLocalBounds();

    // Add Oscillator button at top
    auto buttonArea = bounds.removeFromTop(ADD_BUTTON_HEIGHT).reduced(0, SectionLayout::SPACING_SMALL);
    addOscillatorButton_->setBounds(buttonArea);

    // Small gap
    bounds.removeFromTop(SectionLayout::SPACING_MEDIUM);

    // Oscillator list takes remaining space (up to max height)
    if (oscillatorList_)
    {
        oscillatorList_->setBounds(bounds);
    }
}

void OscillatorSidebarSection::themeChanged(const ColorTheme&) { repaint(); }

int OscillatorSidebarSection::getPreferredHeight() const
{
    // Button height + spacing + list height
    return ADD_BUTTON_HEIGHT + SectionLayout::SPACING_MEDIUM + MAX_LIST_HEIGHT;
}

void OscillatorSidebarSection::refreshList(const std::vector<Oscillator>& oscillators)
{
    if (oscillatorList_)
        oscillatorList_->refreshList(oscillators);
}

void OscillatorSidebarSection::setSelectedOscillator(const OscillatorId& oscillatorId)
{
    if (oscillatorList_)
        oscillatorList_->setSelectedOscillator(oscillatorId);
}

void OscillatorSidebarSection::addListener(Listener* listener) { listeners_.add(listener); }

void OscillatorSidebarSection::removeListener(Listener* listener) { listeners_.remove(listener); }

// OscillatorListComponent::Listener forwarding

void OscillatorSidebarSection::oscillatorSelected(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorSelected(id); });
}

void OscillatorSidebarSection::oscillatorVisibilityChanged(const OscillatorId& id, bool visible)
{
    listeners_.call([id, visible](Listener& l) { l.oscillatorVisibilityChanged(id, visible); });
}

void OscillatorSidebarSection::oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode)
{
    listeners_.call([id, mode](Listener& l) { l.oscillatorModeChanged(id, mode); });
}

void OscillatorSidebarSection::oscillatorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorConfigRequested(id); });
}

void OscillatorSidebarSection::oscillatorColorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorColorConfigRequested(id); });
}

void OscillatorSidebarSection::oscillatorDeleteRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorDeleteRequested(id); });
}

void OscillatorSidebarSection::oscillatorsReordered(int fromIndex, int toIndex)
{
    listeners_.call([fromIndex, toIndex](Listener& l) { l.oscillatorsReordered(fromIndex, toIndex); });
}

void OscillatorSidebarSection::oscillatorPaneSelectionRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorPaneSelectionRequested(id); });
}

void OscillatorSidebarSection::oscillatorNameChanged(const OscillatorId& id, const juce::String& newName)
{
    listeners_.call([id, &newName](Listener& l) { l.oscillatorNameChanged(id, newName); });
}

} // namespace oscil
