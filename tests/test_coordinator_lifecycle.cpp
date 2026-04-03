/*
    Oscil - Coordinator Lifecycle Tests
    Tests for coordinator construction, destruction, and registration
*/

#include "ui/layout/LayoutCoordinator.h"
#include "ui/layout/SourceCoordinator.h"
#include "ui/theme/ThemeCoordinator.h"

#include "OscilTestFixtures.h"

#include <atomic>
#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

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
    EXPECT_FALSE(std::is_copy_constructible<LayoutCoordinator>::value);
    EXPECT_FALSE(std::is_copy_assignable<LayoutCoordinator>::value);
}