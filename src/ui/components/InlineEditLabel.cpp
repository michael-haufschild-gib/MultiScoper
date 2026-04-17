/*
    Oscil - Inline Edit Label Component Implementation
*/

#include "ui/components/InlineEditLabel.h"

#include "ui/components/ListItemIcons.h"
#include "ui/components/OscilButton.h"
#include "ui/components/SurfacePainter.h"

namespace oscil
{

InlineEditLabel::InlineEditLabel(IThemeService& themeService) : ThemedComponent(themeService)
{
    setupComponents();
    updateEditorStyle();
}

InlineEditLabel::InlineEditLabel(IThemeService& themeService, const juce::String& testId)
    : InlineEditLabel(themeService)
{
    setTestId(testId);
}

InlineEditLabel::~InlineEditLabel() {}

void InlineEditLabel::setupComponents()
{
    // Create text editor (hidden initially)
    editor_ = std::make_unique<juce::TextEditor>();
    editor_->setMultiLine(false);
    editor_->setReturnKeyStartsNewLine(false);
    editor_->setScrollbarsShown(false);
    editor_->setCaretVisible(true);
    editor_->setPopupMenuEnabled(false);

    editor_->onReturnKey = [this]() { saveChanges(); };

    editor_->onEscapeKey = [this]() { cancelChanges(); };

    editor_->onFocusLost = [this]() {
        // Only cancel if we're still in edit mode and mouse isn't over buttons
        if (editMode_)
        {
            // Check if focus went to our buttons
            if (saveButton_ && saveButton_->hasKeyboardFocus(false))
                return;
            if (cancelButton_ && cancelButton_->hasKeyboardFocus(false))
                return;

            cancelChanges();
        }
    };

    addChildComponent(*editor_);

    // Create save button
    saveButton_ = std::make_unique<OscilButton>(getThemeService(), "");
    saveButton_->setVariant(ButtonVariant::Icon);
    saveButton_->setIconPath(ListItemIcons::createCheckmarkIcon(static_cast<float>(BUTTON_SIZE)));
    saveButton_->setTooltip("Save (Enter)");
    saveButton_->onClick = [this]() { saveChanges(); };
    addChildComponent(*saveButton_);

    // Create cancel button
    cancelButton_ = std::make_unique<OscilButton>(getThemeService(), "");
    cancelButton_->setVariant(ButtonVariant::Icon);
    cancelButton_->setIconPath(ListItemIcons::createCloseIcon(static_cast<float>(BUTTON_SIZE)));
    cancelButton_->setTooltip("Cancel (Escape)");
    cancelButton_->onClick = [this]() { cancelChanges(); };
    addChildComponent(*cancelButton_);

    updateEditorStyle();
}

void InlineEditLabel::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

void InlineEditLabel::setText(const juce::String& text, bool notify)
{
    if (text_ != text)
    {
        text_ = text;
        if (editor_)
            editor_->setText(text, false);
        repaint();

        if (notify && onTextChanged)
            onTextChanged(text_);
    }
}

void InlineEditLabel::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    if (editor_)
        editor_->setTextToShowWhenEmpty(placeholder, getTheme().textSecondary);
}

void InlineEditLabel::setMaxLength(int maxLength)
{
    maxLength_ = maxLength;
    if (editor_)
        editor_->setInputRestrictions(maxLength);
}

void InlineEditLabel::setValidator(std::function<bool(const juce::String&)> validator)
{
    validator_ = std::move(validator);
}

void InlineEditLabel::setReadOnly(bool readOnly)
{
    readOnly_ = readOnly;
    if (readOnly && editMode_)
        cancelChanges();
}

void InlineEditLabel::enterEditMode()
{
    if (readOnly_ || editMode_)
        return;

    editMode_ = true;
    originalText_ = text_;

    // Update editor with current text
    if (editor_)
    {
        editor_->setText(text_, false);
        editor_->setVisible(true);
        editor_->selectAll();
        editor_->grabKeyboardFocus();
    }

    if (saveButton_)
        saveButton_->setVisible(true);
    if (cancelButton_)
        cancelButton_->setVisible(true);

    updateLayout();
    repaint();

    if (onEditStarted)
        onEditStarted();
}

void InlineEditLabel::exitEditMode(bool saveChanges_)
{
    if (!editMode_)
        return;

    if (saveChanges_)
        saveChanges();
    else
        cancelChanges();
}

void InlineEditLabel::saveChanges()
{
    if (!editMode_)
        return;

    juce::String const newText = editor_ ? editor_->getText().trim() : text_;

    // Validate if validator is set
    if (validator_ && !validator_(newText))
    {
        // Validation failed - revert to original
        cancelChanges();
        return;
    }

    // Don't allow empty text - revert to original
    if (newText.isEmpty())
    {
        cancelChanges();
        return;
    }

    editMode_ = false;

    if (editor_)
        editor_->setVisible(false);
    if (saveButton_)
        saveButton_->setVisible(false);
    if (cancelButton_)
        cancelButton_->setVisible(false);

    bool const changed = (text_ != newText);
    text_ = newText;

    updateLayout();
    repaint();

    if (changed && onTextChanged)
        onTextChanged(text_);
}

void InlineEditLabel::cancelChanges()
{
    if (!editMode_)
        return;

    editMode_ = false;
    text_ = originalText_;

    if (editor_)
    {
        editor_->setText(originalText_, false);
        editor_->setVisible(false);
    }
    if (saveButton_)
        saveButton_->setVisible(false);
    if (cancelButton_)
        cancelButton_->setVisible(false);

    updateLayout();
    repaint();

    if (onEditCancelled)
        onEditCancelled();
}

void InlineEditLabel::setFont(const juce::Font& font)
{
    font_ = font;
    updateEditorStyle();
    repaint();
}

void InlineEditLabel::setTextJustification(juce::Justification justification)
{
    justification_ = justification;
    updateEditorStyle();
    repaint();
}

void InlineEditLabel::setTextColour(juce::Colour colour)
{
    textColour_ = colour;
    useCustomTextColour_ = true;
    updateEditorStyle();
    repaint();
}

void InlineEditLabel::updateEditorStyle()
{
    if (!editor_)
        return;

    const auto& theme = getTheme();
    const auto& glass = getSurface();

    editor_->setFont(font_);
    editor_->setJustification(justification_);

    juce::Colour const textCol = useCustomTextColour_ ? textColour_ : theme.textPrimary;
    editor_->setColour(juce::TextEditor::textColourId, textCol);
    // Glass input background
    editor_->setColour(juce::TextEditor::backgroundColourId, glass.bgGlass);
    editor_->setColour(juce::TextEditor::outlineColourId, glass.borderDefault);
    // Accent border on focus
    editor_->setColour(juce::TextEditor::focusedOutlineColourId, glass.accent);
    editor_->setColour(juce::TextEditor::highlightColourId, glass.accentSubtle);
    editor_->setColour(juce::TextEditor::highlightedTextColourId, textCol);

    editor_->setTextToShowWhenEmpty(placeholder_, theme.textSecondary);
    editor_->setInputRestrictions(maxLength_);
}

void InlineEditLabel::paint(juce::Graphics& g)
{
    if (editMode_)
        return; // Editor and buttons handle rendering

    const auto& theme = getTheme();
    const auto& glass = getSurface();
    auto bounds = getLocalBounds().toFloat();

    // Hover indication — subtle bgHover background to signal editability
    if (isHovered_ && !readOnly_)
    {
        g.setColour(glass.bgHover);
        g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_SM);
    }

    // Get text color — display mode: textPrimary
    juce::Colour const textCol = useCustomTextColour_ ? textColour_ : theme.textPrimary;

    // Draw text with ellipsis if needed
    g.setColour(textCol);
    g.setFont(font_);

    juce::String displayText = text_.isEmpty() ? placeholder_ : text_;
    if (text_.isEmpty())
        g.setColour(theme.textSecondary);

    // Draw text with ellipsis if needed - uses GlyphArrangement for proper text layout
    juce::GlyphArrangement const glyphs = [&]() {
        juce::GlyphArrangement g2;
        g2.addLineOfText(font_, displayText, 0.0f, 0.0f);
        return g2;
    }();
    float const textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

    if (textWidth > bounds.getWidth())
    {
        // Truncate with ellipsis
        juce::String const ellipsis = "...";
        auto measureWidth = [&](const juce::String& text) {
            juce::GlyphArrangement ga;
            ga.addLineOfText(font_, text, 0.0f, 0.0f);
            return ga.getBoundingBox(0, -1, false).getWidth();
        };
        float const ellipsisWidth = measureWidth(ellipsis);
        float const availableWidth = bounds.getWidth() - ellipsisWidth;

        // Binary search for the longest fitting substring (O(log n) measurements)
        int lo = 0;
        int hi = displayText.length();
        while (lo < hi)
        {
            int const mid = (lo + hi + 1) / 2;
            if (measureWidth(displayText.substring(0, mid)) <= availableWidth)
                lo = mid;
            else
                hi = mid - 1;
        }
        displayText = displayText.substring(0, lo) + ellipsis;
    }

    g.drawText(displayText, bounds.toNearestInt(), justification_, false);
}

void InlineEditLabel::resized() { updateLayout(); }

void InlineEditLabel::updateLayout()
{
    auto bounds = getLocalBounds();

    if (editMode_)
    {
        // Buttons on the right
        int const buttonsWidth = (BUTTON_SIZE * 2) + BUTTON_SPACING;
        auto buttonArea = bounds.removeFromRight(buttonsWidth);

        if (cancelButton_)
        {
            cancelButton_->setBounds(
                buttonArea.removeFromRight(BUTTON_SIZE).withSizeKeepingCentre(BUTTON_SIZE, BUTTON_SIZE));
        }

        buttonArea.removeFromRight(BUTTON_SPACING);

        if (saveButton_)
        {
            saveButton_->setBounds(
                buttonArea.removeFromRight(BUTTON_SIZE).withSizeKeepingCentre(BUTTON_SIZE, BUTTON_SIZE));
        }

        // Editor takes remaining space
        if (editor_)
        {
            bounds.removeFromRight(EDITOR_MARGIN);
            editor_->setBounds(bounds);
        }
    }
}

void InlineEditLabel::mouseDown(const juce::MouseEvent& e)
{
    if (onMouseDown)
        onMouseDown(e);
}

void InlineEditLabel::mouseDoubleClick(const juce::MouseEvent& /*e*/)
{
    if (!readOnly_)
        enterEditMode();
}

void InlineEditLabel::mouseEnter(const juce::MouseEvent& /*e*/)
{
    if (!readOnly_ && !editMode_)
    {
        isHovered_ = true;
        repaint();
    }
}

void InlineEditLabel::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (isHovered_)
    {
        isHovered_ = false;
        repaint();
    }
}

void InlineEditLabel::focusLost(FocusChangeType /*cause*/)
{
    // Component-level focus lost - handled by editor's onFocusLost
}

void InlineEditLabel::onThemeChanged(const ColorTheme& /*newTheme*/)
{
    updateEditorStyle();
    // repaint() is called automatically by base class
}

int InlineEditLabel::getPreferredHeight() const
{
    return static_cast<int>(font_.getHeight()) + 8; // Font height + padding
}

} // namespace oscil
