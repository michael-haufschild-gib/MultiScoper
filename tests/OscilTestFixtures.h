/*
    Oscil - Test Fixtures

    Provides standardized test fixtures for different testing scenarios:
    - OscilPluginTestFixture: For tests requiring full plugin (processor + editor)
    - OscilComponentTestFixture: For tests of isolated UI components
    - OscilStateTestFixture: For tests of state/model classes
*/

#pragma once

#include <gtest/gtest.h>
#include <juce_events/juce_events.h>
#include <map>
#include <set>

#include "core/AudioCapturePool.h"
#include "core/CaptureThread.h"
#include "core/InstanceRegistry.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/ServiceContext.h"
#include "ui/theme/ThemeManager.h"
#include "ui/theme/IThemeService.h"
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"
#include "rendering/ShaderRegistry.h"

#include "OscilTestUtils.h"

namespace oscil::test
{

// =============================================================================
// Mock Implementations for Isolated Testing
// =============================================================================

/**
 * Complete mock of IInstanceRegistry for isolated component tests.
 * Unlike incomplete mocks, this one properly tracks listeners.
 */
class MockInstanceRegistry : public IInstanceRegistry
{
public:
    SourceId registerInstance(
        const juce::String& trackIdentifier,
        std::shared_ptr<IAudioBuffer> captureBuffer,
        const juce::String& name = "Track",
        int channelCount = 2,
        double sampleRate = 44100.0,
        std::shared_ptr<AnalysisEngine> analysisEngine = nullptr,
        const juce::String& instanceUUID = juce::String()) override
    {
        (void)trackIdentifier;
        (void)captureBuffer;
        (void)name;
        (void)channelCount;
        (void)sampleRate;
        (void)analysisEngine;
        (void)instanceUUID;
        return SourceId::generate();
    }

    void unregisterInstance(const SourceId& sourceId) override
    {
        sources_.erase(sourceId.id);
        buffers_.erase(sourceId.id);

        for (auto* listener : listeners_)
            listener->sourceRemoved(sourceId);
    }

    std::vector<SourceInfo> getAllSources() const override
    {
        std::vector<SourceInfo> result;
        for (const auto& [key, info] : sources_)
            result.push_back(info);
        return result;
    }

    std::optional<SourceInfo> getSource(const SourceId& sourceId) const override
    {
        auto it = sources_.find(sourceId.id);
        if (it != sources_.end())
            return it->second;
        return std::nullopt;
    }

    std::optional<SourceInfo> getSourceByName(const juce::String& name) const override
    {
        for (const auto& [key, info] : sources_)
        {
            if (info.name.equalsIgnoreCase(name))
                return info;
        }
        return std::nullopt;
    }

    std::optional<SourceInfo> getSourceByInstanceUUID(const juce::String& uuid) const override
    {
        if (uuid.isEmpty())
            return std::nullopt;
        for (const auto& [key, info] : sources_)
        {
            if (info.instanceUUID == uuid)
                return info;
        }
        return std::nullopt;
    }

    std::shared_ptr<IAudioBuffer> getCaptureBuffer(const SourceId& sourceId) const override
    {
        auto it = buffers_.find(sourceId.id);
        if (it != buffers_.end())
            return it->second;
        return nullptr;
    }

    void updateSource(const SourceId& sourceId, const juce::String& name, int numChannels, double sampleRate) override
    {
        auto it = sources_.find(sourceId.id);
        if (it != sources_.end())
        {
            it->second.name = name;
            it->second.channelCount = numChannels;
            it->second.sampleRate = sampleRate;

            for (auto* listener : listeners_)
                listener->sourceUpdated(sourceId);
        }
    }

    size_t getSourceCount() const override { return sources_.size(); }

    void addListener(InstanceRegistryListener* listener) override
    {
        listeners_.insert(listener);
    }

    void removeListener(InstanceRegistryListener* listener) override
    {
        listeners_.erase(listener);
    }

private:
    std::map<juce::String, SourceInfo> sources_;
    std::map<juce::String, std::shared_ptr<IAudioBuffer>> buffers_;
    std::set<InstanceRegistryListener*> listeners_;
};

/**
 * Complete mock of IThemeService for isolated component tests.
 * Properly initializes theme and tracks listeners.
 */
class MockThemeService : public IThemeService
{
public:
    MockThemeService()
    {
        // Initialize with a valid default theme
        currentTheme_.name = "Test Theme";
        currentTheme_.initializeDefaultWaveformColors();
        currentTheme_.backgroundPrimary = juce::Colour(0xFF1E1E1E);
        currentTheme_.backgroundSecondary = juce::Colour(0xFF252526);
        currentTheme_.backgroundPane = juce::Colour(0xFF2D2D2D);
        currentTheme_.textPrimary = juce::Colour(0xFFD4D4D4);
        currentTheme_.textSecondary = juce::Colour(0xFF808080);
        currentTheme_.controlBackground = juce::Colour(0xFF3C3C3C);
        currentTheme_.controlBorder = juce::Colour(0xFF474747);
        currentTheme_.controlHighlight = juce::Colour(0xFF094771);
        currentTheme_.controlActive = juce::Colour(0xFF0E639C);
    }

    const ColorTheme& getCurrentTheme() const override { return currentTheme_; }

    bool setCurrentTheme(const juce::String& themeName) override
    {
        currentTheme_.name = themeName;
        notifyListeners();
        return true;
    }

    std::vector<juce::String> getAvailableThemes() const override
    {
        return {"Test Theme", "Dark", "Light"};
    }

    const ColorTheme* getTheme(const juce::String& name) const override
    {
        if (name == currentTheme_.name)
            return &currentTheme_;
        return nullptr;
    }

    bool isSystemTheme(const juce::String&) const override { return false; }

    bool createTheme(const juce::String& name, const juce::String&) override
    {
        return !name.isEmpty();
    }

    bool updateTheme(const juce::String&, const ColorTheme&) override { return true; }
    bool deleteTheme(const juce::String&) override { return true; }
    bool cloneTheme(const juce::String&, const juce::String&) override { return true; }
    bool importTheme(const juce::String&) override { return true; }
    juce::String exportTheme(const juce::String&) const override { return "{}"; }

    void addListener(ThemeManagerListener* listener) override
    {
        listeners_.insert(listener);
    }

    void removeListener(ThemeManagerListener* listener) override
    {
        listeners_.erase(listener);
    }

    // Test helper: trigger theme change notifications
    void notifyListeners()
    {
        for (auto* listener : listeners_)
            listener->themeChanged(currentTheme_);
    }

private:
    ColorTheme currentTheme_;
    std::set<ThemeManagerListener*> listeners_;
};

// =============================================================================
// Test Fixtures
// =============================================================================

/**
 * Base fixture for tests that need the full JUCE plugin infrastructure.
 *
 * Owns service instances (InstanceRegistry, ThemeManager, ShaderRegistry) via
 * dependency injection. No singletons are used.
 *
 * Use this for:
 * - Tests involving OscilPluginEditor
 * - Integration tests that need full plugin behavior
 * - Any test requiring real services
 */
class OscilPluginTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create owned service instances (no singletons)
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();
        
        // Create centralized capture infrastructure (owned by this fixture)
        capturePool_ = std::make_unique<AudioCapturePool>();
        captureThread_ = std::make_unique<CaptureThread>(*capturePool_);
        captureThread_->startCapturing();

        // Create processor with owned services
        processor = std::make_unique<OscilPluginProcessor>(
            *registry_, *themeManager_, *shaderRegistry_, *memoryBudgetManager_,
            *capturePool_, *captureThread_);

        // Disable GPU rendering for headless test environment
        // OpenGL context operations crash without a real display/window
        processor->getState().setGpuRenderingEnabled(false);

        // Initialize audio processing (required for editor creation)
        processor->prepareToPlay(44100.0, 512);

        // Pump message queue to process any async registrations
        pumpMessageQueue(100);
    }

    void TearDown() override
    {
        // Destroy editor first (if exists)
        editor.reset();

        // Pump to process cleanup callbacks
        pumpMessageQueue(50);

        // Destroy processor (releases capture slot)
        processor.reset();

        // Stop capture thread before destroying pool
        if (captureThread_)
        {
            captureThread_->stopCapturing();
        }

        // Pump before destroying services
        pumpMessageQueue(50);

        // Destroy capture infrastructure in order
        captureThread_.reset();
        capturePool_.reset();

        // Destroy services in reverse order
        memoryBudgetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();

        // Final cleanup
        pumpMessageQueue(50);
    }

    /**
     * Creates the plugin editor. Call this when you need to test editor functionality.
     * Note: Editor creation is deferred because some tests only need processor.
     */
    void createEditor()
    {
        editor = std::make_unique<OscilPluginEditor>(*processor);
        // Pump to let editor initialize
        pumpMessageQueue(100);
    }

    // Accessors for test code that needs direct service access
    InstanceRegistry& getRegistry() { return *registry_; }
    ThemeManager& getThemeManager() { return *themeManager_; }
    ShaderRegistry& getShaderRegistry() { return *shaderRegistry_; }
    MemoryBudgetManager& getMemoryBudgetManager() { return *memoryBudgetManager_; }

    // Owned services
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;

    // Centralized capture infrastructure (replaces per-processor pool/thread)
    std::unique_ptr<AudioCapturePool> capturePool_;
    std::unique_ptr<CaptureThread> captureThread_;

    std::unique_ptr<OscilPluginProcessor> processor;
    std::unique_ptr<OscilPluginEditor> editor;
};

/**
 * Fixture for isolated UI component tests that don't need the full plugin.
 *
 * Uses complete mock implementations that properly track listeners and state.
 * Safe to use for components that don't bypass DI to access singletons.
 *
 * Use this for:
 * - Testing individual UI components in isolation
 * - Tests where you need to control/verify service interactions
 * - Tests that don't require OscilPluginEditor
 */
class OscilComponentTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockRegistry = std::make_unique<MockInstanceRegistry>();
        mockThemeService = std::make_unique<MockThemeService>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
    }

    void TearDown() override
    {
        // Pump to process any pending callbacks
        pumpMessageQueue(50);

        shaderRegistry_.reset();
        mockThemeService.reset();
        mockRegistry.reset();
    }

    /**
     * Creates a ServiceContext from mock services.
     * Returns by value since ServiceContext is a simple struct of references.
     */
    ServiceContext createServiceContext()
    {
        return ServiceContext{*mockRegistry, *mockThemeService, *shaderRegistry_};
    }

    std::unique_ptr<MockInstanceRegistry> mockRegistry;
    std::unique_ptr<MockThemeService> mockThemeService;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
};

/**
 * Fixture for state/model tests that don't need UI or services.
 *
 * Use this for:
 * - Testing OscilState, Oscillator, Pane, etc.
 * - Pure logic tests without UI dependencies
 */
class OscilStateTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // No special setup needed for state tests
    }

    void TearDown() override
    {
        // No special teardown
    }
};

/**
 * Fixture specifically for tests involving processor audio processing.
 *
 * Provides helper methods for audio buffer manipulation.
 */
class OscilAudioTestFixture : public OscilPluginTestFixture
{
protected:
    juce::AudioBuffer<float> createTestBuffer(int numChannels, int numSamples, float value = 0.5f)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample(ch, i, value);
        return buffer;
    }

    juce::AudioBuffer<float> createSineBuffer(int numChannels, int numSamples,
                                               float frequency = 440.0f, float sampleRate = 44100.0f)
    {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float sample = std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate);
                buffer.setSample(ch, i, sample);
            }
        }
        return buffer;
    }

    void processAudio(juce::AudioBuffer<float>& buffer)
    {
        juce::MidiBuffer midi;
        processor->processBlock(buffer, midi);
    }
};

} // namespace oscil::test
