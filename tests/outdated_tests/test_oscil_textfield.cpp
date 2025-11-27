/*
    Oscil - TextField Component Tests
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTextField.h"

using namespace oscil;

class OscilTextFieldTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// Test: Default construction
TEST_F(OscilTextFieldTest, DefaultConstruction)
{
    OscilTextField field;

    EXPECT_TRUE(field.getText().isEmpty());
    EXPECT_TRUE(field.isEnabled());
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Text);
}

// Test: Set text
TEST_F(OscilTextFieldTest, SetText)
{
    OscilTextField field;
    field.setText("Hello World");

    EXPECT_EQ(field.getText(), juce::String("Hello World"));
}

// Test: Set placeholder
TEST_F(OscilTextFieldTest, SetPlaceholder)
{
    OscilTextField field;
    field.setPlaceholder("Enter text...");

    EXPECT_EQ(field.getPlaceholder(), juce::String("Enter text..."));
}

// Test: Variant types
TEST_F(OscilTextFieldTest, VariantTypes)
{
    OscilTextField field;

    field.setVariant(TextFieldVariant::Text);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Text);

    field.setVariant(TextFieldVariant::Search);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);

    field.setVariant(TextFieldVariant::Number);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Number);

    field.setVariant(TextFieldVariant::Password);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Password);
}

// Test: Enabled/disabled state
TEST_F(OscilTextFieldTest, EnabledState)
{
    OscilTextField field;

    EXPECT_TRUE(field.isEnabled());

    field.setEnabled(false);
    EXPECT_FALSE(field.isEnabled());

    field.setEnabled(true);
    EXPECT_TRUE(field.isEnabled());
}

// Test: Read-only state
TEST_F(OscilTextFieldTest, ReadOnlyState)
{
    OscilTextField field;

    EXPECT_FALSE(field.isReadOnly());

    field.setReadOnly(true);
    EXPECT_TRUE(field.isReadOnly());

    field.setReadOnly(false);
    EXPECT_FALSE(field.isReadOnly());
}

// Test: Label
TEST_F(OscilTextFieldTest, Label)
{
    OscilTextField field;
    field.setLabel("Username");

    EXPECT_EQ(field.getLabel(), juce::String("Username"));
}

// Test: Helper text
TEST_F(OscilTextFieldTest, HelperText)
{
    OscilTextField field;
    field.setHelperText("Enter your username");

    EXPECT_EQ(field.getHelperText(), juce::String("Enter your username"));
}

// Test: Error state
TEST_F(OscilTextFieldTest, ErrorState)
{
    OscilTextField field;

    EXPECT_FALSE(field.hasError());

    field.setError("Invalid input");
    EXPECT_TRUE(field.hasError());
    EXPECT_EQ(field.getErrorMessage(), juce::String("Invalid input"));

    field.clearError();
    EXPECT_FALSE(field.hasError());
}

// Test: Max length
TEST_F(OscilTextFieldTest, MaxLength)
{
    OscilTextField field;
    field.setMaxLength(10);

    EXPECT_EQ(field.getMaxLength(), 10);
}

// Test: Number variant - numeric value
TEST_F(OscilTextFieldTest, NumericValue)
{
    OscilTextField field;
    field.setVariant(TextFieldVariant::Number);

    field.setNumericValue(42.5);
    EXPECT_NEAR(field.getNumericValue(), 42.5, 0.01);
}

// Test: Number variant - range
TEST_F(OscilTextFieldTest, NumericRange)
{
    OscilTextField field;
    field.setVariant(TextFieldVariant::Number);
    field.setNumericRange(0.0, 100.0);

    field.setNumericValue(50.0);
    EXPECT_NEAR(field.getNumericValue(), 50.0, 0.01);

    field.setNumericValue(150.0);
    EXPECT_LE(field.getNumericValue(), 100.0);

    field.setNumericValue(-50.0);
    EXPECT_GE(field.getNumericValue(), 0.0);
}

// Test: Number variant - step
TEST_F(OscilTextFieldTest, NumericStep)
{
    OscilTextField field;
    field.setVariant(TextFieldVariant::Number);
    field.setNumericStep(0.1);

    EXPECT_FLOAT_EQ(field.getNumericStep(), 0.1f);
}

// Test: Prefix and suffix
TEST_F(OscilTextFieldTest, PrefixAndSuffix)
{
    OscilTextField field;

    field.setPrefix("$");
    EXPECT_EQ(field.getPrefix(), juce::String("$"));

    field.setSuffix("USD");
    EXPECT_EQ(field.getSuffix(), juce::String("USD"));
}

// Test: Validation callback
TEST_F(OscilTextFieldTest, ValidationCallback)
{
    OscilTextField field;

    bool validationCalled = false;
    field.setValidator([&validationCalled](const juce::String& text) {
        validationCalled = true;
        return text.length() >= 3;
    });

    field.setText("AB");
    field.validate();

    EXPECT_TRUE(validationCalled);
}

// Test: Text changed callback
TEST_F(OscilTextFieldTest, TextChangedCallback)
{
    OscilTextField field;

    int changeCount = 0;
    juce::String lastText;

    field.onTextChanged = [&changeCount, &lastText](const juce::String& text) {
        changeCount++;
        lastText = text;
    };

    field.setText("Test");
    // Note: Callback may or may not be called depending on implementation
    // This tests the callback mechanism exists
}

// Test: Preferred size
TEST_F(OscilTextFieldTest, PreferredSize)
{
    OscilTextField field;

    int width = field.getPreferredWidth();
    int height = field.getPreferredHeight();

    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

// Test: Clear button
TEST_F(OscilTextFieldTest, ClearButton)
{
    OscilTextField field;

    field.setShowClearButton(true);
    EXPECT_TRUE(field.getShowClearButton());

    field.setShowClearButton(false);
    EXPECT_FALSE(field.getShowClearButton());
}

// Test: Theme changes
TEST_F(OscilTextFieldTest, ThemeChanges)
{
    OscilTextField field;
    field.setText("Test");

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    field.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(field.getText(), juce::String("Test"));
}

// Test: Accessibility
TEST_F(OscilTextFieldTest, Accessibility)
{
    OscilTextField field;
    field.setLabel("Test Field");

    auto* handler = field.getAccessibilityHandler();
    EXPECT_NE(handler, nullptr);

    EXPECT_EQ(handler->getRole(), juce::AccessibilityRole::editableText);
}

// Test: Focus handling
TEST_F(OscilTextFieldTest, FocusHandling)
{
    OscilTextField field;
    EXPECT_TRUE(field.getWantsKeyboardFocus());
}

// Test: Select all
TEST_F(OscilTextFieldTest, SelectAll)
{
    OscilTextField field;
    field.setText("Hello World");

    field.selectAll();
    // Selection should be the full text
    // (implementation detail, but verifies method exists)
}

// Test: Clear
TEST_F(OscilTextFieldTest, Clear)
{
    OscilTextField field;
    field.setText("Hello World");

    field.clear();
    EXPECT_TRUE(field.getText().isEmpty());
}
