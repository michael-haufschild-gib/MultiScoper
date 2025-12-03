/*
    Oscil - Oscillator List Component Implementation
*/

#include "ui/panels/OscillatorListComponent.h"
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

    // Draw drag image if dragging
    if (isDraggingItem_ && dragImage_.isValid())
    {
        g.setOpacity(0.6f);
        g.drawImageAt(dragImage_, dragMousePosition_.x, dragMousePosition_.y);
        
        // Draw drag indicator line
        if (dragTargetIndex_ >= 0)
        {
            // Calculate Y position for indicator relative to this component
            // We need to account for viewport scroll position
            int yPos = 0;
            int viewY = viewport_->getViewPositionY();
            
            // Sum heights up to target index
            for (int i = 0; i < dragTargetIndex_; ++i)
            {
                if (static_cast<size_t>(i) < items_.size())
                    yPos += items_[static_cast<size_t>(i)]->getHeight();
            }
            
            // Adjust for scroll and relative positioning
            // container_ is inside viewport_, so yPos is relative to container
            // viewport_ is child of OscillatorListComponent
            int relativeY = yPos - viewY + viewport_->getY();
            
            g.setColour(theme.controlActive);
            g.drawHorizontalLine(relativeY, 0.0f, static_cast<float>(getWidth()));
            g.fillEllipse(0.0f, static_cast<float>(relativeY - 3), 6.0f, 6.0f);
            g.fillEllipse(static_cast<float>(getWidth() - 6), static_cast<float>(relativeY - 3), 6.0f, 6.0f);
        }
    }
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
    selectedOscillatorId_ = oscillatorId;
    for (auto& item : items_)
    {
        if (item)
            item->setSelected(item->getOscillatorId() == selectedOscillatorId_);
    }
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

void OscillatorListComponent::oscillatorColorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorColorConfigRequested(id); });
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
    draggedOscillatorId_ = id;
    dragSourceIndex_ = -1;

    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i]->getOscillatorId() == id)
        {
            dragSourceIndex_ = static_cast<int>(i);
            dragImage_ = items_[i]->createComponentSnapshot(items_[i]->getLocalBounds());
            break;
        }
    }

    if (dragSourceIndex_ >= 0)
    {
        isDraggingItem_ = true;
        dragTargetIndex_ = dragSourceIndex_;
        
        auto mousePos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
        dragMousePosition_ = getLocalPoint(nullptr, mousePos).toInt() - juce::Point<int>(10, 10); // Offset slightly

        juce::Desktop::getInstance().addGlobalMouseListener(this);
        repaint();
    }
}

void OscillatorListComponent::oscillatorMoveRequested(const OscillatorId& id, int direction)
{
    // Find current index
    int currentIndex = -1;
    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i]->getOscillatorId() == id)
        {
            currentIndex = static_cast<int>(i);
            break;
        }
    }

    if (currentIndex == -1) return;

    int newIndex = currentIndex + direction;
    if (newIndex >= 0 && newIndex < static_cast<int>(items_.size()))
    {
        listeners_.call([currentIndex, newIndex](Listener& l) { l.oscillatorsReordered(currentIndex, newIndex); });
    }
}

void OscillatorListComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingItem_)
    {
        auto mousePos = e.getEventRelativeTo(this).getPosition();
        dragMousePosition_ = mousePos;

        // Calculate target index based on mouse Y
        // Convert mouse Y to container coordinates to check against items
        int containerY = mousePos.y - viewport_->getY() + viewport_->getViewPositionY();
        
        int newTarget = getItemIndexAtY(containerY);
        if (newTarget != -1 && newTarget != dragTargetIndex_)
        {
            updateDragIndicator(newTarget);
        }
        
        repaint();
    }
}

void OscillatorListComponent::mouseUp(const juce::MouseEvent&)
{
    if (isDraggingItem_)
    {
        isDraggingItem_ = false;
        juce::Desktop::getInstance().removeGlobalMouseListener(this);

        if (dragSourceIndex_ != -1 && dragTargetIndex_ != -1 && dragSourceIndex_ != dragTargetIndex_)
        {
            // Adjust target index if dragging downwards (since removing source shifts indices)
            // But notify listener with raw indices, let the logic handle it
            listeners_.call([this](Listener& l) { l.oscillatorsReordered(dragSourceIndex_, dragTargetIndex_); });
        }

        dragSourceIndex_ = -1;
        dragTargetIndex_ = -1;
        dragImage_ = juce::Image();
        repaint();
    }
}

int OscillatorListComponent::getItemIndexAtY(int y) const
{
    if (items_.empty()) return 0;

    int currentY = 0;
    for (size_t i = 0; i < items_.size(); ++i)
    {
        int height = items_[i]->getHeight();
        if (y < currentY + height / 2)
        {
            return static_cast<int>(i);
        }
        currentY += height;
    }
    
    return static_cast<int>(items_.size());
}

void OscillatorListComponent::updateDragIndicator(int targetIndex)
{
    dragTargetIndex_ = targetIndex;
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
