/*
    Oscil - Status Bar Component Implementation
*/

#include "ui/panels/StatusBarComponent.h"
#include "ui/theme/ThemeManager.h"
#include "ui/components/TestId.h"

namespace oscil
{

StatusBarComponent::StatusBarComponent()
{
    setOpaque(true);
    // Detect rendering mode at construction time
    renderingMode_ = detectRenderingMode();

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
    const auto& theme = ThemeManager::getInstance().getCurrentTheme();

    // Draw background
    g.setColour(theme.backgroundSecondary);
    g.fillRect(bounds);

    // Draw top border
    g.setColour(theme.controlBorder);
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    // Draw status text
    g.setColour(theme.textSecondary);
    g.setFont(11.0f);

    int x = 10;

    // FPS indicator
    juce::Colour fpsColour = theme.statusActive;
    if (currentFps_ < 30.0f)
        fpsColour = theme.statusError;
    else if (currentFps_ < 55.0f)
        fpsColour = theme.statusWarning;

    g.setColour(fpsColour);
    g.drawText(juce::String::formatted("%.1f FPS", currentFps_), x, 0, 70, bounds.getHeight(), juce::Justification::centredLeft);
    x += 80;

    // CPU usage
    g.setColour(cpuUsage_ > 10.0f ? theme.statusWarning : theme.textSecondary);
    g.drawText(juce::String::formatted("CPU: %.1f%%", cpuUsage_), x, 0, 80, bounds.getHeight(), juce::Justification::centredLeft);
    x += 90;

    // Memory usage
    g.setColour(theme.textSecondary);
    g.drawText(juce::String::formatted("Mem: %.1f MB", memoryUsage_), x, 0, 90, bounds.getHeight(), juce::Justification::centredLeft);
    x += 100;

    // Oscillator count
    g.drawText(juce::String::formatted("Osc: %d", oscillatorCount_), x, 0, 60, bounds.getHeight(), juce::Justification::centredLeft);
    x += 70;

    // Source count
    g.drawText(juce::String::formatted("Src: %d", sourceCount_), x, 0, 60, bounds.getHeight(), juce::Justification::centredLeft);

    // Rendering mode indicator (right-aligned)
    juce::String renderModeText = (renderingMode_ == RenderingMode::OpenGL) ? "OpenGL" : "Software";
    g.setColour(theme.textSecondary);
    g.drawText(renderModeText, bounds.getWidth() - 80, 0, 70, bounds.getHeight(), juce::Justification::centredRight);
}

} // namespace oscil
