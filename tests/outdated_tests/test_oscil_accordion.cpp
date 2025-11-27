/*
    Oscil - Accordion Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilAccordion.h"

using namespace oscil;

class OscilAccordionTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// ============ OscilAccordionSection Tests ============

// Test: Default construction
TEST_F(OscilAccordionTest, SectionDefaultConstruction)
{
    OscilAccordionSection section("Test Section");

    EXPECT_EQ(section.getTitle(), juce::String("Test Section"));
    EXPECT_FALSE(section.isExpanded());
}

// Test: Set expanded
TEST_F(OscilAccordionTest, SectionSetExpanded)
{
    OscilAccordionSection section("Test");

    section.setExpanded(true);
    EXPECT_TRUE(section.isExpanded());

    section.setExpanded(false);
    EXPECT_FALSE(section.isExpanded());
}

// Test: Toggle expanded
TEST_F(OscilAccordionTest, SectionToggleExpanded)
{
    OscilAccordionSection section("Test");

    EXPECT_FALSE(section.isExpanded());

    section.toggle();
    EXPECT_TRUE(section.isExpanded());

    section.toggle();
    EXPECT_FALSE(section.isExpanded());
}

// Test: Set content component
TEST_F(OscilAccordionTest, SectionSetContent)
{
    OscilAccordionSection section("Test");

    auto content = std::make_unique<juce::Component>();
    auto* contentPtr = content.get();

    section.setContent(std::move(content));
    EXPECT_EQ(section.getContent(), contentPtr);
}

// Test: Set title
TEST_F(OscilAccordionTest, SectionSetTitle)
{
    OscilAccordionSection section("Original");

    section.setTitle("Updated Title");
    EXPECT_EQ(section.getTitle(), juce::String("Updated Title"));
}

// Test: Callback on expand/collapse
TEST_F(OscilAccordionTest, SectionCallback)
{
    OscilAccordionSection section("Test");

    int changeCount = 0;
    bool lastState = false;

    section.onExpandedChanged = [&changeCount, &lastState](bool expanded) {
        changeCount++;
        lastState = expanded;
    };

    section.setExpanded(true, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_TRUE(lastState);

    section.setExpanded(false, true);
    EXPECT_EQ(changeCount, 2);
    EXPECT_FALSE(lastState);
}

// ============ OscilAccordion Tests ============

// Test: Default construction
TEST_F(OscilAccordionTest, AccordionDefaultConstruction)
{
    OscilAccordion accordion;

    EXPECT_EQ(accordion.getNumSections(), 0);
}

// Test: Add sections
TEST_F(OscilAccordionTest, AddSections)
{
    OscilAccordion accordion;

    accordion.addSection("Section A");
    accordion.addSection("Section B");
    accordion.addSection("Section C");

    EXPECT_EQ(accordion.getNumSections(), 3);
}

// Test: Get section by index
TEST_F(OscilAccordionTest, GetSectionByIndex)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    auto* section = accordion.getSection(1);
    ASSERT_NE(section, nullptr);
    EXPECT_EQ(section->getTitle(), juce::String("Section B"));
}

// Test: Remove section
TEST_F(OscilAccordionTest, RemoveSection)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");
    accordion.addSection("Section B");
    accordion.addSection("Section C");

    accordion.removeSection(1);
    EXPECT_EQ(accordion.getNumSections(), 2);
}

// Test: Clear sections
TEST_F(OscilAccordionTest, ClearSections)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.clearSections();
    EXPECT_EQ(accordion.getNumSections(), 0);
}

// Test: Allow multiple expand
TEST_F(OscilAccordionTest, AllowMultipleExpand)
{
    OscilAccordion accordion;

    accordion.setAllowMultipleExpanded(true);
    EXPECT_TRUE(accordion.getAllowMultipleExpanded());

    accordion.setAllowMultipleExpanded(false);
    EXPECT_FALSE(accordion.getAllowMultipleExpanded());
}

// Test: Single expand mode (mutual exclusivity)
TEST_F(OscilAccordionTest, SingleExpandMode)
{
    OscilAccordion accordion;
    accordion.setAllowMultipleExpanded(false);

    accordion.addSection("Section A");
    accordion.addSection("Section B");
    accordion.addSection("Section C");

    accordion.expandSection(0);
    EXPECT_TRUE(accordion.getSection(0)->isExpanded());

    accordion.expandSection(2);
    EXPECT_FALSE(accordion.getSection(0)->isExpanded());
    EXPECT_TRUE(accordion.getSection(2)->isExpanded());
}

// Test: Multiple expand mode
TEST_F(OscilAccordionTest, MultipleExpandMode)
{
    OscilAccordion accordion;
    accordion.setAllowMultipleExpanded(true);

    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.expandSection(0);
    accordion.expandSection(1);

    EXPECT_TRUE(accordion.getSection(0)->isExpanded());
    EXPECT_TRUE(accordion.getSection(1)->isExpanded());
}

// Test: Collapse all
TEST_F(OscilAccordionTest, CollapseAll)
{
    OscilAccordion accordion;
    accordion.setAllowMultipleExpanded(true);

    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.expandSection(0);
    accordion.expandSection(1);

    accordion.collapseAll();

    EXPECT_FALSE(accordion.getSection(0)->isExpanded());
    EXPECT_FALSE(accordion.getSection(1)->isExpanded());
}

// Test: Expand all
TEST_F(OscilAccordionTest, ExpandAll)
{
    OscilAccordion accordion;
    accordion.setAllowMultipleExpanded(true);

    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.expandAll();

    EXPECT_TRUE(accordion.getSection(0)->isExpanded());
    EXPECT_TRUE(accordion.getSection(1)->isExpanded());
}

// Test: Callback on section change
TEST_F(OscilAccordionTest, CallbackOnSectionChange)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    int changeCount = 0;
    int lastIndex = -1;

    accordion.onSectionChanged = [&changeCount, &lastIndex](int index, bool) {
        changeCount++;
        lastIndex = index;
    };

    accordion.expandSection(1);
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastIndex, 1);
}

// Test: Spacing between sections
TEST_F(OscilAccordionTest, SectionSpacing)
{
    OscilAccordion accordion;

    accordion.setSpacing(12);
    EXPECT_EQ(accordion.getSpacing(), 12);
}

// Test: Preferred size
TEST_F(OscilAccordionTest, PreferredSize)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    int width = accordion.getPreferredWidth();
    int height = accordion.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Theme changes
TEST_F(OscilAccordionTest, ThemeChanges)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");
    accordion.expandSection(0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    accordion.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(accordion.getSection(0)->isExpanded());
}

// Test: Accessibility
TEST_F(OscilAccordionTest, Accessibility)
{
    OscilAccordion accordion;
    accordion.addSection("Section A");

    auto* handler = accordion.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);
}
