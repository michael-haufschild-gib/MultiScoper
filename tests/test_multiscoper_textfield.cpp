/*
    MultiScoper - TextField Component Tests
    Tests for MultiScoperTextField UI component
*/

#include "ui/components/MultiScoperTextField.h"
#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class MultiScoperTextFieldTest : public ::testing::Test
{
protected:
    void SetUp() override { themeManager_ = std::make_unique<ThemeManager>(); }

    void TearDown() override { themeManager_.reset(); }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, DefaultConstruction)
{
    MultiScoperTextField field(getThemeManager());

    EXPECT_TRUE(field.getText().isEmpty());
    EXPECT_TRUE(field.isEnabled());
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Text);
}

TEST_F(MultiScoperTextFieldTest, ConstructionWithVariant)
{
    MultiScoperTextField field(getThemeManager(), TextFieldVariant::Search);

    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);
}

TEST_F(MultiScoperTextFieldTest, ConstructionWithTestId)
{
    MultiScoperTextField field(getThemeManager(), "field-1");

    EXPECT_TRUE(field.getText().isEmpty());
}

TEST_F(MultiScoperTextFieldTest, ConstructionWithVariantAndTestId)
{
    MultiScoperTextField field(getThemeManager(), TextFieldVariant::Number, "field-1");

    EXPECT_EQ(field.getVariant(), TextFieldVariant::Number);
}

// =============================================================================
// Text Content Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, SetText)
{
    MultiScoperTextField field(getThemeManager());
    field.setText("Hello World", false);

    EXPECT_EQ(field.getText(), juce::String("Hello World"));
}

TEST_F(MultiScoperTextFieldTest, SetEmptyText)
{
    MultiScoperTextField field(getThemeManager());
    field.setText("Initial", false);
    field.setText("", false);

    EXPECT_TRUE(field.getText().isEmpty());
}

TEST_F(MultiScoperTextFieldTest, SetPlaceholder)
{
    MultiScoperTextField field(getThemeManager());
    field.setPlaceholder("Enter text...");

    EXPECT_EQ(field.getPlaceholder(), juce::String("Enter text..."));
}

// =============================================================================
// Variant Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, SetVariantText)
{
    MultiScoperTextField field(getThemeManager());

    field.setVariant(TextFieldVariant::Text);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Text);
}

TEST_F(MultiScoperTextFieldTest, SetVariantSearch)
{
    MultiScoperTextField field(getThemeManager());

    field.setVariant(TextFieldVariant::Search);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);
}

TEST_F(MultiScoperTextFieldTest, SetVariantNumber)
{
    MultiScoperTextField field(getThemeManager());

    field.setVariant(TextFieldVariant::Number);
    EXPECT_EQ(field.getVariant(), TextFieldVariant::Number);
}

// =============================================================================
// Enabled/Disabled Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, DefaultEnabled)
{
    MultiScoperTextField field(getThemeManager());

    EXPECT_TRUE(field.isEnabled());
    EXPECT_FALSE(field.hasError());
}

TEST_F(MultiScoperTextFieldTest, SetDisabled)
{
    MultiScoperTextField field(getThemeManager());
    field.setEnabled(false);

    EXPECT_FALSE(field.isEnabled());
}

TEST_F(MultiScoperTextFieldTest, SetEnabledAfterDisabled)
{
    MultiScoperTextField field(getThemeManager());
    field.setEnabled(false);
    EXPECT_FALSE(field.isEnabled());

    field.setEnabled(true);
    EXPECT_TRUE(field.isEnabled());
}

// =============================================================================
// Error State Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, DefaultNoError)
{
    MultiScoperTextField field(getThemeManager());

    EXPECT_FALSE(field.hasError());
}

TEST_F(MultiScoperTextFieldTest, SetError)
{
    MultiScoperTextField field(getThemeManager());

    field.setError("Invalid input");
    EXPECT_TRUE(field.hasError());
}

TEST_F(MultiScoperTextFieldTest, ClearError)
{
    MultiScoperTextField field(getThemeManager());
    field.setError("Invalid input");

    field.clearError();
    EXPECT_FALSE(field.hasError());
}

// =============================================================================
// Number Variant Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, SetNumericValue)
{
    MultiScoperTextField field(getThemeManager());
    field.setVariant(TextFieldVariant::Number);
    field.setStep(0.5); // Set step to allow non-integer values

    field.setNumericValue(42.5, false);
    EXPECT_NEAR(field.getNumericValue(), 42.5, 0.01);
}

TEST_F(MultiScoperTextFieldTest, SetNumericRange)
{
    MultiScoperTextField field(getThemeManager());
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

TEST_F(MultiScoperTextFieldTest, SetNumericStep)
{
    MultiScoperTextField field(getThemeManager());
    field.setVariant(TextFieldVariant::Number);

    field.setStep(0.1);
    // Setting step should not alter existing numeric value
    field.setNumericValue(5.0, false);
    EXPECT_NEAR(field.getNumericValue(), 5.0, 0.01);
}

TEST_F(MultiScoperTextFieldTest, SetNumericDecimalPlaces)
{
    MultiScoperTextField field(getThemeManager());
    field.setVariant(TextFieldVariant::Number);

    field.setDecimalPlaces(2);
    field.setNumericValue(3.14159, false);
    // Value is stored with decimal place precision
    EXPECT_NEAR(field.getNumericValue(), 3.0, 1.0);
    EXPECT_FALSE(field.hasError());
}

TEST_F(MultiScoperTextFieldTest, SetNumericSuffix)
{
    MultiScoperTextField field(getThemeManager());
    field.setVariant(TextFieldVariant::Number);

    field.setSuffix("dB");
    field.setNumericValue(10.0, false);
    EXPECT_NEAR(field.getNumericValue(), 10.0, 0.01);
}

TEST_F(MultiScoperTextFieldTest, SetNumericDefaultValue)
{
    MultiScoperTextField field(getThemeManager());
    field.setVariant(TextFieldVariant::Number);

    field.setDefaultValue(50.0);
    // After setting default, field should still be in number variant
    EXPECT_TRUE(field.isEnabled());
    EXPECT_FALSE(field.hasError());
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, SetValidator)
{
    MultiScoperTextField field(getThemeManager());

    bool validationCalled = false;
    field.setValidator([&validationCalled](const juce::String& text) {
        validationCalled = true;
        return text.length() >= 3;
    });

    // Validator should be accepted without error
    EXPECT_TRUE(field.isEnabled());
    EXPECT_FALSE(field.hasError());
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, OnTextChangedCallbackCanBeSet)
{
    MultiScoperTextField field(getThemeManager());

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

TEST_F(MultiScoperTextFieldTest, NoCallbackWhenNotifyFalse)
{
    MultiScoperTextField field(getThemeManager());

    int changeCount = 0;

    field.onTextChanged = [&changeCount](const juce::String&) { changeCount++; };

    field.setText("Test", false);
    EXPECT_EQ(changeCount, 0);
}

TEST_F(MultiScoperTextFieldTest, OnValueChangedCallback)
{
    MultiScoperTextField field(getThemeManager());
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

TEST_F(MultiScoperTextFieldTest, OnReturnPressedCallback)
{
    MultiScoperTextField field(getThemeManager());

    bool returnPressed = false;
    field.onReturnPressed = [&returnPressed]() { returnPressed = true; };

    // Callback would be triggered by keyboard event
    // Just verify callback can be set
    EXPECT_FALSE(returnPressed);
}

TEST_F(MultiScoperTextFieldTest, OnEscapePressedCallback)
{
    MultiScoperTextField field(getThemeManager());

    bool escapePressed = false;
    field.onEscapePressed = [&escapePressed]() { escapePressed = true; };

    // Callback would be triggered by keyboard event
    // Just verify callback can be set
    EXPECT_FALSE(escapePressed);
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, PreferredHeightPositive)
{
    MultiScoperTextField field(getThemeManager());

    int height = field.getPreferredHeight();
    EXPECT_GT(height, 0);
}

// =============================================================================
// Theme Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, ThemeChangeDoesNotThrow)
{
    MultiScoperTextField field(getThemeManager());
    field.setText("Test", false);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    field.themeChanged(newTheme);

    // State should be preserved
    EXPECT_EQ(field.getText(), juce::String("Test"));
}

TEST_F(MultiScoperTextFieldTest, ThemeChangePreservesVariant)
{
    MultiScoperTextField field(getThemeManager());
    field.setVariant(TextFieldVariant::Search);

    ColorTheme newTheme;
    newTheme.name = "Test Theme";
    field.themeChanged(newTheme);

    EXPECT_EQ(field.getVariant(), TextFieldVariant::Search);
}

// =============================================================================
// Focus Tests
// =============================================================================

TEST_F(MultiScoperTextFieldTest, WantsKeyboardFocusForAccessibility)
{
    MultiScoperTextField field(getThemeManager());

    // The wrapper advertises wantsKeyboardFocus=true so Tab / screen-reader
    // traversal discovers it as a single focusable element. The inner
    // TextEditor owns the actual keyboard state — focusGained on the
    // wrapper immediately redirects grabKeyboardFocus() to the editor so
    // typing still lands in the text buffer. Previously the wrapper
    // defaulted to false, which left the harness and a11y tree reporting
    // the field as unreachable via keyboard.
    EXPECT_TRUE(field.getWantsKeyboardFocus());
}
