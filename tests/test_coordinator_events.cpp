/*
    Oscil - Coordinator Event Tests
    Tests for coordinator event callbacks and notifications
*/

#include <gtest/gtest.h>
#include "ui/theme/ThemeCoordinator.h"
#include "ui/layout/LayoutCoordinator.h"
#include "ui/theme/ThemeManager.h"
#include <atomic>

using namespace oscil;

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
// ThemeCoordinator Event Tests
// =============================================================================

TEST(ThemeCoordinatorEventTests, ThemeChangedCallsCallback)
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

TEST(ThemeCoordinatorEventTests, NullCallbackHandled)
{
    MockThemeService mockThemeService;

    ThemeCoordinator coordinator(mockThemeService, nullptr);

    ColorTheme newTheme;
    newTheme.name = "Test";

    mockThemeService.setTheme(newTheme);

    // Theme should still be applied even with null callback
    EXPECT_EQ(mockThemeService.getCurrentTheme().name, "Test");
}

// =============================================================================
// LayoutCoordinator Event Tests
// =============================================================================

TEST(LayoutCoordinatorEventTests, SidebarWidthChangedCallsCallback)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    layout.setSidebarWidth(400);

    EXPECT_EQ(callbackCount, 1);
}

TEST(LayoutCoordinatorEventTests, SidebarCollapseChangedCallsCallback)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    layout.setSidebarCollapsed(true);

    EXPECT_EQ(callbackCount, 1);
}

TEST(LayoutCoordinatorEventTests, WindowSizeChangedDoesNotCallCallback)
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

TEST(LayoutCoordinatorEventTests, NullCallbackHandled)
{
    WindowLayout layout;

    LayoutCoordinator coordinator(layout, nullptr);

    layout.setSidebarWidth(500);
    layout.setSidebarCollapsed(true);

    // Layout state should still be updated even with null callback
    EXPECT_EQ(layout.getSidebarWidth(), 500);
    EXPECT_TRUE(layout.isSidebarCollapsed());
}

TEST(LayoutCoordinatorEventTests, MultipleLayoutChanges)
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

TEST(LayoutCoordinatorEventTests, NoCallbackWhenValueUnchanged)
{
    WindowLayout layout;
    std::atomic<int> callbackCount{0};

    LayoutCoordinator coordinator(layout, [&callbackCount]() { ++callbackCount; });

    int currentWidth = layout.getSidebarWidth();
    layout.setSidebarWidth(currentWidth);  // Same value

    EXPECT_EQ(callbackCount, 0);
}
