/*
    Oscil - Sidebar Component Implementation
    Collapsible sidebar with sources list and oscillator list
*/

#include "ui/SidebarComponent.h"
#include "core/PluginProcessor.h"
#include "ui/components/TestId.h"

namespace oscil
{

// SidebarResizeHandle implementation

SidebarResizeHandle::SidebarResizeHandle()
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("sidebar_resizeHandle");
#endif
}

void SidebarResizeHandle::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();

    if (isDragging_ || isHovered_)
    {
        g.setColour(theme.controlActive);
    }
    else
    {
        g.setColour(theme.controlBorder);
    }

    // Draw resize grip lines
    auto bounds = getLocalBounds();
    int centerX = bounds.getCentreX();
    int topY = bounds.getHeight() / 3;
    int bottomY = bounds.getHeight() * 2 / 3;

    g.drawVerticalLine(centerX - 1, static_cast<float>(topY), static_cast<float>(bottomY));
    g.drawVerticalLine(centerX + 1, static_cast<float>(topY), static_cast<float>(bottomY));
}

void SidebarResizeHandle::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SidebarResizeHandle::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

void SidebarResizeHandle::mouseDown(const juce::MouseEvent& e)
{
    isDragging_ = true;
    dragStartX_ = e.getScreenX();
    if (onResizeStart)
        onResizeStart();
    repaint();
}

void SidebarResizeHandle::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging_ && onResizeDrag)
    {
        int deltaX = e.getScreenX() - dragStartX_;
        onResizeDrag(deltaX);
    }
}

void SidebarResizeHandle::mouseUp(const juce::MouseEvent&)
{
    isDragging_ = false;
    if (onResizeEnd)
        onResizeEnd();
    repaint();
}

// SidebarCollapseButton implementation

SidebarCollapseButton::SidebarCollapseButton()
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void SidebarCollapseButton::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background
    if (isHovered_)
    {
        g.setColour(theme.controlHighlight);
        g.fillRoundedRectangle(bounds, 4.0f);
    }

    // Draw chevron
    g.setColour(theme.textPrimary);
    juce::Path chevron;

    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float size = 6.0f;

    if (collapsed_)
    {
        // Chevron pointing right (expand)
        chevron.startNewSubPath(centerX - size / 2, centerY - size);
        chevron.lineTo(centerX + size / 2, centerY);
        chevron.lineTo(centerX - size / 2, centerY + size);
    }
    else
    {
        // Chevron pointing left (collapse)
        chevron.startNewSubPath(centerX + size / 2, centerY - size);
        chevron.lineTo(centerX - size / 2, centerY);
        chevron.lineTo(centerX + size / 2, centerY + size);
    }

    g.strokePath(chevron, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void SidebarCollapseButton::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SidebarCollapseButton::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

void SidebarCollapseButton::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()) && onClick)
        onClick();
}

void SidebarCollapseButton::setCollapsed(bool collapsed)
{
    collapsed_ = collapsed;
    repaint();
}

// SidebarComponent implementation

SidebarComponent::SidebarComponent(OscilPluginProcessor& processor)
    : processor_(processor)
{
    // processor_ stored for potential future direct state access
    // Currently all operations go through the Listener interface
    juce::ignoreUnused(processor_);
    ThemeManager::getInstance().addListener(this);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("sidebar");
#endif

    // Create resize handle (on left edge since sidebar is on right)
    resizeHandle_ = std::make_unique<SidebarResizeHandle>();
    resizeHandle_->onResizeStart = []()
    {
        // Store current width for delta calculations
    };
    resizeHandle_->onResizeDrag = [this](int deltaX)
    {
        // Moving left increases width (sidebar is on right)
        int newWidth = expandedWidth_ - deltaX;
        newWidth = juce::jlimit(WindowLayout::MIN_SIDEBAR_WIDTH,
                                WindowLayout::MAX_SIDEBAR_WIDTH,
                                newWidth);

        if (newWidth != expandedWidth_)
        {
            expandedWidth_ = newWidth;
            notifySidebarWidthChanged();
        }
    };
    resizeHandle_->onResizeEnd = []()
    {
        // Finalize resize
    };
    addAndMakeVisible(resizeHandle_.get());

    // Create collapse button
    collapseButton_ = std::make_unique<SidebarCollapseButton>();
    collapseButton_->onClick = [this]() { toggleCollapsed(); };
    addAndMakeVisible(collapseButton_.get());

    // Add Oscillator button
    addOscillatorButton_ = std::make_unique<OscilButton>("+ Add Oscillator", "sidebar_addOscillator");
    addOscillatorButton_->setVariant(ButtonVariant::Primary);
    addOscillatorButton_->onClick = [this]() {
        listeners_.call([](Listener& l) { l.addOscillatorDialogRequested(); });
    };
    addAndMakeVisible(addOscillatorButton_.get());

    // Oscillators section with toolbar
    oscillatorToolbar_ = std::make_unique<OscillatorListToolbar>();
    oscillatorToolbar_->addListener(this);
    addAndMakeVisible(oscillatorToolbar_.get());

    listViewport_ = std::make_unique<juce::Viewport>();
    listContainer_ = std::make_unique<juce::Component>();
    listViewport_->setViewedComponent(listContainer_.get(), false);
    listViewport_->setScrollBarsShown(true, false);
    listViewport_->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);  // Allow items to handle drag
    addAndMakeVisible(listViewport_.get());

    // Empty state label (shown when no oscillators)
    emptyStateLabel_ = std::make_unique<juce::Label>();
    emptyStateLabel_->setText("No oscillators yet.\nClick '+ Add Oscillator' above to get started.", juce::dontSendNotification);
    emptyStateLabel_->setJustificationType(juce::Justification::centred);
    emptyStateLabel_->setColour(juce::Label::textColourId, ThemeManager::getInstance().getCurrentTheme().textSecondary);
    addChildComponent(emptyStateLabel_.get());  // Initially hidden

    // Control sections in scrollable viewport
    accordionViewport_ = std::make_unique<juce::Viewport>();
    accordion_ = std::make_unique<OscilAccordion>();

    accordionViewport_->setViewedComponent(accordion_.get(), false);
    accordionViewport_->setScrollBarsShown(true, false);
    accordionViewport_->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);  // Prevent trackpad gestures from scrolling
    addAndMakeVisible(accordionViewport_.get());

    setupSections();
}

SidebarComponent::~SidebarComponent()
{
    // Clean up global mouse listener if drag was in progress
    if (isDraggingItem_)
    {
        juce::Desktop::getInstance().removeGlobalMouseListener(this);
    }

    ThemeManager::getInstance().removeListener(this);
    if (oscillatorToolbar_)
        oscillatorToolbar_->removeListener(this);

    // Remove listeners from oscillator items
    for (auto& item : oscillatorItems_)
    {
        if (item)
            item->removeListener(this);
    }
}

void SidebarComponent::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Background
    g.fillAll(theme.backgroundSecondary);

    // Left border
    g.setColour(theme.controlBorder);
    g.drawVerticalLine(0, 0.0f, static_cast<float>(getHeight()));

    // Draw drag indicator line when dragging
    if (isDraggingItem_ && dragTargetIndex_ >= 0 && listContainer_ && listViewport_)
    {
        // Calculate y position of the indicator in local coordinates
        int indicatorY = listViewport_->getY();
        int currentY = 0;

        // Sum up heights of items before the target index
        for (int i = 0; i < dragTargetIndex_ && i < static_cast<int>(oscillatorItems_.size()); ++i)
        {
            currentY += oscillatorItems_[static_cast<size_t>(i)]->getPreferredHeight();
        }

        // Adjust for scroll position
        indicatorY += currentY - listViewport_->getViewPosition().y;

        // Draw the indicator line
        g.setColour(theme.controlActive);
        float lineY = static_cast<float>(indicatorY);
        float leftX = static_cast<float>(RESIZE_HANDLE_WIDTH + 4);
        float rightX = static_cast<float>(getWidth() - 8);

        g.drawLine(leftX, lineY, rightX, lineY, 2.0f);

        // Draw small triangles at each end
        juce::Path leftArrow;
        leftArrow.addTriangle(leftX, lineY - 4.0f, leftX, lineY + 4.0f, leftX + 6.0f, lineY);
        g.fillPath(leftArrow);

        juce::Path rightArrow;
        rightArrow.addTriangle(rightX, lineY - 4.0f, rightX, lineY + 4.0f, rightX - 6.0f, lineY);
        g.fillPath(rightArrow);
    }

    // Draw drag ghost image
    if (isDraggingItem_ && dragImage_.isValid())
    {
        // Position the ghost centered horizontally, offset slightly from cursor
        int ghostX = RESIZE_HANDLE_WIDTH + 4;
        int ghostY = dragMousePosition_.y - dragImage_.getHeight() / 2;

        // Clamp ghost position to stay within sidebar bounds
        ghostY = juce::jlimit(0, getHeight() - dragImage_.getHeight(), ghostY);

        // Draw with semi-transparency and slight shadow effect
        g.setOpacity(0.75f);
        g.drawImageAt(dragImage_, ghostX, ghostY);

        // Add subtle border around ghost
        g.setOpacity(1.0f);
        g.setColour(theme.controlActive.withAlpha(0.5f));
        g.drawRect(ghostX, ghostY, dragImage_.getWidth(), dragImage_.getHeight(), 1);
    }
}

void SidebarComponent::resized()
{
    auto bounds = getLocalBounds();

    // Resize handle on left edge
    resizeHandle_->setBounds(bounds.removeFromLeft(RESIZE_HANDLE_WIDTH));

    if (collapsed_)
    {
        // Collapsed state - only show collapse button centered
        collapseButton_->setBounds(bounds.withSizeKeepingCentre(24, 24));
        addOscillatorButton_->setVisible(false);
        oscillatorToolbar_->setVisible(false);
        listViewport_->setVisible(false);
        emptyStateLabel_->setVisible(false);
        accordionViewport_->setVisible(false);
    }
    else
    {
        // Expanded state
        bool hasOscillators = !oscillatorItems_.empty();
        addOscillatorButton_->setVisible(true);
        oscillatorToolbar_->setVisible(true);
        listViewport_->setVisible(hasOscillators);
        emptyStateLabel_->setVisible(!hasOscillators);
        accordionViewport_->setVisible(true);

        // Collapse button at top right
        auto topRow = bounds.removeFromTop(SECTION_HEADER_HEIGHT).reduced(4, 4);
        collapseButton_->setBounds(topRow.removeFromRight(24));

        // Add Oscillator button at top
        auto buttonArea = bounds.removeFromTop(ADD_BUTTON_HEIGHT).reduced(8, 4);
        addOscillatorButton_->setBounds(buttonArea);

        // Oscillators toolbar
        auto toolbarArea = bounds.removeFromTop(OSCILLATOR_TOOLBAR_HEIGHT);
        oscillatorToolbar_->setBounds(toolbarArea);

        // Empty state label position (same area as oscillators list)
        if (!hasOscillators)
        {
            auto emptyArea = bounds.reduced(8, 0).withHeight(100);
            emptyStateLabel_->setBounds(emptyArea);
        }

        // Calculate total content height based on each item's preferred height
        int oscillatorsContentHeight = 0;
        for (const auto& item : oscillatorItems_)
        {
            oscillatorsContentHeight += item->getPreferredHeight();
        }

        // Calculate oscillators area height - flexible height for items
        int oscillatorsHeight = std::max(100, std::min(300, oscillatorsContentHeight + 8));

        // Oscillators list area
        auto oscillatorsArea = bounds.removeFromTop(oscillatorsHeight).reduced(4, 0);
        listViewport_->setBounds(oscillatorsArea);

        // Update oscillators container size
        listContainer_->setSize(listViewport_->getWidth() - 8,
                                 std::max(oscillatorsContentHeight, oscillatorsArea.getHeight()));

        // Position oscillator items using their preferred heights
        int y = 0;
        for (auto& item : oscillatorItems_)
        {
            int itemHeight = item->getPreferredHeight();
            item->setBounds(0, y, listContainer_->getWidth(), itemHeight);
            y += itemHeight;
        }

        // Control sections viewport - takes remaining space
        accordionViewport_->setBounds(bounds.reduced(4, 0));
        
        // Update accordion width to match viewport
        if (accordion_)
        {
            int width = accordionViewport_->getWidth() - (accordionViewport_->getScrollBarThickness() + 4);
            accordion_->setSize(width, accordion_->getPreferredHeight());
        }
    }
}

void SidebarComponent::themeChanged(const ColorTheme&)
{
    repaint();
}

void SidebarComponent::setCollapsed(bool collapsed)
{
    if (collapsed_ != collapsed)
    {
        collapsed_ = collapsed;
        collapseButton_->setCollapsed(collapsed);
        resized();
        notifySidebarCollapsedStateChanged();
    }
}

void SidebarComponent::toggleCollapsed()
{
    setCollapsed(!collapsed_);
}

void SidebarComponent::setSidebarWidth(int width)
{
    int newWidth = juce::jlimit(WindowLayout::MIN_SIDEBAR_WIDTH,
                                WindowLayout::MAX_SIDEBAR_WIDTH,
                                width);
    if (expandedWidth_ != newWidth)
    {
        expandedWidth_ = newWidth;
        notifySidebarWidthChanged();
    }
}

int SidebarComponent::getEffectiveWidth() const
{
    return collapsed_ ? WindowLayout::COLLAPSED_SIDEBAR_WIDTH : expandedWidth_;
}

void SidebarComponent::refreshOscillatorList(const std::vector<Oscillator>& oscillators)
{
    // Remove old listeners
    for (auto& item : oscillatorItems_)
    {
        if (item)
            item->removeListener(this);
    }

    oscillatorItems_.clear();

    // Remove old children from the container before adding new ones
    listContainer_->removeAllChildren();

    // Store all oscillators for filtering
    allOscillators_ = oscillators;

    // Apply filter
    std::vector<Oscillator> filteredOscillators;
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
            filteredOscillators.push_back(osc);
    }

    // Create items for filtered oscillators
    for (const auto& osc : filteredOscillators)
    {
        auto item = std::make_unique<OscillatorListItemComponent>(osc);
        item->setSelected(osc.getId() == selectedOscillatorId_);
        item->addListener(this);
        listContainer_->addAndMakeVisible(*item);
        oscillatorItems_.push_back(std::move(item));
    }

    // Show empty state label if no oscillators
    bool hasOscillators = !filteredOscillators.empty();
    emptyStateLabel_->setVisible(!hasOscillators);
    listViewport_->setVisible(hasOscillators);

    // Update toolbar oscillator count
    updateOscillatorCounts();

    resized();
}

void SidebarComponent::setSelectedOscillator(const OscillatorId& oscillatorId)
{
    if (selectedOscillatorId_ != oscillatorId)
    {
        selectedOscillatorId_ = oscillatorId;

        for (auto& item : oscillatorItems_)
        {
            item->setSelected(item->getOscillatorId() == oscillatorId);
        }

        // Re-layout since selected item has different height
        resized();
    }
}

void SidebarComponent::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void SidebarComponent::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void SidebarComponent::notifySidebarWidthChanged()
{
    listeners_.call([this](Listener& l) { l.sidebarWidthChanged(expandedWidth_); });
}

void SidebarComponent::notifySidebarCollapsedStateChanged()
{
    listeners_.call([this](Listener& l) { l.sidebarCollapsedStateChanged(collapsed_); });
}

void SidebarComponent::notifyOscillatorSelected(const OscillatorId& oscillatorId)
{
    listeners_.call([&oscillatorId](Listener& l) { l.oscillatorSelected(oscillatorId); });
}

void SidebarComponent::notifyOscillatorConfigRequested(const OscillatorId& oscillatorId)
{
    listeners_.call([&oscillatorId](Listener& l) { l.oscillatorConfigRequested(oscillatorId); });
}

void SidebarComponent::notifyOscillatorDeleteRequested(const OscillatorId& oscillatorId)
{
    listeners_.call([&oscillatorId](Listener& l) { l.oscillatorDeleteRequested(oscillatorId); });
}

void SidebarComponent::notifyOscillatorModeChanged(const OscillatorId& oscillatorId, ProcessingMode mode)
{
    listeners_.call([&oscillatorId, mode](Listener& l) { l.oscillatorModeChanged(oscillatorId, mode); });
}

void SidebarComponent::notifyOscillatorVisibilityChanged(const OscillatorId& oscillatorId, bool visible)
{
    listeners_.call([&oscillatorId, visible](Listener& l) { l.oscillatorVisibilityChanged(oscillatorId, visible); });
}

void SidebarComponent::notifyAddOscillatorRequested(const AddOscillatorDialog::Result& result)
{
    listeners_.call([&result](Listener& l) { l.addOscillatorRequested(result); });
}

void SidebarComponent::notifyOscillatorsReordered(int fromIndex, int toIndex)
{
    listeners_.call([fromIndex, toIndex](Listener& l) { l.oscillatorsReordered(fromIndex, toIndex); });
}

// Drag-drop helper implementations
int SidebarComponent::getItemIndexAtY(int y) const
{
    // Convert y to list container coordinates
    if (!listContainer_ || oscillatorItems_.empty())
        return -1;

    auto viewportPos = listViewport_->getViewPosition();
    int containerY = y - listViewport_->getY() + viewportPos.y;

    // Find which item the y coordinate is in
    int currentY = 0;
    for (size_t i = 0; i < oscillatorItems_.size(); ++i)
    {
        int itemHeight = oscillatorItems_[i]->getPreferredHeight();
        if (containerY < currentY + itemHeight)
        {
            // Check if in top or bottom half
            if (containerY < currentY + itemHeight / 2)
                return static_cast<int>(i);
            else
                return static_cast<int>(i + 1);
        }
        currentY += itemHeight;
    }

    return static_cast<int>(oscillatorItems_.size());
}

void SidebarComponent::updateDragIndicator(int targetIndex)
{
    if (targetIndex != dragTargetIndex_)
    {
        dragTargetIndex_ = targetIndex;
        repaint();
    }
}

void SidebarComponent::finishDrag(int fromIndex, int toIndex)
{
    isDraggingItem_ = false;
    juce::Desktop::getInstance().removeGlobalMouseListener(this);

    // Adjust toIndex if dragging down (to account for the removed item)
    int adjustedToIndex = toIndex;
    if (toIndex > fromIndex)
        adjustedToIndex--;

    if (fromIndex != adjustedToIndex && fromIndex >= 0 && adjustedToIndex >= 0)
    {
        notifyOscillatorsReordered(fromIndex, adjustedToIndex);
    }

    dragSourceIndex_ = -1;
    dragTargetIndex_ = -1;
    dragImage_ = juce::Image();  // Clear the drag ghost image
    repaint();
}

// MouseListener implementation for drag-drop
void SidebarComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingItem_)
    {
        auto localPos = getLocalPoint(e.eventComponent, e.position.toInt());

        // Update drag target for indicator line
        int newTargetIndex = getItemIndexAtY(localPos.y);
        updateDragIndicator(newTargetIndex);

        // Update mouse position for ghost drawing
        dragMousePosition_ = localPos;
        repaint();
    }
}

void SidebarComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (isDraggingItem_)
    {
        finishDrag(dragSourceIndex_, dragTargetIndex_);
    }
}

// OscillatorListItemComponent::Listener implementation
void SidebarComponent::oscillatorSelected(const OscillatorId& id)
{
    setSelectedOscillator(id);
    notifyOscillatorSelected(id);
}

void SidebarComponent::oscillatorVisibilityChanged(const OscillatorId& id, bool visible)
{
    notifyOscillatorVisibilityChanged(id, visible);
}

void SidebarComponent::oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode)
{
    notifyOscillatorModeChanged(id, mode);
}

void SidebarComponent::oscillatorConfigRequested(const OscillatorId& id)
{
    notifyOscillatorConfigRequested(id);
}

void SidebarComponent::oscillatorDeleteRequested(const OscillatorId& id)
{
    // Use callAsync because this callback may trigger refreshOscillatorList()
    // which destroys the OscillatorListItemComponent that's still on the call stack
    juce::MessageManager::callAsync([safeSidebar = juce::Component::SafePointer<SidebarComponent>(this), id]()
    {
        if (safeSidebar != nullptr)
            safeSidebar->notifyOscillatorDeleteRequested(id);
    });
}

void SidebarComponent::oscillatorDragStarted(const OscillatorId& id)
{
    // Find the index of the dragged item
    dragSourceIndex_ = -1;
    for (size_t i = 0; i < oscillatorItems_.size(); ++i)
    {
        if (oscillatorItems_[i]->getOscillatorId() == id)
        {
            dragSourceIndex_ = static_cast<int>(i);
            break;
        }
    }

    if (dragSourceIndex_ >= 0)
    {
        isDraggingItem_ = true;
        draggedOscillatorId_ = id;
        dragTargetIndex_ = dragSourceIndex_;

        // Capture snapshot of the dragged item for visual feedback
        auto* draggedItem = oscillatorItems_[static_cast<size_t>(dragSourceIndex_)].get();
        dragImage_ = draggedItem->createComponentSnapshot(draggedItem->getLocalBounds());

        // Initialize mouse position to the item's current center
        auto itemBounds = draggedItem->getBoundsInParent();
        auto viewportY = listViewport_->getY() - listViewport_->getViewPosition().y;
        dragMousePosition_ = {getWidth() / 2, viewportY + itemBounds.getCentreY()};

        // Start tracking mouse globally for drag
        juce::Desktop::getInstance().addGlobalMouseListener(this);
    }
}

void SidebarComponent::oscillatorMoveRequested(const OscillatorId& id, int direction)
{
    // Find the index of the oscillator to move
    int sourceIndex = -1;
    for (size_t i = 0; i < oscillatorItems_.size(); ++i)
    {
        if (oscillatorItems_[i]->getOscillatorId() == id)
        {
            sourceIndex = static_cast<int>(i);
            break;
        }
    }

    if (sourceIndex < 0)
        return;

    // Calculate target index
    int targetIndex = sourceIndex + direction;

    // Bounds check
    if (targetIndex < 0 || targetIndex >= static_cast<int>(oscillatorItems_.size()))
        return;

    // Reuse existing reorder logic
    finishDrag(sourceIndex, targetIndex);
}

// OscillatorListToolbar::Listener implementation
void SidebarComponent::filterModeChanged(OscillatorFilterMode mode)
{
    currentFilterMode_ = mode;
    // Re-apply with new filter
    refreshOscillatorList(allOscillators_);
}

void SidebarComponent::updateOscillatorCounts()
{
    if (oscillatorToolbar_)
    {
        int totalCount = static_cast<int>(allOscillators_.size());
        int visibleCount = 0;
        for (const auto& osc : allOscillators_)
        {
            if (osc.isVisible())
                visibleCount++;
        }
        oscillatorToolbar_->setOscillatorCount(totalCount, visibleCount);
    }
}

void SidebarComponent::refreshSourceList(const std::vector<SourceInfo>& /*sources*/)
{
    // Sources now handled by PluginEditor for the Add Oscillator dialog
}

void SidebarComponent::refreshPaneList(const std::vector<Pane>& panes)
{
    // Cache panes for PluginEditor to use when showing the Add Oscillator dialog
    currentPanes_ = panes;
}

void SidebarComponent::setupSections()
{
    accordion_->clearSections();
    accordion_->setAllowMultiExpand(true); // Allow multiple sections open

    // Create timing section
    timingSection_ = std::make_unique<TimingSidebarSection>();
    timingSection_->addListener(this);

    auto* timing = accordion_->addSection("TIMING", timingSection_.get());
    if (timing)
    {
        timing->setTestId("sidebar_timing");
        timing->setExpanded(true, false);
    }

    // Create options section
    optionsSection_ = std::make_unique<OptionsSection>();
    optionsSection_->addListener(this);

    // Initialize themes
    auto& themeManager = ThemeManager::getInstance();
    optionsSection_->setAvailableThemes(themeManager.getAvailableThemes());
    optionsSection_->setCurrentTheme(themeManager.getCurrentTheme().name);

    auto* options = accordion_->addSection("OPTIONS", optionsSection_.get());
    if (options)
    {
        options->setTestId("sidebar_options");
        options->setExpanded(true, false);
    }
}

// TimingSidebarSection::Listener implementation
void SidebarComponent::timingModeChanged(TimingMode mode)
{
    listeners_.call([mode](Listener& l) { l.timingModeChanged(mode); });
}

void SidebarComponent::noteIntervalChanged(NoteInterval interval)
{
    listeners_.call([interval](Listener& l) { l.noteIntervalChanged(interval); });
}

void SidebarComponent::timeIntervalChanged(int ms)
{
    listeners_.call([ms](Listener& l) { l.timeIntervalChanged(ms); });
}

void SidebarComponent::hostSyncChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.hostSyncChanged(enabled); });
}

void SidebarComponent::waveformModeChanged(WaveformMode mode)
{
    listeners_.call([mode](Listener& l) { l.waveformModeChanged(mode); });
}

void SidebarComponent::bpmChanged(float bpm)
{
    listeners_.call([bpm](Listener& l) { l.bpmChanged(bpm); });
}

// OptionsSection::Listener implementation
void SidebarComponent::gainChanged(float dB)
{
    listeners_.call([dB](Listener& l) { l.gainChanged(dB); });
}

void SidebarComponent::showGridChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.showGridChanged(enabled); });
}

void SidebarComponent::autoScaleChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.autoScaleChanged(enabled); });
}

void SidebarComponent::holdDisplayChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.holdDisplayChanged(enabled); });
}

void SidebarComponent::layoutChanged(int columnCount)
{
    listeners_.call([columnCount](Listener& l) { l.layoutChanged(columnCount); });
}

void SidebarComponent::themeChanged(const juce::String& themeName)
{
    listeners_.call([&themeName](Listener& l) { l.themeChanged(themeName); });
}

void SidebarComponent::gpuRenderingChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.gpuRenderingChanged(enabled); });
}

} // namespace oscil