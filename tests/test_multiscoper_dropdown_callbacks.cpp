/*
    MultiScoper - Dropdown Component Callback Regression Tests
    Regression coverage for issues surfaced by the 2026-04-21 dropdown audit.
*/

#include "ui/components/MultiScoperDropdown.h"

#include "MultiScoperTestFixtures.h"

using namespace multiscoper;
using namespace multiscoper::test;

class MultiScoperDropdownCallbacksTest : public MultiScoperComponentTestFixture
{
protected:
    void SetUp() override { MultiScoperComponentTestFixture::SetUp(); }
};

// setSelectedIndices in multi-select mode must NOT fire onSelectionChangedId,
// because getSelectedId() returns only the first ID and would mislead
// listeners that assume it reflects the whole selection.
TEST_F(MultiScoperDropdownCallbacksTest, MultiSelectDoesNotFireOnSelectionChangedId)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.setMultiSelect(true);
    dropdown.addItem("A", "a");
    dropdown.addItem("B", "b");
    dropdown.addItem("C", "c");

    int idCallCount = 0;
    juce::String lastId;
    dropdown.onSelectionChangedId = [&](const juce::String& id) {
        ++idCallCount;
        lastId = id;
    };

    int multiCallCount = 0;
    dropdown.onMultiSelectionChanged = [&](const std::set<int>&) { ++multiCallCount; };

    dropdown.setSelectedIndices({0, 2}, true);

    EXPECT_EQ(multiCallCount, 1);
    EXPECT_EQ(idCallCount, 0);
    EXPECT_TRUE(lastId.isEmpty());
}

// Single-select path still fires onSelectionChangedId.
TEST_F(MultiScoperDropdownCallbacksTest, SingleSelectFiresOnSelectionChangedId)
{
    MultiScoperDropdown dropdown(*mockThemeService);
    dropdown.addItem("A", "a");
    dropdown.addItem("B", "b");

    int idCallCount = 0;
    juce::String lastId;
    dropdown.onSelectionChangedId = [&](const juce::String& id) {
        ++idCallCount;
        lastId = id;
    };

    dropdown.setSelectedIndices({1}, true);

    EXPECT_EQ(idCallCount, 1);
    EXPECT_EQ(lastId, juce::String("b"));
}
