/*
    Oscil - Sidebar Component Implementation
    Collapsible sidebar with sources list and oscillator list
*/

#include "ui/layout/SidebarComponent.h"
#include "ui/components/TestId.h"
#include "ui/animation/OscilAnimationService.h"

namespace oscil
{

// SidebarResizeHandle implementation

SidebarResizeHandle::SidebarResizeHandle(IThemeService& themeService)
    : themeService_(themeService)
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("sidebar_resizeHandle");
#endif
}

void SidebarResizeHandle::paint(juce::Graphics& g)
{
    auto& theme = themeService_.getCurrentTheme();

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

SidebarCollapseButton::SidebarCollapseButton(IThemeService& themeService)
    : themeService_(themeService)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void SidebarCollapseButton::paint(juce::Graphics& g)
{
    auto& theme = themeService_.getCurrentTheme();
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

SidebarComponent::SidebarComponent(ServiceContext& context)
    : context_(context)
    , themeService_(context.themeService)
    , instanceRegistry_(context.instanceRegistry)
{
    themeService_.addListener(this);

    // Initialize width value to expanded width
    currentWidthValue_ = static_cast<float>(expandedWidth_);

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("sidebar");
#endif

    // Create resize handle
    resizeHandle_ = std::make_unique<SidebarResizeHandle>(themeService_);
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
    collapseButton_ = std::make_unique<SidebarCollapseButton>(themeService_);
    collapseButton_->onClick = [this]() { toggleCollapsed(); };
    addAndMakeVisible(collapseButton_.get());

    // All sections in scrollable viewport
    accordionViewport_ = std::make_unique<juce::Viewport>();
    accordion_ = std::make_unique<OscilAccordion>(themeService_);

    accordionViewport_->setViewedComponent(accordion_.get(), false);
    accordionViewport_->setScrollBarsShown(true, false);
    accordionViewport_->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    addAndMakeVisible(accordionViewport_.get());

    setupSections();

    // Force initial layout to size accordion after sections are added
    resized();
}

SidebarComponent::~SidebarComponent()
{
    // ScopedAnimator handles completion on destruction
    themeService_.removeListener(this);
    if (oscillatorSection_)
        oscillatorSection_->removeListener(this);
    if (timingSection_)
        timingSection_->removeListener(this);
    if (optionsSection_)
        optionsSection_->removeListener(this);
}

void SidebarComponent::parentHierarchyChanged()
{
    juce::Component::parentHierarchyChanged();
    if (auto* service = findAnimationService(this))
    {
        animService_ = service;
    }
}

void SidebarComponent::paint(juce::Graphics& g)
{
    auto& theme = themeService_.getCurrentTheme();

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
        accordionViewport_->setVisible(false);
    }
    else
    {
        // Expanded state
        accordionViewport_->setVisible(true);

        // Collapse button at top right
        auto topRow = bounds.removeFromTop(SECTION_HEADER_HEIGHT).reduced(PADDING_SMALL, PADDING_SMALL);
        collapseButton_->setBounds(topRow.removeFromRight(COLLAPSE_BUTTON_SIZE));

        // All sections in accordion viewport - takes remaining space
        accordionViewport_->setBounds(bounds.reduced(PADDING_SMALL, 0));

        // Update accordion width to match viewport
        if (accordion_)
        {
            int width = juce::jmax(1, accordionViewport_->getWidth() - (accordionViewport_->getScrollBarThickness() + PADDING_SMALL));
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

        // Animate width transition
        float targetWidth = collapsed ? static_cast<float>(WindowLayout::COLLAPSED_SIDEBAR_WIDTH)
                                       : static_cast<float>(expandedWidth_);

        if (animService_ && OscilAnimationService::shouldAnimate())
        {
            float startWidth = currentWidthValue_;
            juce::WeakReference<SidebarComponent> weakThis(this);
            widthAnimator_.set(animService_->createExpandAnimation(
                [weakThis, startWidth, targetWidth](float v) {
                    if (auto* sidebar = weakThis.get())
                    {
                        sidebar->currentWidthValue_ = juce::jmap(v, startWidth, targetWidth);
                        sidebar->notifySidebarWidthChanged();
                    }
                }));
            widthAnimator_.start();
        }
        else
        {
            currentWidthValue_ = targetWidth;
            notifySidebarWidthChanged();
        }

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

        // If not collapsed, update width immediately (no animation during resize drag)
        if (!collapsed_)
        {
            currentWidthValue_ = static_cast<float>(newWidth);
        }

        notifySidebarWidthChanged();
    }
}

int SidebarComponent::getEffectiveWidth() const
{
    // Return animated width during transitions
    return static_cast<int>(currentWidthValue_);
}

void SidebarComponent::refreshOscillatorList(const std::vector<Oscillator>& oscillators)
{
    if (oscillatorSection_)
        oscillatorSection_->refreshList(oscillators);
}

void SidebarComponent::setSelectedOscillator(const OscillatorId& oscillatorId)
{
    if (selectedOscillatorId_ != oscillatorId)
    {
        selectedOscillatorId_ = oscillatorId;
        if (oscillatorSection_)
            oscillatorSection_->setSelectedOscillator(oscillatorId);
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

    // Create oscillator section (first section with add button and oscillator list)
    oscillatorSection_ = std::make_unique<OscillatorSidebarSection>(context_);
    oscillatorSection_->addListener(this);

    auto* oscillators = accordion_->addSection("OSCILLATORS", oscillatorSection_.get());
    if (oscillators)
    {
        oscillators->setTestId("sidebar_oscillators");
        oscillators->setExpanded(true, false);
    }

    // Create timing section
    timingSection_ = std::make_unique<TimingSidebarSection>(themeService_);
    timingSection_->addListener(this);

    auto* timing = accordion_->addSection("TIMING", timingSection_.get());
    if (timing)
    {
        timing->setTestId("sidebar_timing");
        timing->setExpanded(true, false);
    }

    // Create options section
    optionsSection_ = std::make_unique<OptionsSection>(themeService_);
    optionsSection_->addListener(this);

    optionsSection_->setAvailableThemes(themeService_.getAvailableThemes());
    optionsSection_->setCurrentTheme(themeService_.getCurrentTheme().name);

    auto* options = accordion_->addSection("OPTIONS", optionsSection_.get());
    if (options)
    {
        options->setTestId("sidebar_options");
        options->setExpanded(true, false);
    }
}

// OscillatorSidebarSection::Listener implementation
void SidebarComponent::addOscillatorDialogRequested()
{
    listeners_.call([](Listener& l) { l.addOscillatorDialogRequested(); });
}

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

void SidebarComponent::oscillatorPaneSelectionRequested(const OscillatorId& id)
{
    listeners_.call([id](Listener& l) { l.oscillatorPaneSelectionRequested(id); });
}

void SidebarComponent::oscillatorNameChanged(const OscillatorId& id, const juce::String& newName)
{
    // Name changes are handled internally by OscillatorListComponent
    juce::ignoreUnused(id, newName);
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

void SidebarComponent::layoutChanged(int columnCount)
{
    listeners_.call([columnCount](Listener& l) { l.layoutChanged(columnCount); });
}

void SidebarComponent::themeChanged(const juce::String& themeName)
{
    // Capture by value to avoid dangling reference if listener triggers re-entrancy
    listeners_.call([themeName](Listener& l) { l.themeChanged(themeName); });
}

void SidebarComponent::gpuRenderingChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.gpuRenderingChanged(enabled); });
}

void SidebarComponent::qualityPresetChanged(QualityPreset preset)
{
    listeners_.call([preset](Listener& l) { l.qualityPresetChanged(preset); });
}

void SidebarComponent::bufferDurationChanged(BufferDuration duration)
{
    listeners_.call([duration](Listener& l) { l.bufferDurationChanged(duration); });
}

void SidebarComponent::autoAdjustQualityChanged(bool enabled)
{
    listeners_.call([enabled](Listener& l) { l.autoAdjustQualityChanged(enabled); });
}

} // namespace oscil
