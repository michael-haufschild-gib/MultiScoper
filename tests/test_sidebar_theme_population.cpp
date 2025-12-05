/*
    Oscil - Sidebar Theme Population Test
    Verifies that the SidebarComponent correctly populates the theme dropdown in OptionsSection
*/

#include <gtest/gtest.h>
#include "ui/layout/SidebarComponent.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/ServiceContext.h"
#include "ui/layout/sections/OptionsSection.h"
#include "ui/theme/ThemeManager.h"
#include "rendering/ShaderRegistry.h"

using namespace oscil;

class SidebarThemePopulationTest : public ::testing::Test
{
protected:
    // Owned service instances (no singletons)
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;

    std::unique_ptr<OscilPluginProcessor> processor;
    std::unique_ptr<SidebarComponent> sidebar;

    void SetUp() override
    {
        // Create owned service instances before other initialization
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();

        // ThemeManager usually has default themes initialized

        // Create processor with owned services
        processor = std::make_unique<OscilPluginProcessor>(*registry_, *themeManager_, *shaderRegistry_, *memoryBudgetManager_);

        // Initialize sidebar with ServiceContext using owned services
        ServiceContext context{*registry_, *themeManager_, *shaderRegistry_};
        sidebar = std::make_unique<SidebarComponent>(context);
    }

    void TearDown() override
    {
        // Reset in reverse order
        sidebar.reset();
        processor.reset();

        // Destroy services in reverse order
        memoryBudgetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
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
