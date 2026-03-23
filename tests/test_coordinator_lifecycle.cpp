/*
    Oscil - Coordinator Lifecycle Tests
    Tests for coordinator construction, destruction, and registration
*/

#include "core/InstanceRegistry.h"
#include "ui/layout/LayoutCoordinator.h"
#include "ui/layout/SourceCoordinator.h"
#include "ui/theme/ThemeCoordinator.h"
#include "ui/theme/ThemeManager.h"

#include <atomic>
#include <gtest/gtest.h>

using namespace oscil;

// =============================================================================
// Mock IInstanceRegistry for Testing
// =============================================================================

class MockInstanceRegistry : public IInstanceRegistry
{
public:
    void addListener(InstanceRegistryListener* listener) override { listeners_.add(listener); }

    void removeListener(InstanceRegistryListener* listener) override { listeners_.remove(listener); }

    std::vector<SourceInfo> getAllSources() const override { return sources_; }

    std::shared_ptr<IAudioBuffer> getCaptureBuffer(const SourceId& sourceId) const override
    {
        for (const auto& source : sources_)
        {
            if (source.sourceId == sourceId)
                return source.buffer.lock();
        }
        return nullptr;
    }

    // Additional required interface methods
    SourceId registerInstance(const juce::String& /*trackIdentifier*/, std::shared_ptr<IAudioBuffer> /*captureBuffer*/,
                              const juce::String& /*name*/, int /*channelCount*/, double /*sampleRate*/,
                              std::shared_ptr<AnalysisEngine> /*analysisEngine*/) override
    {
        return SourceId::generate();
    }

    void unregisterInstance(const SourceId& /*sourceId*/) override {}

    std::optional<SourceInfo> getSource(const SourceId& sourceId) const override
    {
        for (const auto& source : sources_)
        {
            if (source.sourceId == sourceId)
                return source;
        }
        return std::nullopt;
    }

    void updateSource(const SourceId& /*sourceId*/, const juce::String& /*name*/, int /*channelCount*/,
                      double /*sampleRate*/) override
    {
    }

    size_t getSourceCount() const override { return sources_.size(); }

    // Test helpers
    void addSource(const SourceId& id, const juce::String& sourceName)
    {
        SourceInfo info;
        info.sourceId = id;
        info.name = sourceName;
        sources_.push_back(info);
        notifySourceAdded(id);
    }

    void removeSource(const SourceId& id)
    {
        sources_.erase(
            std::remove_if(sources_.begin(), sources_.end(), [&id](const SourceInfo& s) { return s.sourceId == id; }),
            sources_.end());
        notifySourceRemoved(id);
    }

    void updateSourceNotify(const SourceId& id) { notifySourceUpdated(id); }

    void clear() { sources_.clear(); }

    int getListenerCount() const { return listeners_.size(); }

private:
    void notifySourceAdded(const SourceId& id)
    {
        listeners_.call([&id](InstanceRegistryListener& l) { l.sourceAdded(id); });
    }

    void notifySourceRemoved(const SourceId& id)
    {
        listeners_.call([&id](InstanceRegistryListener& l) { l.sourceRemoved(id); });
    }

    void notifySourceUpdated(const SourceId& id)
    {
        listeners_.call([&id](InstanceRegistryListener& l) { l.sourceUpdated(id); });
    }

    std::vector<SourceInfo> sources_;
    juce::ListenerList<InstanceRegistryListener> listeners_;
};

// =============================================================================
// Mock IThemeService for Testing
// =============================================================================

class MockThemeService : public IThemeService
{
public:
    void addListener(ThemeManagerListener* listener) override { listeners_.add(listener); }

    void removeListener(ThemeManagerListener* listener) override { listeners_.remove(listener); }

    const ColorTheme& getCurrentTheme() const override { return currentTheme_; }

    bool setCurrentTheme(const juce::String& /*themeName*/) override { return true; }

    std::vector<juce::String> getAvailableThemes() const override { return {"default"}; }

    const ColorTheme* getTheme(const juce::String& /*themeName*/) const override { return &currentTheme_; }

    bool isSystemTheme(const juce::String& /*name*/) const override { return true; }

    bool createTheme(const juce::String& /*name*/, const juce::String& /*sourceTheme*/) override { return true; }

    bool updateTheme(const juce::String& /*name*/, const ColorTheme& /*theme*/) override { return true; }

    bool deleteTheme(const juce::String& /*name*/) override { return true; }

    bool cloneTheme(const juce::String& /*sourceName*/, const juce::String& /*newName*/) override { return true; }

    bool importTheme(const juce::String& /*json*/) override { return true; }
    juce::String exportTheme(const juce::String& /*name*/) const override { return "{}"; }

    void setTheme(const ColorTheme& theme)
    {
        currentTheme_ = theme;
        notifyThemeChanged();
    }

    int getListenerCount() const { return listeners_.size(); }

    // Test helper to directly notify
    void notifyThemeChanged()
    {
        listeners_.call([this](ThemeManagerListener& l) { l.themeChanged(currentTheme_); });
    }

private:
    ColorTheme currentTheme_;
    juce::ListenerList<ThemeManagerListener> listeners_;
};

// =============================================================================
// SourceCoordinator Lifecycle Tests
// =============================================================================

TEST(SourceCoordinatorLifecycleTests, RegistrationOnConstruction)
{
    MockInstanceRegistry mockRegistry;
    std::atomic<int> callbackCount{0};

    EXPECT_EQ(mockRegistry.getListenerCount(), 0);

    {
        SourceCoordinator coordinator(mockRegistry, [&callbackCount]() { ++callbackCount; });
        EXPECT_EQ(mockRegistry.getListenerCount(), 1);
    }

    // After destruction
    EXPECT_EQ(mockRegistry.getListenerCount(), 0);
}

TEST(SourceCoordinatorLifecycleTests, NoCopyConstruction)
{
    static_assert(!std::is_copy_constructible<SourceCoordinator>::value,
                  "SourceCoordinator should not be copy constructible");
    static_assert(!std::is_copy_assignable<SourceCoordinator>::value,
                  "SourceCoordinator should not be copy assignable");
    // Also verify move semantics are disabled
    EXPECT_FALSE(std::is_copy_constructible<SourceCoordinator>::value);
    EXPECT_FALSE(std::is_copy_assignable<SourceCoordinator>::value);
}

// =============================================================================
// ThemeCoordinator Lifecycle Tests
// =============================================================================

TEST(ThemeCoordinatorLifecycleTests, RegistrationOnConstruction)
{
    MockThemeService mockThemeService;
    std::atomic<int> callbackCount{0};
    ColorTheme lastReceivedTheme;

    EXPECT_EQ(mockThemeService.getListenerCount(), 0);

    {
        ThemeCoordinator coordinator(mockThemeService, [&callbackCount, &lastReceivedTheme](const ColorTheme& theme) {
            ++callbackCount;
            lastReceivedTheme = theme;
        });
        EXPECT_EQ(mockThemeService.getListenerCount(), 1);
    }

    // After destruction
    EXPECT_EQ(mockThemeService.getListenerCount(), 0);
}

TEST(ThemeCoordinatorLifecycleTests, NoCopyConstruction)
{
    static_assert(!std::is_copy_constructible<ThemeCoordinator>::value,
                  "ThemeCoordinator should not be copy constructible");
    static_assert(!std::is_copy_assignable<ThemeCoordinator>::value, "ThemeCoordinator should not be copy assignable");
    EXPECT_FALSE(std::is_copy_constructible<ThemeCoordinator>::value);
    EXPECT_FALSE(std::is_copy_assignable<ThemeCoordinator>::value);
}

// =============================================================================
// LayoutCoordinator Lifecycle Tests
// =============================================================================

TEST(LayoutCoordinatorLifecycleTests, RegistrationAndCallback)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    {
        LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

        // Trigger a layout change
        layout.setSidebarWidth(layout.getSidebarWidth() + 50);

        EXPECT_EQ(callbackCount, 1);
    }
}

TEST(LayoutCoordinatorLifecycleTests, NoCopyConstruction)
{
    static_assert(!std::is_copy_constructible<LayoutCoordinator>::value,
                  "LayoutCoordinator should not be copy constructible");
    static_assert(!std::is_copy_assignable<LayoutCoordinator>::value,
                  "LayoutCoordinator should not be copy assignable");
    EXPECT_FALSE(std::is_copy_constructible<LayoutCoordinator>::value);
    EXPECT_FALSE(std::is_copy_assignable<LayoutCoordinator>::value);
}