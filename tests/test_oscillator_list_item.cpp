/*
    MultiScoper - Oscillator List Item Tests
    Tests for the item-level OscillatorListItemComponent (construction, label binding).
*/

#include "core/Oscillator.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "ui/components/InlineEditLabel.h"
#include "ui/panels/OscillatorListItem.h"
#include "ui/theme/ThemeManager.h"

#include "MultiScoperTestFixtures.h"
#include "MultiScoperTestUtils.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <gtest/gtest.h>

using namespace multiscoper;
using namespace multiscoper::test;

namespace
{

// Find the first child of the given type in parent. Returns nullptr if none.
template <typename T>
T* findChildOfType(juce::Component& parent)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        if (auto* typed = dynamic_cast<T*>(parent.getChildComponent(i)))
            return typed;
    }
    return nullptr;
}

} // anonymous namespace

class OscillatorListItemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
        mockRegistry_ = std::make_unique<MockInstanceRegistry>();
    }

    void TearDown() override
    {
        pumpMessageQueue(50);
        mockRegistry_.reset();
        themeManager_.reset();
        pumpMessageQueue(50);
    }

    IThemeService& getThemeService() { return *themeManager_; }
    IInstanceRegistry& getRegistry() { return *mockRegistry_; }

    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<MockInstanceRegistry> mockRegistry_;
};

TEST_F(OscillatorListItemTest, LabelUpdatesFromOscillator)
{
    Oscillator osc;
    osc.setName("Initial Name");

    OscillatorListItemComponent item(osc, getRegistry(), getThemeService());

    auto* nameLabel = findChildOfType<InlineEditLabel>(item);
    ASSERT_NE(nameLabel, nullptr);
    EXPECT_EQ(nameLabel->getText(), "Initial Name");

    osc.setName("Updated Name");
    item.updateFromOscillator(osc);

    EXPECT_EQ(nameLabel->getText(), "Updated Name");
}

TEST_F(OscillatorListItemTest, NameLabelIsVisibleOnConstruction)
{
    Oscillator osc;
    osc.setName("Visible Test");

    OscillatorListItemComponent item(osc, getRegistry(), getThemeService());

    auto* nameLabel = findChildOfType<InlineEditLabel>(item);
    ASSERT_NE(nameLabel, nullptr);
    EXPECT_TRUE(nameLabel->isVisible());
}

TEST_F(OscillatorListItemTest, SetListIndexRegistersDisplayPositionTestIds)
{
    Oscillator osc;
    osc.setOrderIndex(42); // Simulate an arbitrary underlying order index.

    OscillatorListItemComponent item(osc, getRegistry(), getThemeService());

    // Before setListIndex, the item has no testId (it is assigned by the list).
    EXPECT_FALSE(item.hasTestId());

    item.setListIndex(3);
    EXPECT_EQ(item.getTestId(), "sidebar_oscillators_item_3");

    // Reassigning to a new display position re-registers under the new id.
    item.setListIndex(7);
    EXPECT_EQ(item.getTestId(), "sidebar_oscillators_item_7");
}
