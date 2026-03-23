/*
    Oscil - Accordion Component Tests
    Tests for OscilAccordion and OscilAccordionSection UI components
*/

#include "ui/components/OscilAccordion.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace oscil;

class OscilAccordionTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }
    IThemeService& getThemeService() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// OscilAccordionSection Tests
// =============================================================================

TEST_F(OscilAccordionTest, SectionDefaultConstruction)
{
    OscilAccordionSection section(getThemeService(), "Test Section");

    EXPECT_EQ(section.getTitle(), juce::String("Test Section"));
    EXPECT_FALSE(section.isExpanded());
    EXPECT_TRUE(section.isEnabled());
}

TEST_F(OscilAccordionTest, SectionSetExpanded)
{
    OscilAccordionSection section(getThemeService(), "Test");

    section.setExpanded(true, false);
    EXPECT_TRUE(section.isExpanded());

    section.setExpanded(false, false);
    EXPECT_FALSE(section.isExpanded());
}

TEST_F(OscilAccordionTest, SectionToggleExpanded)
{
    OscilAccordionSection section(getThemeService(), "Test");

    EXPECT_FALSE(section.isExpanded());

    section.toggle();
    EXPECT_TRUE(section.isExpanded());

    section.toggle();
    EXPECT_FALSE(section.isExpanded());
}

TEST_F(OscilAccordionTest, SectionSetContent)
{
    OscilAccordionSection section(getThemeService(), "Test");

    juce::Component content;
    section.setContent(&content);
    EXPECT_EQ(section.getContent(), &content);
}

TEST_F(OscilAccordionTest, SectionClearContent)
{
    OscilAccordionSection section(getThemeService(), "Test");

    juce::Component content;
    section.setContent(&content);
    section.setContent(nullptr);
    EXPECT_EQ(section.getContent(), nullptr);
}

TEST_F(OscilAccordionTest, SectionSetTitle)
{
    OscilAccordionSection section(getThemeService(), "Original");

    section.setTitle("Updated Title");
    EXPECT_EQ(section.getTitle(), juce::String("Updated Title"));
}

TEST_F(OscilAccordionTest, SectionEnabled)
{
    OscilAccordionSection section(getThemeService(), "Test");

    EXPECT_TRUE(section.isEnabled());

    section.setEnabled(false);
    EXPECT_FALSE(section.isEnabled());

    section.setEnabled(true);
    EXPECT_TRUE(section.isEnabled());
}

TEST_F(OscilAccordionTest, SectionCallback)
{
    OscilAccordionSection section(getThemeService(), "Test");

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

TEST_F(OscilAccordionTest, SectionExpandedCallbackCalledWithAnimation)
{
    OscilAccordionSection section(getThemeService(), "Test");

    int changeCount = 0;

    section.onExpandedChanged = [&changeCount](bool) { changeCount++; };

    // Second parameter is `animate`, not `notify` - callback always fires
    section.setExpanded(true, false); // no animation
    EXPECT_EQ(changeCount, 1);        // Callback is still called
}

TEST_F(OscilAccordionTest, SectionHeaderHeight)
{
    OscilAccordionSection section(getThemeService(), "Test");

    EXPECT_GT(section.getHeaderHeight(), 0);
}

TEST_F(OscilAccordionTest, SectionPreferredHeight)
{
    OscilAccordionSection section(getThemeService(), "Test");

    EXPECT_GT(section.getPreferredHeight(), 0);
}

// =============================================================================
// OscilAccordion Tests
// =============================================================================

TEST_F(OscilAccordionTest, AccordionDefaultConstruction)
{
    OscilAccordion accordion(getThemeService());

    EXPECT_EQ(accordion.getNumSections(), 0);
    EXPECT_FALSE(accordion.getAllowMultiExpand());
}

TEST_F(OscilAccordionTest, AddSections)
{
    OscilAccordion accordion(getThemeService());

    accordion.addSection("Section A");
    accordion.addSection("Section B");
    accordion.addSection("Section C");

    EXPECT_EQ(accordion.getNumSections(), 3);
}

TEST_F(OscilAccordionTest, AddSectionWithContent)
{
    OscilAccordion accordion(getThemeService());

    juce::Component content;
    auto* section = accordion.addSection("Section A", &content);

    ASSERT_NE(section, nullptr);
    EXPECT_EQ(section->getContent(), &content);
}

TEST_F(OscilAccordionTest, GetSectionByIndex)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    auto* section = accordion.getSection(1);
    ASSERT_NE(section, nullptr);
    EXPECT_EQ(section->getTitle(), juce::String("Section B"));
}

TEST_F(OscilAccordionTest, GetSectionOutOfBounds)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");

    auto* section = accordion.getSection(999);
    EXPECT_EQ(section, nullptr);
}

TEST_F(OscilAccordionTest, RemoveSection)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");
    accordion.addSection("Section B");
    accordion.addSection("Section C");

    accordion.removeSection(1);
    EXPECT_EQ(accordion.getNumSections(), 2);
}

TEST_F(OscilAccordionTest, ClearSections)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.clearSections();
    EXPECT_EQ(accordion.getNumSections(), 0);
}

TEST_F(OscilAccordionTest, AllowMultiExpand)
{
    OscilAccordion accordion(getThemeService());

    accordion.setAllowMultiExpand(true);
    EXPECT_TRUE(accordion.getAllowMultiExpand());

    accordion.setAllowMultiExpand(false);
    EXPECT_FALSE(accordion.getAllowMultiExpand());
}

TEST_F(OscilAccordionTest, SingleExpandMode)
{
    OscilAccordion accordion(getThemeService());
    accordion.setAllowMultiExpand(false);

    accordion.addSection("Section A");
    accordion.addSection("Section B");
    accordion.addSection("Section C");

    accordion.expandSection(0);
    EXPECT_TRUE(accordion.getSection(0)->isExpanded());

    accordion.expandSection(2);
    EXPECT_FALSE(accordion.getSection(0)->isExpanded());
    EXPECT_TRUE(accordion.getSection(2)->isExpanded());
}

TEST_F(OscilAccordionTest, MultipleExpandMode)
{
    OscilAccordion accordion(getThemeService());
    accordion.setAllowMultiExpand(true);

    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.expandSection(0);
    accordion.expandSection(1);

    EXPECT_TRUE(accordion.getSection(0)->isExpanded());
    EXPECT_TRUE(accordion.getSection(1)->isExpanded());
}

TEST_F(OscilAccordionTest, CollapseSection)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");

    accordion.expandSection(0);
    EXPECT_TRUE(accordion.getSection(0)->isExpanded());

    accordion.collapseSection(0);
    EXPECT_FALSE(accordion.getSection(0)->isExpanded());
}

TEST_F(OscilAccordionTest, CollapseAll)
{
    OscilAccordion accordion(getThemeService());
    accordion.setAllowMultiExpand(true);

    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.expandSection(0);
    accordion.expandSection(1);

    accordion.collapseAll();

    EXPECT_FALSE(accordion.getSection(0)->isExpanded());
    EXPECT_FALSE(accordion.getSection(1)->isExpanded());
}

TEST_F(OscilAccordionTest, ExpandAll)
{
    OscilAccordion accordion(getThemeService());
    accordion.setAllowMultiExpand(true);

    accordion.addSection("Section A");
    accordion.addSection("Section B");

    accordion.expandAll();

    EXPECT_TRUE(accordion.getSection(0)->isExpanded());
    EXPECT_TRUE(accordion.getSection(1)->isExpanded());
}

TEST_F(OscilAccordionTest, SectionSpacing)
{
    OscilAccordion accordion(getThemeService());

    accordion.setSpacing(12);
    EXPECT_EQ(accordion.getSpacing(), 12);
}

TEST_F(OscilAccordionTest, PreferredHeight)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");
    accordion.addSection("Section B");

    int height = accordion.getPreferredHeight();
    EXPECT_GT(height, 0);
}

TEST_F(OscilAccordionTest, ThemeChanges)
{
    OscilAccordion accordion(getThemeService());
    accordion.addSection("Section A");
    accordion.expandSection(0);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    accordion.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(accordion.getSection(0)->isExpanded());
}

TEST_F(OscilAccordionTest, SectionThemeChanges)
{
    OscilAccordionSection section(getThemeService(), "Test");
    section.setExpanded(true, false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    section.themeChanged(newTheme);

    // State should be preserved
    EXPECT_TRUE(section.isExpanded());
}

TEST_F(OscilAccordionTest, SectionWantsKeyboardFocus)
{
    OscilAccordionSection section(getThemeService(), "Test");

    // Accordion sections should be focusable for keyboard navigation
    EXPECT_TRUE(section.getWantsKeyboardFocus());
}
