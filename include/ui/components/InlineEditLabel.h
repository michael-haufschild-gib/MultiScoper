/*
    Oscil - Inline Edit Label Component
    A label that becomes editable on double-click with save/cancel buttons
*/

#pragma once

#include "ui/components/ComponentConstants.h"
#include "ui/components/TestId.h"
#include "ui/components/ThemedComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace oscil
{

class OscilButton;
class IThemeService;

/**
 * A label component that supports inline editing.
 *
 * Features:
 * - Display mode: Shows text as a themed label with ellipsis overflow
 * - Edit mode: Double-click activates TextEditor with save/cancel buttons
 * - Keyboard shortcuts: Enter to save, Escape to cancel
 * - Auto-select: Selects all text when entering edit mode
 * - Validation: Optional callback to validate before saving
 * - Empty text handling: Reverts to original if empty string entered
 */
class InlineEditLabel
    : public ThemedComponent
    , public TestIdSupport
{
public:
    explicit InlineEditLabel(IThemeService& themeService);
    InlineEditLabel(IThemeService& themeService, const juce::String& testId);
    ~InlineEditLabel() override;

    /// Set the displayed text, optionally firing the onChange callback.
    void setText(const juce::String& text, bool notify = false);
    juce::String getText() const { return text_; }

    // Configuration
    void setPlaceholder(const juce::String& placeholder);
    juce::String getPlaceholder() const { return placeholder_; }

    void setMaxLength(int maxLength);
    int getMaxLength() const { return maxLength_; }

    void setValidator(std::function<bool(const juce::String&)> validator);

    void setReadOnly(bool readOnly);
    bool isReadOnly() const { return readOnly_; }

    /// Enter inline text editing mode (shows the text editor overlay).
    void enterEditMode();
    /// Exit inline editing, optionally committing the changed text.
    void exitEditMode(bool saveChanges);
    bool isInEditMode() const { return editMode_; }

    // Styling
    void setFont(const juce::Font& font);
    juce::Font getFont() const { return font_; }

    void setTextJustification(juce::Justification justification);
    juce::Justification getTextJustification() const { return justification_; }

    void setTextColour(juce::Colour colour);
    juce::Colour getTextColour() const { return textColour_; }

    // Callbacks
    std::function<void(const juce::String& newText)> onTextChanged;
    std::function<void()> onEditStarted;
    std::function<void()> onEditCancelled;
    std::function<void(const juce::MouseEvent&)> onMouseDown;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void focusLost(FocusChangeType cause) override;

    // Size hints
    int getPreferredHeight() const;

protected:
    void onThemeChanged(const ColorTheme& newTheme) override;

private:
    void setupComponents();
    void updateLayout();
    void saveChanges();
    void cancelChanges();
    void updateEditorStyle();

    // Dependencies

    // State
    juce::String text_;
    juce::String originalText_;
    juce::String placeholder_ = "Enter text...";
    bool editMode_ = false;
    bool readOnly_ = false;
    int maxLength_ = 256;

    // Styling
    juce::Font font_{juce::FontOptions(ComponentLayout::FONT_SIZE_DEFAULT)};
    juce::Justification justification_ = juce::Justification::centredLeft;
    juce::Colour textColour_;
    bool useCustomTextColour_ = false;

    // Validation
    std::function<bool(const juce::String&)> validator_;

    // Child components
    std::unique_ptr<juce::TextEditor> editor_;
    std::unique_ptr<OscilButton> saveButton_;
    std::unique_ptr<OscilButton> cancelButton_;

    // Layout constants
    static constexpr int BUTTON_SIZE = 20;
    static constexpr int BUTTON_SPACING = 2;
    static constexpr int EDITOR_MARGIN = 2;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InlineEditLabel)
};

} // namespace oscil
