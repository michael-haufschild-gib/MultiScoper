/*
    Oscil - State Builder
    Fluent builder for creating OscilState objects in tests
*/

#pragma once

#include "core/OscilState.h"
#include "core/Oscillator.h"

#include <memory>
#include <vector>

namespace oscil::test
{

/**
 * Fluent builder for creating test OscilState objects
 *
 * Example usage:
 *   auto state = StateBuilder()
 *       .withThemeName("Dark Professional")
 *       .withColumnLayout(ColumnLayout::Double)
 *       .withOscillator(oscillator1)
 *       .withOscillator(oscillator2)
 *       .build();
 */
class StateBuilder
{
public:
    StateBuilder()
        : themeName_("Dark Professional")
        , columnLayout_(ColumnLayout::Single)
        , sidebarWidth_(300)
        , sidebarCollapsed_(false)
        , statusBarVisible_(true)
        , showGrid_(true)
        , autoScale_(true)
        , gainDb_(0.0f)
        , gpuRenderingEnabled_(true)
    {
    }

    /**
     * Set the theme name
     */
    StateBuilder& withThemeName(const juce::String& themeName)
    {
        themeName_ = themeName;
        return *this;
    }

    /**
     * Set the column layout
     */
    StateBuilder& withColumnLayout(ColumnLayout layout)
    {
        columnLayout_ = layout;
        return *this;
    }

    /**
     * Set single column layout
     */
    StateBuilder& withSingleColumn()
    {
        columnLayout_ = ColumnLayout::Single;
        return *this;
    }

    /**
     * Set double column layout
     */
    StateBuilder& withDoubleColumn()
    {
        columnLayout_ = ColumnLayout::Double;
        return *this;
    }

    /**
     * Set triple column layout
     */
    StateBuilder& withTripleColumn()
    {
        columnLayout_ = ColumnLayout::Triple;
        return *this;
    }

    /**
     * Add an oscillator
     */
    StateBuilder& withOscillator(const Oscillator& oscillator)
    {
        oscillators_.push_back(oscillator);
        return *this;
    }

    /**
     * Add multiple oscillators
     */
    StateBuilder& withOscillators(const std::vector<Oscillator>& oscillators)
    {
        oscillators_.insert(oscillators_.end(), oscillators.begin(), oscillators.end());
        return *this;
    }

    /**
     * Set sidebar width
     */
    StateBuilder& withSidebarWidth(int width)
    {
        sidebarWidth_ = width;
        return *this;
    }

    /**
     * Set sidebar collapsed state
     */
    StateBuilder& withSidebarCollapsed(bool collapsed)
    {
        sidebarCollapsed_ = collapsed;
        return *this;
    }

    /**
     * Collapse the sidebar
     */
    StateBuilder& withCollapsedSidebar()
    {
        sidebarCollapsed_ = true;
        return *this;
    }

    /**
     * Expand the sidebar
     */
    StateBuilder& withExpandedSidebar()
    {
        sidebarCollapsed_ = false;
        return *this;
    }

    /**
     * Set status bar visibility
     */
    StateBuilder& withStatusBarVisible(bool visible)
    {
        statusBarVisible_ = visible;
        return *this;
    }

    /**
     * Show status bar
     */
    StateBuilder& withStatusBar()
    {
        statusBarVisible_ = true;
        return *this;
    }

    /**
     * Hide status bar
     */
    StateBuilder& withoutStatusBar()
    {
        statusBarVisible_ = false;
        return *this;
    }

    /**
     * Set show grid option
     */
    StateBuilder& withShowGrid(bool show)
    {
        showGrid_ = show;
        return *this;
    }

    /**
     * Enable grid
     */
    StateBuilder& withGrid()
    {
        showGrid_ = true;
        return *this;
    }

    /**
     * Disable grid
     */
    StateBuilder& withoutGrid()
    {
        showGrid_ = false;
        return *this;
    }

    /**
     * Set auto scale option
     */
    StateBuilder& withAutoScale(bool autoScale)
    {
        autoScale_ = autoScale;
        return *this;
    }

    /**
     * Enable auto scale
     */
    StateBuilder& withAutoScaling()
    {
        autoScale_ = true;
        return *this;
    }

    /**
     * Disable auto scale
     */
    StateBuilder& withoutAutoScaling()
    {
        autoScale_ = false;
        return *this;
    }

    /**
     * Set gain in dB
     */
    StateBuilder& withGainDb(float gainDb)
    {
        gainDb_ = gainDb;
        return *this;
    }

    /**
     * Set GPU rendering enabled
     */
    StateBuilder& withGpuRendering(bool enabled)
    {
        gpuRenderingEnabled_ = enabled;
        return *this;
    }

    /**
     * Enable GPU rendering
     */
    StateBuilder& withGpu()
    {
        gpuRenderingEnabled_ = true;
        return *this;
    }

    /**
     * Disable GPU rendering (CPU fallback)
     */
    StateBuilder& withCpuRendering()
    {
        gpuRenderingEnabled_ = false;
        return *this;
    }

    /**
     * Build and return a unique_ptr to the OscilState
     */
    std::unique_ptr<OscilState> buildUnique()
    {
        auto state = std::make_unique<OscilState>();

        // Set properties
        state->setThemeName(themeName_);
        state->setColumnLayout(columnLayout_);
        state->setSidebarWidth(sidebarWidth_);
        state->setSidebarCollapsed(sidebarCollapsed_);
        state->setStatusBarVisible(statusBarVisible_);
        state->setShowGridEnabled(showGrid_);
        state->setAutoScaleEnabled(autoScale_);
        state->setGainDb(gainDb_);
        state->setGpuRenderingEnabled(gpuRenderingEnabled_);

        // Add oscillators
        for (const auto& osc : oscillators_)
        {
            state->addOscillator(osc);
        }

        return state;
    }

    /**
     * Build and return an OscilState by value
     * NOTE: Disabled because OscilState has deleted copy constructor
     */
    // OscilState build()
    // {
    //     auto ptr = buildUnique();
    //     return std::move(*ptr);
    // }

    /**
     * Build and return a shared_ptr to the OscilState
     */
    std::shared_ptr<OscilState> buildShared() { return std::shared_ptr<OscilState>(buildUnique().release()); }

    /**
     * Build and serialize to XML string
     */
    juce::String buildXml()
    {
        auto state = buildUnique();
        return state->toXmlString();
    }

private:
    juce::String themeName_;
    ColumnLayout columnLayout_;
    std::vector<Oscillator> oscillators_;
    int sidebarWidth_;
    bool sidebarCollapsed_;
    bool statusBarVisible_;
    bool showGrid_;
    bool autoScale_;
    float gainDb_;
    bool gpuRenderingEnabled_;
};

} // namespace oscil::test
