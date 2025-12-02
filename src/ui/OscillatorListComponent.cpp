/*
    Oscil - Oscillator List Component Implementation
*/

#include "ui/OscillatorListComponent.h"
#include "ui/components/ComponentConstants.h"

namespace oscil
{

OscillatorListComponent::OscillatorListComponent(IInstanceRegistry& instanceRegistry)
    : instanceRegistry_(instanceRegistry)
{
    setOpaque(true);
    ThemeManager::getInstance().addListener(this);

    setTestId("oscillatorList");

    // Toolbar
    toolbar_ = std::make_unique<OscillatorListToolbar>();
    toolbar_->addListener(this);
    addAndMakeVisible(toolbar_.get());

    // Viewport and Container
    viewport_ = std::make_unique<juce::Viewport>();
    container_ = std::make_unique<juce::Component>();
    viewport_->setViewedComponent(container_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    viewport_->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    addAndMakeVisible(viewport_.get());

    // Empty state label
    emptyStateLabel_ = std::make_unique<juce::Label>();
    emptyStateLabel_->setText("No oscillators yet.\nClick '+ Add Oscillator' above to get started.", juce::dontSendNotification);
    emptyStateLabel_->setJustificationType(juce::Justification::centred);
    emptyStateLabel_->setColour(juce::Label::textColourId, ThemeManager::getInstance().getCurrentTheme().textSecondary);
    addChildComponent(emptyStateLabel_.get());
}

OscillatorListComponent::~OscillatorListComponent()
{
    removeAllChildren(); // Prevent double-free/UAF in base Component destructor

    if (isDraggingItem_)
    {
        juce::Desktop::getInstance().removeGlobalMouseListener(this);
    }

    ThemeManager::getInstance().removeListener(this);
    toolbar_->removeListener(this);

    for (auto& item : items_)
    {
        if (item)
            item->removeListener(this);
    }
}

void OscillatorListComponent::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

void OscillatorListComponent::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.fillAll(theme.backgroundSecondary);
}

void OscillatorListComponent::resized()
{
    auto bounds = getLocalBounds();

    toolbar_->setBounds(bounds.removeFromTop(OSCILLATOR_TOOLBAR_HEIGHT));

    if (items_.empty())
    {
        emptyStateLabel_->setBounds(bounds.reduced(ComponentLayout::SPACING_MD));
    }
    
    viewport_->setBounds(bounds);

    if (!items_.empty())
    {
        int contentHeight = 0;
        for (const auto& item : items_)
        {
            contentHeight += item->getPreferredHeight();
        }

        container_->setSize(viewport_->getWidth() - ComponentLayout::SPACING_MD, contentHeight);

        int y = 0;
        for (auto& item : items_)
        {
            int itemHeight = item->getPreferredHeight();
            item->setBounds(0, y, container_->getWidth(), itemHeight);
            y += itemHeight;
        }
    }
}

// ...

void OscillatorListComponent::updateOscillatorCounts()
{
    if (toolbar_)
    {
        int totalCount = static_cast<int>(allOscillators_.size());
        int visibleCount = 0;
        for (const auto& osc : allOscillators_)
        {
            if (osc.isVisible())
                visibleCount++;
        }
        toolbar_->setOscillatorCount(totalCount, visibleCount);
    }
}

void OscillatorListComponent::refreshList(const std::vector<Oscillator>& oscillators)
{
    // Remove listeners
    for (auto& item : items_)
    {
        if (item)
            item->removeListener(this);
    }

    items_.clear();
    container_->removeAllChildren();

    allOscillators_ = oscillators;

    // Filter
    std::vector<Oscillator> filtered;
    for (const auto& osc : oscillators)
    {
        bool include = false;
        switch (currentFilterMode_)
        {
            case OscillatorFilterMode::All:     include = true; break;
            case OscillatorFilterMode::Visible: include = osc.isVisible(); break;
            case OscillatorFilterMode::Hidden:  include = !osc.isVisible(); break;
        }
        if (include)
            filtered.push_back(osc);
    }

    // Create items
    for (const auto& osc : filtered)
    {
        auto item = std::make_unique<OscillatorListItemComponent>(osc, instanceRegistry_);
        item->setSelected(osc.getId() == selectedOscillatorId_);
        item->addListener(this);
        container_->addAndMakeVisible(*item);
        items_.push_back(std::move(item));
    }

    bool hasOscillators = !filtered.empty();
    emptyStateLabel_->setVisible(!hasOscillators);
    viewport_->setVisible(hasOscillators);

    updateOscillatorCounts();
    resized();
}

void OscillatorListComponent::setSelectedOscillator(const OscillatorId& oscillatorId)
{
}

void OscillatorListComponent::oscillatorSelected(const OscillatorId& id)
{
    setSelectedOscillator(id);
    listeners_.call([id](Listener& l) { l.oscillatorSelected(id); });
}

void OscillatorListComponent::oscillatorVisibilityChanged(const OscillatorId& id, bool visible)
{
    listeners_.call([id, visible](Listener& l) { l.oscillatorVisibilityChanged(id, visible); });
}

void OscillatorListComponent::oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode)
{
    listeners_.call([id, mode](Listener& l) { l.oscillatorModeChanged(id, mode); });
}

void OscillatorListComponent::oscillatorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorConfigRequested(id); });
}

void OscillatorListComponent::oscillatorDeleteRequested(const OscillatorId& id)
{
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<OscillatorListComponent>(this), id]()
    {
        if (safeThis != nullptr)
            safeThis->listeners_.call([id](Listener& l) { l.oscillatorDeleteRequested(id); });
    });
}

void OscillatorListComponent::oscillatorDragStarted(const OscillatorId& id)
{
}

void OscillatorListComponent::oscillatorMoveRequested(const OscillatorId& id, int direction)
{
}

void OscillatorListComponent::mouseDrag(const juce::MouseEvent& e)
{
}

void OscillatorListComponent::mouseUp(const juce::MouseEvent&)
{
}

int OscillatorListComponent::getItemIndexAtY(int y) const
{
    return -1;
}

void OscillatorListComponent::updateDragIndicator(int targetIndex)
{
}

void OscillatorListComponent::themeChanged(const ColorTheme&)
{
    repaint();
}

void OscillatorListComponent::filterModeChanged(OscillatorFilterMode mode)
{
    currentFilterMode_ = mode;
    refreshList(allOscillators_);
}

void OscillatorListComponent::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void OscillatorListComponent::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

} // namespace oscil
