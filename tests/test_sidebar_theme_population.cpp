/*
    Oscil - Sidebar Theme Population Tests
    Verifies that SidebarComponent correctly populates the theme dropdown in
    OptionsSection with the names supplied by ThemeManager.
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
#include "rendering/PresetManager.h"

using namespace oscil;

class SidebarThemePopulationTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<PresetManager> presetManager_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<OscilPluginProcessor> processor;
    std::unique_ptr<SidebarComponent> sidebar;

    void SetUp() override
    {
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        presetManager_ = std::make_unique<PresetManager>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();

        processor = std::make_unique<OscilPluginProcessor>(
            *registry_, *themeManager_, *shaderRegistry_, *presetManager_, *memoryBudgetManager_);

        ServiceContext context{*registry_, *themeManager_, *shaderRegistry_, *presetManager_};
        sidebar = std::make_unique<SidebarComponent>(context);
    }

    void TearDown() override
    {
        sidebar.reset();
        processor.reset();
        memoryBudgetManager_.reset();
        presetManager_.reset();
        shaderRegistry_.reset();
        themeManager_.reset();
        registry_.reset();
    }
};

// Bug caught: theme dropdown empty because SidebarComponent failed to query
// ThemeManager during initialization, leaving user with no selectable themes.
TEST_F(SidebarThemePopulationTest, DropdownItemCountMatchesAvailableThemes)
{
    auto* optionsSection = sidebar->getOptionsSection();
    ASSERT_NE(optionsSection, nullptr);

    auto* themeDropdown = optionsSection->getThemeDropdown();
    ASSERT_NE(themeDropdown, nullptr);

    auto availableThemes = themeManager_->getAvailableThemes();
    ASSERT_FALSE(availableThemes.empty()) << "ThemeManager should have at least one default theme";

    EXPECT_EQ(themeDropdown->getNumItems(), static_cast<int>(availableThemes.size()))
        << "Dropdown item count must match ThemeManager::getAvailableThemes()";
}

// Bug caught: dropdown populated with empty strings or placeholder IDs instead
// of real theme names, making it impossible to identify themes.
TEST_F(SidebarThemePopulationTest, EveryAvailableThemeNameIsNonEmpty)
{
    auto availableThemes = themeManager_->getAvailableThemes();

    for (size_t i = 0; i < availableThemes.size(); ++i)
    {
        EXPECT_FALSE(availableThemes[i].isEmpty())
            << "Theme at index " << i << " has an empty name";
    }
}

// Bug caught: the initial selection in the dropdown doesn't match the active
// theme in ThemeManager, causing a visual desync on editor open.
TEST_F(SidebarThemePopulationTest, DropdownSelectionMatchesCurrentTheme)
{
    auto* optionsSection = sidebar->getOptionsSection();
    ASSERT_NE(optionsSection, nullptr);

    auto* themeDropdown = optionsSection->getThemeDropdown();
    ASSERT_NE(themeDropdown, nullptr);

    juce::String currentThemeName = themeManager_->getCurrentTheme().name;
    EXPECT_FALSE(currentThemeName.isEmpty());

    juce::String selectedId = themeDropdown->getSelectedId();
    EXPECT_EQ(selectedId, currentThemeName)
        << "Dropdown selection '" << selectedId.toStdString()
        << "' does not match active theme '" << currentThemeName.toStdString() << "'";
}
