/*
    Oscil - Coordinator Tests
    Tests for SourceCoordinator, ThemeCoordinator, and LayoutCoordinator
*/

#include <gtest/gtest.h>
#include "ui/layout/SourceCoordinator.h"
#include "ui/theme/ThemeCoordinator.h"
#include "ui/layout/LayoutCoordinator.h"
#include "core/InstanceRegistry.h"
#include "ui/theme/ThemeManager.h"
#include <atomic>
#include <chrono>
#include <thread>

using namespace oscil;

// =============================================================================
// Mock IInstanceRegistry for Testing
// =============================================================================

class MockInstanceRegistry : public IInstanceRegistry
{
public:
    void addListener(InstanceRegistryListener* listener) override
    {
        listeners_.add(listener);
    }

    void removeListener(InstanceRegistryListener* listener) override
    {
        listeners_.remove(listener);
    }

    std::vector<SourceInfo> getAllSources() const override
    {
        return sources_;
    }

    std::shared_ptr<SharedCaptureBuffer> getCaptureBuffer(const SourceId& sourceId) const override
    {
        for (const auto& source : sources_)
        {
            if (source.sourceId == sourceId)
                return source.buffer.lock();
        }
        return nullptr;
    }

    // Additional required interface methods
    SourceId registerInstance(const juce::String& /*trackIdentifier*/,
                               std::shared_ptr<SharedCaptureBuffer> /*captureBuffer*/,
                               const juce::String& /*name*/,
                               int /*channelCount*/,
                               double /*sampleRate*/) override
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

    void updateSource(const SourceId& /*sourceId*/, const juce::String& /*name*/,
                      int /*channelCount*/, double /*sampleRate*/) override {}

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
            std::remove_if(sources_.begin(), sources_.end(),
                [&id](const SourceInfo& s) { return s.sourceId == id; }),
            sources_.end());
        notifySourceRemoved(id);
    }

    void updateSourceNotify(const SourceId& id)
    {
        notifySourceUpdated(id);
    }

    void clear()
    {
        sources_.clear();
    }

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
    void addListener(ThemeManagerListener* listener) override
    {
        listeners_.add(listener);
    }

    void removeListener(ThemeManagerListener* listener) override
    {
        listeners_.remove(listener);
    }

    const ColorTheme& getCurrentTheme() const override
    {
        return currentTheme_;
    }

    bool setCurrentTheme(const juce::String& /*themeName*/) override
    {
        return true;
    }

    std::vector<juce::String> getAvailableThemes() const override
    {
        return {"default"};
    }

    const ColorTheme* getTheme(const juce::String& /*themeName*/) const override
    {
        return &currentTheme_;
    }

    bool isSystemTheme(const juce::String& /*name*/) const override
    {
        return true;
    }

    bool createTheme(const juce::String& /*name*/, const juce::String& /*sourceTheme*/) override
    {
        return true;
    }

    bool updateTheme(const juce::String& /*name*/, const ColorTheme& /*theme*/) override
    {
        return true;
    }

    bool deleteTheme(const juce::String& /*name*/) override
    {
        return true;
    }

    bool cloneTheme(const juce::String& /*sourceName*/, const juce::String& /*newName*/) override
    {
        return true;
    }

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
// SourceCoordinator Tests
// =============================================================================

TEST(SourceCoordinatorTests, RegistrationOnConstruction)
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

TEST(SourceCoordinatorTests, RefreshFromRegistryOnConstruction)
{
    MockInstanceRegistry mockRegistry;
    std::atomic<int> callbackCount{0};

    SourceId id1 = SourceId::generate();
    SourceId id2 = SourceId::generate();
    mockRegistry.addSource(id1, "Track 1");
    mockRegistry.addSource(id2, "Track 2");

    // Reset
    mockRegistry.clear();
    mockRegistry.addSource(id1, "Track 1");
    mockRegistry.addSource(id2, "Track 2");

    SourceCoordinator coordinator(mockRegistry, [&callbackCount]() { ++callbackCount; });

    EXPECT_EQ(coordinator.getAvailableSources().size(), 2);
}

TEST(SourceCoordinatorTests, GetAvailableSourcesReturnsCorrectList)
{
    MockInstanceRegistry mockRegistry;

    SourceId id1 = SourceId::generate();
    SourceId id2 = SourceId::generate();
    mockRegistry.addSource(id1, "Track 1");
    mockRegistry.addSource(id2, "Track 2");
    // Clear listeners added by addSource
    mockRegistry.clear();
    mockRegistry.addSource(id1, "Track 1");
    mockRegistry.addSource(id2, "Track 2");

    SourceCoordinator coordinator(mockRegistry, []() {});

    auto sources = coordinator.getAvailableSources();
    EXPECT_EQ(sources.size(), 2);

    // Verify both IDs are in the list
    bool foundId1 = std::find(sources.begin(), sources.end(), id1) != sources.end();
    bool foundId2 = std::find(sources.begin(), sources.end(), id2) != sources.end();
    EXPECT_TRUE(foundId1);
    EXPECT_TRUE(foundId2);
}

TEST(SourceCoordinatorTests, RefreshFromRegistry)
{
    MockInstanceRegistry mockRegistry;

    SourceCoordinator coordinator(mockRegistry, []() {});

    EXPECT_EQ(coordinator.getAvailableSources().size(), 0);

    // Add sources directly to registry (bypassing notification)
    SourceId id1 = SourceId::generate();
    SourceInfo info;
    info.sourceId = id1;
    info.name = "Track 1";
    // We need to add to internal list - use addSource which will trigger callback
    // Instead just refresh - but the mock won't have data
    // This tests the refresh mechanism
    coordinator.refreshFromRegistry();

    // Still 0 because mock's sources_ is empty (we didn't add via the mock's addSource)
    EXPECT_EQ(coordinator.getAvailableSources().size(), 0);
}

TEST(SourceCoordinatorTests, NoCopyConstruction)
{
    // Verify SourceCoordinator is not copyable (compile-time check)
    static_assert(!std::is_copy_constructible<SourceCoordinator>::value,
                  "SourceCoordinator should not be copy constructible");
    static_assert(!std::is_copy_assignable<SourceCoordinator>::value,
                  "SourceCoordinator should not be copy assignable");
}

// =============================================================================
// ThemeCoordinator Tests
// =============================================================================

TEST(ThemeCoordinatorTests, RegistrationOnConstruction)
{
    MockThemeService mockThemeService;
    std::atomic<int> callbackCount{0};
    ColorTheme lastReceivedTheme;

    EXPECT_EQ(mockThemeService.getListenerCount(), 0);

    {
        ThemeCoordinator coordinator(mockThemeService,
            [&callbackCount, &lastReceivedTheme](const ColorTheme& theme) {
                ++callbackCount;
                lastReceivedTheme = theme;
            });
        EXPECT_EQ(mockThemeService.getListenerCount(), 1);
    }

    // After destruction
    EXPECT_EQ(mockThemeService.getListenerCount(), 0);
}

TEST(ThemeCoordinatorTests, ThemeChangedCallsCallback)
{
    MockThemeService mockThemeService;
    std::atomic<int> callbackCount{0};
    ColorTheme lastReceivedTheme;

    ThemeCoordinator coordinator(mockThemeService,
        [&callbackCount, &lastReceivedTheme](const ColorTheme& theme) {
            ++callbackCount;
            lastReceivedTheme = theme;
        });

    ColorTheme newTheme;
    newTheme.name = "Custom Theme";
    newTheme.textPrimary = juce::Colours::red;

    mockThemeService.setTheme(newTheme);

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(lastReceivedTheme.name, "Custom Theme");
    EXPECT_EQ(lastReceivedTheme.textPrimary, juce::Colours::red);
}

TEST(ThemeCoordinatorTests, GetCurrentTheme)
{
    MockThemeService mockThemeService;
    ColorTheme initialTheme;
    initialTheme.name = "Initial";
    mockThemeService.setTheme(initialTheme);

    ThemeCoordinator coordinator(mockThemeService, [](const ColorTheme&) {});

    const auto& theme = coordinator.getCurrentTheme();
    EXPECT_EQ(theme.name, "Initial");
}

TEST(ThemeCoordinatorTests, NullCallbackHandled)
{
    MockThemeService mockThemeService;

    // Coordinator should handle null callback gracefully
    ThemeCoordinator coordinator(mockThemeService, nullptr);

    ColorTheme newTheme;
    newTheme.name = "Test";

    // Should not crash
    mockThemeService.setTheme(newTheme);
}

TEST(ThemeCoordinatorTests, NoCopyConstruction)
{
    static_assert(!std::is_copy_constructible<ThemeCoordinator>::value,
                  "ThemeCoordinator should not be copy constructible");
    static_assert(!std::is_copy_assignable<ThemeCoordinator>::value,
                  "ThemeCoordinator should not be copy assignable");
}

// =============================================================================
// LayoutCoordinator Tests
// =============================================================================

TEST(LayoutCoordinatorTests, RegistrationAndCallback)
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

TEST(LayoutCoordinatorTests, SidebarWidthChangedCallsCallback)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    layout.setSidebarWidth(400);

    EXPECT_EQ(callbackCount, 1);
}

TEST(LayoutCoordinatorTests, SidebarCollapseChangedCallsCallback)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    layout.setSidebarCollapsed(true);

    EXPECT_EQ(callbackCount, 1);
}

TEST(LayoutCoordinatorTests, WindowSizeChangedDoesNotCallCallback)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    // According to implementation, windowSizeChanged doesn't call the callback
    // (it's managed by JUCE)
    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    layout.setWindowWidth(1500);

    // Window size changes don't trigger the layout changed callback
    // (per the implementation comment: "Window size is managed by JUCE")
    EXPECT_EQ(callbackCount, 0);
}

TEST(LayoutCoordinatorTests, GetLayout)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    // Modify through coordinator's layout reference
    coordinator.getLayout().setSidebarWidth(450);

    EXPECT_EQ(layout.getSidebarWidth(), 450);
    EXPECT_EQ(coordinator.getLayout().getSidebarWidth(), 450);
}

TEST(LayoutCoordinatorTests, GetLayoutConst)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });
    const LayoutCoordinator& constCoord = coordinator;

    EXPECT_EQ(constCoord.getLayout().getSidebarWidth(), layout.getSidebarWidth());
}

TEST(LayoutCoordinatorTests, NullCallbackHandled)
{
    WindowLayout layout;

    // Should not crash with null callback
    LayoutCoordinator coordinator(layout, nullptr);

    layout.setSidebarWidth(500);
    layout.setSidebarCollapsed(true);

    // No crash = success
}

TEST(LayoutCoordinatorTests, NoCopyConstruction)
{
    static_assert(!std::is_copy_constructible<LayoutCoordinator>::value,
                  "LayoutCoordinator should not be copy constructible");
    static_assert(!std::is_copy_assignable<LayoutCoordinator>::value,
                  "LayoutCoordinator should not be copy assignable");
}

TEST(LayoutCoordinatorTests, MultipleLayoutChanges)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    layout.setSidebarWidth(350);
    layout.setSidebarWidth(400);
    layout.setSidebarCollapsed(true);
    layout.setSidebarCollapsed(false);

    EXPECT_EQ(callbackCount, 4);
}

TEST(LayoutCoordinatorTests, NoCallbackWhenValueUnchanged)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    int currentWidth = layout.getSidebarWidth();
    layout.setSidebarWidth(currentWidth);  // Same value

    EXPECT_EQ(callbackCount, 0);
}
