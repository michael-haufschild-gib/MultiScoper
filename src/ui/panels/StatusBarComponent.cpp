/*
    Oscil - Status Bar Component Implementation
*/

#include "ui/panels/StatusBarComponent.h"
#include "ui/theme/ThemeManager.h"
#include "ui/components/TestId.h"

namespace oscil
{

StatusBarComponent::StatusBarComponent(IThemeService& themeService)
    : themeService_(themeService)
{
    setOpaque(true);
    // Detect rendering mode at construction time
    renderingMode_ = detectRenderingMode();

    auto createLabel = [this](std::unique_ptr<juce::Label>& label, const juce::String& testId) {
        label = std::make_unique<juce::Label>();
        label->setFont(juce::FontOptions(11.0f));
        label->setJustificationType(juce::Justification::centredLeft);
        addChildComponent(*label);
        
#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
        label->setComponentID(testId);
#else
        juce::ignoreUnused(testId);
#endif
    };

    createLabel(fpsLabel_, "statusBar_fps");
    createLabel(cpuLabel_, "statusBar_cpu");
    createLabel(memoryLabel_, "statusBar_mem");
    createLabel(oscillatorLabel_, "statusBar_osc");
    createLabel(sourceLabel_, "statusBar_src");
    
    renderModeLabel_ = std::make_unique<juce::Label>();
    renderModeLabel_->setFont(juce::FontOptions(11.0f));
    renderModeLabel_->setJustificationType(juce::Justification::centredRight);
    addChildComponent(*renderModeLabel_);
#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    renderModeLabel_->setComponentID("statusBar_mode");
#endif

    // Initialize text
    updateFpsLabel();
    updateCpuLabel();
    updateMemoryLabel();
    updateOscillatorLabel();
    updateSourceLabel();
    updateRenderModeLabel();

#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID("statusBar");
#endif
}

RenderingMode StatusBarComponent::detectRenderingMode()
{
#if OSCIL_ENABLE_OPENGL
    return RenderingMode::OpenGL;
#else
    return RenderingMode::Software;
#endif
}

void StatusBarComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto& theme = themeService_.getCurrentTheme();

    // Draw background
    g.setColour(theme.backgroundSecondary);
    g.fillRect(bounds);

    // Draw top border
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
}

void StatusBarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10, 0);
    
    // Manually position right-aligned item and reserve space
    if (renderModeLabel_)
    {
        renderModeLabel_->setBounds(bounds.removeFromRight(80));
        bounds.removeFromRight(10); // Spacing
    }

    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    flex.alignItems = juce::FlexBox::AlignItems::center;

    // Add left-aligned items
    flex.items.add(juce::FlexItem(*fpsLabel_).withWidth(70).withHeight(bounds.getHeight()));
    flex.items.add(juce::FlexItem(*cpuLabel_).withWidth(80).withHeight(bounds.getHeight()));
    flex.items.add(juce::FlexItem(*memoryLabel_).withWidth(90).withHeight(bounds.getHeight()));
    flex.items.add(juce::FlexItem(*oscillatorLabel_).withWidth(60).withHeight(bounds.getHeight()));
    flex.items.add(juce::FlexItem(*sourceLabel_).withWidth(60).withHeight(bounds.getHeight()));

    // Perform layout for left items
    flex.performLayout(bounds);
}

void StatusBarComponent::setFps(float fps)
{
    if (std::abs(currentFps_ - fps) > 0.1f)
    {
        currentFps_ = fps;
        updateFpsLabel();
    }
}

void StatusBarComponent::setCpuUsage(float cpu)
{
    if (std::abs(cpuUsage_ - cpu) > 0.1f)
    {
        cpuUsage_ = cpu;
        updateCpuLabel();
    }
}

void StatusBarComponent::setMemoryUsage(float memory)
{
    if (std::abs(memoryUsage_ - memory) > 0.1f)
    {
        memoryUsage_ = memory;
        updateMemoryLabel();
    }
}

void StatusBarComponent::setOscillatorCount(int count)
{
    if (oscillatorCount_ != count)
    {
        oscillatorCount_ = count;
        updateOscillatorLabel();
    }
}

void StatusBarComponent::setSourceCount(int count)
{
    if (sourceCount_ != count)
    {
        sourceCount_ = count;
        updateSourceLabel();
    }
}

void StatusBarComponent::setRenderingMode(RenderingMode mode)
{
    if (renderingMode_ != mode)
    {
        renderingMode_ = mode;
        updateRenderModeLabel();
    }
}

void StatusBarComponent::updateFpsLabel()
{
    if (!fpsLabel_) return;
    
    const auto& theme = themeService_.getCurrentTheme();
    juce::Colour fpsColour = theme.statusActive;
    
    if (currentFps_ < 30.0f)
        fpsColour = theme.statusError;
    else if (currentFps_ < 55.0f)
        fpsColour = theme.statusWarning;

    fpsLabel_->setText(juce::String::formatted("%.1f FPS", currentFps_), juce::dontSendNotification);
    fpsLabel_->setColour(juce::Label::textColourId, fpsColour);
}

void StatusBarComponent::updateCpuLabel()
{
    if (!cpuLabel_) return;

    const auto& theme = themeService_.getCurrentTheme();
    cpuLabel_->setText(juce::String::formatted("CPU: %.1f%%", cpuUsage_), juce::dontSendNotification);
    cpuLabel_->setColour(juce::Label::textColourId, cpuUsage_ > 10.0f ? theme.statusWarning : theme.textSecondary);
}

void StatusBarComponent::updateMemoryLabel()
{
    if (!memoryLabel_) return;

    const auto& theme = themeService_.getCurrentTheme();
    memoryLabel_->setText(juce::String::formatted("Mem: %.1f MB", memoryUsage_), juce::dontSendNotification);
    memoryLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

void StatusBarComponent::updateOscillatorLabel()
{
    if (!oscillatorLabel_) return;

    const auto& theme = themeService_.getCurrentTheme();
    oscillatorLabel_->setText(juce::String::formatted("Osc: %d", oscillatorCount_), juce::dontSendNotification);
    oscillatorLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

void StatusBarComponent::updateSourceLabel()
{
    if (!sourceLabel_) return;

    const auto& theme = themeService_.getCurrentTheme();
    sourceLabel_->setText(juce::String::formatted("Src: %d", sourceCount_), juce::dontSendNotification);
    sourceLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

void StatusBarComponent::updateRenderModeLabel()
{
    if (!renderModeLabel_) return;

    const auto& theme = themeService_.getCurrentTheme();
    juce::String renderModeText = (renderingMode_ == RenderingMode::OpenGL) ? "OpenGL" : "Software";
    renderModeLabel_->setText(renderModeText, juce::dontSendNotification);
    renderModeLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
}

} // namespace oscil