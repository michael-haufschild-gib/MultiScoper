/*
    Oscil - Oscillator List Item Theme Test
    Verifies that list items update their colors when the theme changes
*/

#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/panels/OscillatorListItem.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/Oscillator.h"
#include "ui/theme/ThemeManager.h"
#include "ui/components/InlineEditLabel.h"

using namespace oscil;

// Dummy Registry
class MockRegistry : public IInstanceRegistry
{
public:
    SourceId registerInstance(const juce::String&, std::shared_ptr<IAudioBuffer>, const juce::String&, int, double) override { return SourceId::generate(); }
    void unregisterInstance(const SourceId&) override {}
    std::vector<SourceInfo> getAllSources() const override { return {}; }
    std::optional<SourceInfo> getSource(const SourceId&) const override { return std::nullopt; }
    std::shared_ptr<IAudioBuffer> getCaptureBuffer(const SourceId&) const override { return nullptr; }
    void updateSource(const SourceId&, const juce::String&, int, double) override {}
    size_t getSourceCount() const override { return 0; }
    void addListener(InstanceRegistryListener*) override {}
    void removeListener(InstanceRegistryListener*) override {}
};

class OscillatorListItemThemeTest : public ::testing::Test
{
protected:
    // No SetUp needed
};

TEST_F(OscillatorListItemThemeTest, UpdatesColorsOnThemeChange)
{
    // 1. Create mock registry and item
    MockRegistry registry;
    Oscillator osc;
    OscillatorListItemComponent item(osc, registry);

    // 2. Find the name label
    InlineEditLabel* nameLabel = nullptr;
    for (int i = 0; i < item.getNumChildComponents(); ++i)
    {
        if (auto* label = dynamic_cast<InlineEditLabel*>(item.getChildComponent(i)))
        {
            nameLabel = label;
            break;
        }
    }
    ASSERT_NE(nameLabel, nullptr);

    // 3. Create a dark theme
    ColorTheme darkTheme;
    darkTheme.name = "Dark Test";
    darkTheme.textPrimary = juce::Colours::white;
    darkTheme.backgroundSecondary = juce::Colours::black;
    
    ThemeManager::getInstance().createTheme("Dark Test", "");
    ThemeManager::getInstance().updateTheme("Dark Test", darkTheme);
    ThemeManager::getInstance().setCurrentTheme("Dark Test");

    // Verify label color is white
    // Note: Component::findColour might default if not set, but themeChanged should call setTextColour
    // juce::Label stores text colour in textColourId
    EXPECT_EQ(nameLabel->findColour(juce::Label::textColourId), juce::Colours::white);

    // 4. Create a light theme
    ColorTheme lightTheme;
    lightTheme.name = "Light Test";
    lightTheme.textPrimary = juce::Colours::black;
    lightTheme.backgroundSecondary = juce::Colours::white;

    ThemeManager::getInstance().createTheme("Light Test", "");
    ThemeManager::getInstance().updateTheme("Light Test", lightTheme);
    
    // 5. Switch to light theme
    ThemeManager::getInstance().setCurrentTheme("Light Test");

    // 6. Verify label color changed to black
    EXPECT_EQ(nameLabel->findColour(juce::Label::textColourId), juce::Colours::black);
    
    // Cleanup
    ThemeManager::getInstance().deleteTheme("Dark Test");
    ThemeManager::getInstance().deleteTheme("Light Test");
}
