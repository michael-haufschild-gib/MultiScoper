/*
    Oscil - Sidebar Theme Population Test
    Verifies that the SidebarComponent correctly populates the theme dropdown in OptionsSection
*/

#include <gtest/gtest.h>
#include "ui/layout/SidebarComponent.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/ServiceContext.h"
#include "ui/layout/sections/OptionsSection.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class SidebarThemePopulationTest : public ::testing::Test
{
protected:
    std::unique_ptr<OscilPluginProcessor> processor;
    std::unique_ptr<SidebarComponent> sidebar;

    // Helper to access registry (friend access doesn't inherit to TEST_F generated classes)
    static InstanceRegistry& getRegistry() { return InstanceRegistry::getInstance(); }
    static ThemeManager& getThemeManager() { return ThemeManager::getInstance(); }

    void SetUp() override
    {
        // Ensure some themes exist
        auto& themeManager = getThemeManager();
        // (ThemeManager usually has default themes initialized)

        processor = std::make_unique<OscilPluginProcessor>(getRegistry(), themeManager);
        // Initialize sidebar with ServiceContext
        ServiceContext context{getRegistry(), themeManager};
        sidebar = std::make_unique<SidebarComponent>(context);
    }

    void TearDown() override
    {
        sidebar.reset();
        processor.reset();
    }
};

TEST_F(SidebarThemePopulationTest, ThemeDropdownIsPopulatedOnInitialization)
{
    // Get the OptionsSection from sidebar
    auto* optionsSection = sidebar->getOptionsSection();
    ASSERT_NE(optionsSection, nullptr) << "OptionsSection should be created by SidebarComponent";

    // Get the theme dropdown
    auto* themeDropdown = optionsSection->getThemeDropdown();
    ASSERT_NE(themeDropdown, nullptr) << "ThemeDropdown should exist in OptionsSection";

    // Verify it has items
    int itemCount = themeDropdown->getNumItems();
    
    // We expect at least 1 theme (the default one)
    EXPECT_GT(itemCount, 0) << "Theme dropdown should be populated with available themes";
    
    // Debug output
    if (itemCount == 0)
    {
        std::cout << "Test Failed: Theme dropdown is empty." << std::endl;
    }
    else
    {
        std::cout << "Theme dropdown has " << itemCount << " items." << std::endl;
    }
}
