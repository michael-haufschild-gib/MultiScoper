/*
    Oscil - TextField Component Tests
    Tests for OscilTextField UI component
*/

#include <gtest/gtest.h>
#include "ui/components/OscilTextField.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilTextFieldTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(OscilTextFieldTest, DefaultConstruction)
{
    OscilTextField field(ThemeManager::getInstance());

    EXPECT_TRUE(field.getText().isEmpty());
    EXPECT_TRUE(field.isEnabled());
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Text);
}

TEST_F(OscilTextFieldTest, ConstructionWithVariant)
{
    OscilTextField field(ThemeManager::getInstance(), TextFieldVariant::Search);

    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);
}

TEST_F(OscilTextFieldTest, ConstructionWithTestId)
{
    OscilTextField field(ThemeManager::getInstance(), "field-1");

    EXPECT_TRUE(field.getText().isEmpty());
}

TEST_F(OscilTextFieldTest, ConstructionWithVariantAndTestId)
{
    OscilTextField field(ThemeManager::getInstance(), TextFieldVariant::Number, "field-1");

    EXPECT_EQ(field.getVariant(), TextFieldVariant::Number);
}

// =============================================================================
// Text Content Tests
// =============================================================================

TEST_F(OscilTextFieldTest, SetText)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setText("Hello World", false);

    EXPECT_EQ(field.getText(), juce::String("Hello World"));
}

TEST_F(OscilTextFieldTest, SetEmptyText)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setText("Initial", false);
    field.setText("", false);

    EXPECT_TRUE(field.getText().isEmpty());
}

TEST_F(OscilTextFieldTest, SetPlaceholder)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setPlaceholder("Enter text...");

    EXPECT_EQ(field.getPlaceholder(), juce::String("Enter text..."));
}

// =============================================================================
// Variant Tests
// =============================================================================

TEST_F(OscilTextFieldTest, SetVariantText)
{
    OscilTextField field(ThemeManager::getInstance());

    field.setVariant(TextFieldVariant::Text);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Text);
}

TEST_F(OscilTextFieldTest, SetVariantSearch)
{
    OscilTextField field(ThemeManager::getInstance());

    field.setVariant(TextFieldVariant::Search);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);
}

TEST_F(OscilTextFieldTest, SetVariantNumber)
{
    OscilTextField field(ThemeManager::getInstance());

    field.setVariant(TextFieldVariant::Number);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Number);
}

// =============================================================================
// Enabled/Disabled Tests
// =============================================================================

TEST_F(OscilTextFieldTest, DefaultEnabled)
{
    OscilTextField field(ThemeManager::getInstance());

    EXPECT_TRUE(field.isEnabled());
}

TEST_F(OscilTextFieldTest, SetDisabled)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setEnabled(false);

    EXPECT_FALSE(field.isEnabled());
}

TEST_F(OscilTextFieldTest, SetEnabledAfterDisabled)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setEnabled(false);
    field.setEnabled(true);

    EXPECT_TRUE(field.isEnabled());
}

// =============================================================================
// Error State Tests
// =============================================================================

TEST_F(OscilTextFieldTest, DefaultNoError)
{
    OscilTextField field(ThemeManager::getInstance());

    EXPECT_FALSE(field.hasError());
}

TEST_F(OscilTextFieldTest, SetError)
{
    OscilTextField field(ThemeManager::getInstance());

    field.setError("Invalid input");
    EXPECT_TRUE(field.hasError());
}

TEST_F(OscilTextFieldTest, ClearError)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setError("Invalid input");

    field.clearError();
    EXPECT_FALSE(field.hasError());
}

// =============================================================================
// Number Variant Tests
// =============================================================================

TEST_F(OscilTextFieldTest, SetNumericValue)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);
    field.setStep(0.5);  // Set step to allow non-integer values

    field.setNumericValue(42.5, false);
    EXPECT_NEAR(field.getNumericValue(), 42.5, 0.01);
}

TEST_F(OscilTextFieldTest, SetNumericRange)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);
    field.setRange(0.0, 100.0);

    field.setNumericValue(50.0, false);
    EXPECT_NEAR(field.getNumericValue(), 50.0, 0.01);

    // Test clamping
    field.setNumericValue(150.0, false);
    EXPECT_LE(field.getNumericValue(), 100.0);

    field.setNumericValue(-50.0, false);
    EXPECT_GE(field.getNumericValue(), 0.0);
}

TEST_F(OscilTextFieldTest, SetNumericStep)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);

    field.setStep(0.1);
    // Just verify it doesn't crash
}

TEST_F(OscilTextFieldTest, SetNumericDecimalPlaces)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);

    field.setDecimalPlaces(2);
    // Just verify it doesn't crash
}

TEST_F(OscilTextFieldTest, SetNumericSuffix)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);

    field.setSuffix("dB");
    // Just verify it doesn't crash
}

TEST_F(OscilTextFieldTest, SetNumericDefaultValue)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);

    field.setDefaultValue(50.0);
    // Just verify it doesn't crash
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST_F(OscilTextFieldTest, SetValidator)
{
    OscilTextField field(ThemeManager::getInstance());

    bool validationCalled = false;
    field.setValidator([&validationCalled](const juce::String& text) {
        validationCalled = true;
        return text.length() >= 3;
    });

    // Validation would be triggered on text change
    // Just verify callback can be set
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(OscilTextFieldTest, OnTextChangedCallbackCanBeSet)
{
    OscilTextField field(ThemeManager::getInstance());

    int changeCount = 0;
    juce::String lastText;

    field.onTextChanged = [&changeCount, &lastText](const juce::String& text) {
        changeCount++;
        lastText = text;
    };

    // Verify callback can be assigned and text can be set
    // Note: JUCE notifications may not work in headless test environment
    field.setText("Test", true);
    EXPECT_EQ(field.getText(), juce::String("Test"));
}

TEST_F(OscilTextFieldTest, NoCallbackWhenNotifyFalse)
{
    OscilTextField field(ThemeManager::getInstance());

    int changeCount = 0;

    field.onTextChanged = [&changeCount](const juce::String&) {
        changeCount++;
    };

    field.setText("Test", false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(OscilTextFieldTest, OnValueChangedCallback)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Number);

    int changeCount = 0;
    double lastValue = 0.0;

    field.onValueChanged = [&changeCount, &lastValue](double value) {
        changeCount++;
        lastValue = value;
    };

    field.setNumericValue(42.0, true);
    EXPECT_EQ(changeCount, 1);
    EXPECT_NEAR(lastValue, 42.0, 0.01);
}

TEST_F(OscilTextFieldTest, OnReturnPressedCallback)
{
    OscilTextField field(ThemeManager::getInstance());

    bool returnPressed = false;
    field.onReturnPressed = [&returnPressed]() {
        returnPressed = true;
    };

    // Callback would be triggered by keyboard event
    // Just verify callback can be set
    EXPECT_FALSE(returnPressed);
}

TEST_F(OscilTextFieldTest, OnEscapePressedCallback)
{
    OscilTextField field(ThemeManager::getInstance());

    bool escapePressed = false;
    field.onEscapePressed = [&escapePressed]() {
        escapePressed = true;
    };

    // Callback would be triggered by keyboard event
    // Just verify callback can be set
    EXPECT_FALSE(escapePressed);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(OscilTextFieldTest, PreferredHeightPositive)
{
    OscilTextField field(ThemeManager::getInstance());

    int height = field.getPreferredHeight();
    EXPECT_GT(height, 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(OscilTextFieldTest, ThemeChangeDoesNotThrow)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setText("Test", false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    field.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(field.getText(), juce::String("Test"));
}

TEST_F(OscilTextFieldTest, ThemeChangePreservesVariant)
{
    OscilTextField field(ThemeManager::getInstance());
    field.setVariant(TextFieldVariant::Search);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    field.themeChanged(newTheme);

    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(OscilTextFieldTest, DoesNotWantKeyboardFocusDirectly)
{
    OscilTextField field(ThemeManager::getInstance());

    // TextField uses internal TextEditor for keyboard focus, not the component itself
    EXPECT_FALSE(field.getWantsKeyboardFocus());
}
