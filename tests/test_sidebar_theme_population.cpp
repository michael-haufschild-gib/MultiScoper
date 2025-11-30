/*
    Oscil - Sidebar Theme Population Test
    Verifies that the SidebarComponent correctly populates the theme dropdown in OptionsSection
*/

#include <gtest/gtest.h>
#include "ui/SidebarComponent.h"
#include "core/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "ui/sections/OptionsSection.h"
#include "ui/ThemeManager.h"

using namespace oscil;

class SidebarThemePopulationTest : public ::testing::Test
{
protected:
    std::unique_ptr<OscilPluginProcessor> processor;
    std::unique_ptr<SidebarComponent> sidebar;

    void SetUp() override
    {
        // Ensure some themes exist
        auto& themeManager = ThemeManager::getInstance();
        // (ThemeManager usually has default themes initialized)
        
        processor = std::make_unique<OscilPluginProcessor>(InstanceRegistry::getInstance(), themeManager);
        // Initialize sidebar with processor
        sidebar = std::make_unique<SidebarComponent>(*processor);
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
