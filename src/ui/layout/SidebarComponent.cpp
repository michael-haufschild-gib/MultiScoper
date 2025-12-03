/*
    Oscil - Sidebar Component Implementation
    Collapsible sidebar with sources list and oscillator list
*/

#include "ui/layout/SidebarComponent.h"
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

SidebarComponent::SidebarComponent(IInstanceRegistry& instanceRegistry)
    : instanceRegistry_(instanceRegistry)
{
    ThemeManager::getInstance().addListener(this);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("sidebar");
#endif

    // Create resize handle
    resizeHandle_ = std::make_unique<SidebarResizeHandle>();
    resizeHandle_->onResizeStart = []() {};
    resizeHandle_->onResizeDrag = [this](int deltaX)
    {
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
    resizeHandle_->onResizeEnd = []() {};
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

    // Oscillator List
    oscillatorList_ = std::make_unique<OscillatorListComponent>(instanceRegistry_);
    oscillatorList_->addListener(this);
    addAndMakeVisible(oscillatorList_.get());

    // Control sections in scrollable viewport
    accordionViewport_ = std::make_unique<juce::Viewport>();
    accordion_ = std::make_unique<OscilAccordion>();

    accordionViewport_->setViewedComponent(accordion_.get(), false);
    accordionViewport_->setScrollBarsShown(true, false);
    accordionViewport_->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    addAndMakeVisible(accordionViewport_.get());

    setupSections();
}

SidebarComponent::~SidebarComponent()
{
    ThemeManager::getInstance().removeListener(this);
    if (oscillatorList_)
        oscillatorList_->removeListener(this);
}

void SidebarComponent::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Background
    g.fillAll(theme.backgroundSecondary);

    // Left border
    g.setColour(theme.controlBorder);
    g.drawVerticalLine(0, 0.0f, static_cast<float>(getHeight()));
}

void SidebarComponent::resized()
{
    auto bounds = getLocalBounds();

    // Resize handle on left edge
    resizeHandle_->setBounds(bounds.removeFromLeft(RESIZE_HANDLE_WIDTH));

    if (collapsed_)
    {
        // Collapsed state
        collapseButton_->setBounds(bounds.withSizeKeepingCentre(COLLAPSE_BUTTON_SIZE, COLLAPSE_BUTTON_SIZE));
        addOscillatorButton_->setVisible(false);
        oscillatorList_->setVisible(false);
        accordionViewport_->setVisible(false);
    }
    else
    {
        // Expanded state
        addOscillatorButton_->setVisible(true);
        oscillatorList_->setVisible(true);
        accordionViewport_->setVisible(true);

        // Collapse button at top right
        auto topRow = bounds.removeFromTop(SECTION_HEADER_HEIGHT).reduced(PADDING_SMALL, PADDING_SMALL);
        collapseButton_->setBounds(topRow.removeFromRight(COLLAPSE_BUTTON_SIZE));

        // Add Oscillator button at top
        auto buttonArea = bounds.removeFromTop(ADD_BUTTON_HEIGHT).reduced(PADDING_MEDIUM, PADDING_SMALL);
        addOscillatorButton_->setBounds(buttonArea);

        // Oscillator List area
        // Flexible height based on list content size, but constrained
        // Ideally we'd ask the list for preferred height, but it's dynamic.
        // We'll give it a fixed slice for now or let it expand.
        // Let's give it a reasonable fixed slice or based on actual item count if we could know it easily.
        // For now, let's use MAX_LIST_HEIGHT as the allocated space, and let the list viewport handle scrolling.
        // Wait, previously we calculated height based on items. OscillatorListComponent manages its own internal viewport.
        // We should give it a fixed area or dynamic area.
        // Let's give it MAX_LIST_HEIGHT for now, or remaining space if less.
        
        auto listArea = bounds.removeFromTop(MAX_LIST_HEIGHT).reduced(PADDING_SMALL, 0);
        oscillatorList_->setBounds(listArea);

        // Control sections viewport - takes remaining space
        accordionViewport_->setBounds(bounds.reduced(PADDING_SMALL, 0));
        
        // Update accordion width to match viewport
        if (accordion_)
        {
            int width = accordionViewport_->getWidth() - (accordionViewport_->getScrollBarThickness() + PADDING_SMALL);
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
    if (oscillatorList_)
        oscillatorList_->refreshList(oscillators);
}

void SidebarComponent::setSelectedOscillator(const OscillatorId& oscillatorId)
{
    if (selectedOscillatorId_ != oscillatorId)
    {
        selectedOscillatorId_ = oscillatorId;
        if (oscillatorList_)
            oscillatorList_->setSelectedOscillator(oscillatorId);
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

void SidebarComponent::refreshSourceList(const std::vector<SourceInfo>& /*sources*/)
{
    // Handled by PluginEditor/AddOscillatorDialog
}

void SidebarComponent::refreshPaneList(const std::vector<Pane>& panes)
{
    currentPanes_ = panes;
}

void SidebarComponent::setupSections()
{
    accordion_->clearSections();
    accordion_->setAllowMultiExpand(true);

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

// OscillatorListComponent::Listener implementation
void SidebarComponent::oscillatorSelected(const OscillatorId& id)
{
    selectedOscillatorId_ = id;
    listeners_.call([id](Listener& l) { l.oscillatorSelected(id); });
}

void SidebarComponent::oscillatorVisibilityChanged(const OscillatorId& id, bool visible)
{
    listeners_.call([id, visible](Listener& l) { l.oscillatorVisibilityChanged(id, visible); });
}

void SidebarComponent::oscillatorModeChanged(const OscillatorId& id, ProcessingMode mode)
{
    listeners_.call([id, mode](Listener& l) { l.oscillatorModeChanged(id, mode); });
}

void SidebarComponent::oscillatorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorConfigRequested(id); });
}

void SidebarComponent::oscillatorColorConfigRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorColorConfigRequested(id); });
}

void SidebarComponent::oscillatorDeleteRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorDeleteRequested(id); });
}

void SidebarComponent::oscillatorsReordered(int fromIndex, int toIndex)
{
    listeners_.call([fromIndex, toIndex](Listener& l) { l.oscillatorsReordered(fromIndex, toIndex); });
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

void SidebarComponent::timeIntervalChanged(float ms)
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
