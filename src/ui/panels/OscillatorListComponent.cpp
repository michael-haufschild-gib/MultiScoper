/*
    Oscil - Oscillator List Component Implementation
*/

#include "ui/panels/OscillatorListComponent.h"

#include "ui/components/ComponentConstants.h"

namespace oscil
{

OscillatorListComponent::OscillatorListComponent(ServiceContext& context)
    : instanceRegistry_(context.instanceRegistry)
    , themeService_(context.themeService)
{
    setOpaque(true);
    themeService_.addListener(this);

    setTestId("oscillatorList");

    // Toolbar - reference is always valid, so no null check needed
    toolbar_ = std::make_unique<OscillatorListToolbar>(context);

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
    emptyStateLabel_->setText("No oscillators yet.\nClick '+ Add Oscillator' above to get started.",
                              juce::dontSendNotification);
    emptyStateLabel_->setJustificationType(juce::Justification::centred);
    emptyStateLabel_->setColour(juce::Label::textColourId, themeService_.getCurrentTheme().textSecondary);
    addChildComponent(emptyStateLabel_.get());
}

OscillatorListComponent::OscillatorListComponent(IThemeService& themeService, IInstanceRegistry& instanceRegistry)
    : instanceRegistry_(instanceRegistry)
    , themeService_(themeService)
{
    setOpaque(true);
    themeService_.addListener(this);

    setTestId("oscillatorList");

    // Toolbar
    toolbar_ = std::make_unique<OscillatorListToolbar>(themeService_);
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
    emptyStateLabel_->setText("No oscillators yet.\nClick '+ Add Oscillator' above to get started.",
                              juce::dontSendNotification);
    emptyStateLabel_->setJustificationType(juce::Justification::centred);
    emptyStateLabel_->setColour(juce::Label::textColourId, themeService_.getCurrentTheme().textSecondary);
    addChildComponent(emptyStateLabel_.get());
}

OscillatorListComponent::~OscillatorListComponent()
{
    removeAllChildren(); // Prevent double-free/UAF in base Component destructor

    themeService_.removeListener(this);
    toolbar_->removeListener(this);

    for (auto& item : items_)
    {
        if (item)
            item->removeListener(this);
    }
}

void OscillatorListComponent::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

void OscillatorListComponent::paint(juce::Graphics& g)
{
    auto& theme = themeService_.getCurrentTheme();
    g.fillAll(theme.backgroundSecondary);

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

std::vector<Oscillator> OscillatorListComponent::filterOscillators(const std::vector<Oscillator>& oscillators) const
{
    std::vector<Oscillator> filtered;
    filtered.reserve(oscillators.size());
    for (const auto& osc : oscillators)
    {
        bool include = false;
        switch (currentFilterMode_)
        {
            case OscillatorFilterMode::All:
                include = true;
                break;
            case OscillatorFilterMode::Visible:
                include = osc.isVisible();
                break;
            case OscillatorFilterMode::Hidden:
                include = !osc.isVisible();
                break;
        }
        if (include)
            filtered.push_back(osc);
    }
    return filtered;
}

void OscillatorListComponent::syncContainerChildren()
{
    if (static_cast<size_t>(container_->getNumChildComponents()) != items_.size())
    {
        container_->removeAllChildren();
        for (auto& item : items_)
            container_->addAndMakeVisible(*item);
        return;
    }

    for (int i = 0; i < static_cast<int>(items_.size()); ++i)
    {
        if (container_->getChildComponent(i) != items_[static_cast<size_t>(i)].get())
        {
            container_->removeAllChildren();
            for (auto& item : items_)
                container_->addAndMakeVisible(*item);
            return;
        }
    }
}

void OscillatorListComponent::rebuildItems(
    const std::vector<Oscillator>& filtered,
    std::unordered_map<juce::String, std::unique_ptr<OscillatorListItemComponent>>& reusedItems)
{
    for (const auto& osc : filtered)
    {
        std::unique_ptr<OscillatorListItemComponent> item;

        auto it = reusedItems.find(osc.getId().id);
        if (it != reusedItems.end())
        {
            item = std::move(it->second);
            reusedItems.erase(it);
            item->updateFromOscillator(osc);
            juce::Logger::writeToLog("[OscList] Reused item for " + osc.getName() +
                                     " order=" + juce::String(osc.getOrderIndex()));
        }
        else
        {
            item = std::make_unique<OscillatorListItemComponent>(osc, instanceRegistry_, themeService_);
            juce::Logger::writeToLog("[OscList] Created new item for " + osc.getName() +
                                     " order=" + juce::String(osc.getOrderIndex()));
        }

        item->setSelected(osc.getId() == selectedOscillatorId_);
        item->addListener(this);

        if (item->getParentComponent() != container_.get())
            container_->addAndMakeVisible(*item);

        items_.push_back(std::move(item));
    }
}

void OscillatorListComponent::refreshList(const std::vector<Oscillator>& oscillators)
{
    allOscillators_ = oscillators;
    auto filtered = filterOscillators(oscillators);

    juce::Logger::writeToLog("[OscList] refreshList: " + juce::String(static_cast<int>(oscillators.size())) +
                             " total, " + juce::String(static_cast<int>(filtered.size())) + " filtered, " +
                             juce::String(static_cast<int>(items_.size())) + " existing items");

    // Map existing items by ID for reuse
    std::unordered_map<juce::String, std::unique_ptr<OscillatorListItemComponent>> reusedItems;
    for (auto& item : items_)
    {
        if (item)
        {
            item->removeListener(this);
            reusedItems[item->getOscillatorId().id] = std::move(item);
        }
    }
    items_.clear();

    rebuildItems(filtered, reusedItems);
    reusedItems.clear();

    // Assign contiguous test IDs based on position in the displayed (filtered) list
    for (size_t i = 0; i < items_.size(); ++i)
        items_[i]->setListIndex(static_cast<int>(i));

    syncContainerChildren();

    emptyStateLabel_->setVisible(filtered.empty());
    viewport_->setVisible(!filtered.empty());

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

    // Layout needs to update because selected item expands
    resized();
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
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<OscillatorListComponent>(this), id]() {
        if (safeThis != nullptr)
            safeThis->listeners_.call([id](Listener& l) { l.oscillatorDeleteRequested(id); });
    });
}

void OscillatorListComponent::oscillatorDragStarted(const OscillatorId& id)
{
    // Find the component for this ID
    OscillatorListItemComponent* sourceComponent = nullptr;
    int sourceIndex = -1;

    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i]->getOscillatorId() == id)
        {
            sourceComponent = items_[i].get();
            sourceIndex = static_cast<int>(i);
            break;
        }
    }

    if (sourceComponent)
    {
        // Create drag description using DynamicObject properly
        auto* dragObj = new juce::DynamicObject();
        dragObj->setProperty("type", "oscillator");
        dragObj->setProperty("id", id.id);
        dragObj->setProperty("index", sourceIndex);
        juce::var dragDescription(dragObj);

        // Create snapshot for drag image
        juce::Image dragImage = sourceComponent->createComponentSnapshot(sourceComponent->getLocalBounds());

        // Start JUCE drag operation (1.0f = scale factor for the drag image)
        startDragging(dragDescription, sourceComponent, juce::ScaledImage(dragImage), true);
    }
}

bool OscillatorListComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description.isObject() && dragSourceDetails.description.hasProperty("type") &&
        dragSourceDetails.description.getProperty("type", "").toString() == "oscillator")
    {
        return true;
    }
    return false;
}

void OscillatorListComponent::itemDragEnter(const SourceDetails& dragSourceDetails) { itemDragMove(dragSourceDetails); }

void OscillatorListComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    auto mousePos = dragSourceDetails.localPosition;

    // Convert mouse Y to container coordinates to check against items
    int containerY = mousePos.y - viewport_->getY() + viewport_->getViewPositionY();

    int newTarget = getItemIndexAtY(containerY);
    if (newTarget != dragTargetIndex_)
    {
        updateDragIndicator(newTarget);
    }
}

void OscillatorListComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/) { updateDragIndicator(-1); }

void OscillatorListComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    int sourceIndex = static_cast<int>(dragSourceDetails.description.getProperty("index", -1));
    int targetIndex = dragTargetIndex_;

    updateDragIndicator(-1);

    if (sourceIndex != -1 && targetIndex != -1 && sourceIndex != targetIndex)
    {
        if (sourceIndex < targetIndex)
        {
            targetIndex--;
        }

        listeners_.call([sourceIndex, targetIndex](Listener& l) { l.oscillatorsReordered(sourceIndex, targetIndex); });
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

    if (currentIndex == -1)
        return;

    int newIndex = currentIndex + direction;
    if (newIndex >= 0 && newIndex < static_cast<int>(items_.size()))
    {
        listeners_.call([currentIndex, newIndex](Listener& l) { l.oscillatorsReordered(currentIndex, newIndex); });
    }
}

void OscillatorListComponent::oscillatorPaneSelectionRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorPaneSelectionRequested(id); });
}

void OscillatorListComponent::oscillatorNameChanged(const OscillatorId& id, const juce::String& newName)
{
    listeners_.call([id, &newName](Listener& l) { l.oscillatorNameChanged(id, newName); });
}

int OscillatorListComponent::getItemIndexAtY(int y) const
{
    if (items_.empty())
        return 0;

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
    if (dragTargetIndex_ != targetIndex)
    {
        dragTargetIndex_ = targetIndex;
        repaint();
    }
}

void OscillatorListComponent::themeChanged(const ColorTheme&) { repaint(); }

void OscillatorListComponent::filterModeChanged(OscillatorFilterMode mode)
{
    currentFilterMode_ = mode;
    refreshList(allOscillators_);
}

void OscillatorListComponent::addListener(Listener* listener) { listeners_.add(listener); }

void OscillatorListComponent::removeListener(Listener* listener) { listeners_.remove(listener); }

} // namespace oscil