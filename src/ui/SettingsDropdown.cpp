/*
    Oscil - Settings Dropdown Implementation
    PRD aligned settings panel with visual theme list and layout icons
*/

#include "ui/SettingsDropdown.h"
#include "core/PluginProcessor.h"

namespace oscil
{

//==============================================================================
// ThemeListItem implementation
//==============================================================================

ThemeListItem::ThemeListItem(const juce::String& themeName)
    : themeName_(themeName)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ThemeListItem::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background - highlight if selected or hovered
    if (selected_)
    {
        g.setColour(theme.controlActive);
        g.fillRoundedRectangle(bounds.reduced(2), 4.0f);
    }
    else if (hovered_)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(2), 4.0f);
    }

    // Text
    g.setColour(selected_ ? theme.textHighlight : theme.textPrimary);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(themeName_, bounds.reduced(12, 0), juce::Justification::centredLeft, true);
}

void ThemeListItem::mouseEnter(const juce::MouseEvent&)
{
    hovered_ = true;
    repaint();
}

void ThemeListItem::mouseExit(const juce::MouseEvent&)
{
    hovered_ = false;
    repaint();
}

void ThemeListItem::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()) && onClick)
    {
        onClick(themeName_);
    }
}

void ThemeListItem::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        repaint();
    }
}

//==============================================================================
// LayoutIconButton implementation
//==============================================================================

LayoutIconButton::LayoutIconButton(ColumnLayout layout)
    : layout_(layout)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void LayoutIconButton::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background - rounded rectangle
    if (selected_)
    {
        g.setColour(theme.controlActive);
        g.fillRoundedRectangle(bounds.reduced(2), 6.0f);
    }
    else if (hovered_)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(2), 6.0f);
    }
    else
    {
        g.setColour(theme.controlBackground);
        g.fillRoundedRectangle(bounds.reduced(2), 6.0f);
    }

    // Border
    g.setColour(selected_ ? theme.controlActive : theme.controlBorder);
    g.drawRoundedRectangle(bounds.reduced(2), 6.0f, 1.0f);

    // Draw the layout icon
    drawLayoutIcon(g, bounds.reduced(8));
}

void LayoutIconButton::drawLayoutIcon(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.setColour(selected_ ? theme.textHighlight : theme.textPrimary);

    int numColumns = static_cast<int>(layout_);
    float gap = 3.0f;
    float totalGap = gap * static_cast<float>(numColumns - 1);
    float colWidth = (bounds.getWidth() - totalGap) / static_cast<float>(numColumns);

    for (int i = 0; i < numColumns; ++i)
    {
        float x = bounds.getX() + static_cast<float>(i) * (colWidth + gap);
        juce::Rectangle<float> colRect(x, bounds.getY(), colWidth, bounds.getHeight());
        g.fillRoundedRectangle(colRect, 2.0f);
    }
}

void LayoutIconButton::mouseEnter(const juce::MouseEvent&)
{
    hovered_ = true;
    repaint();
}

void LayoutIconButton::mouseExit(const juce::MouseEvent&)
{
    hovered_ = false;
    repaint();
}

void LayoutIconButton::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()) && onClick)
    {
        onClick(layout_);
    }
}

void LayoutIconButton::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        repaint();
    }
}

//==============================================================================
// SettingsDropdown implementation
//==============================================================================

SettingsDropdown::SettingsDropdown(OscilPluginProcessor& processor)
    : processor_(processor)
{
    juce::ignoreUnused(processor_);  // May be used for future settings

    setTestId("settingsDropdown");

    setupThemeSection();
    setupLayoutSection();
    setupStatusBarSection();

    ThemeManager::getInstance().addListener(this);
}

SettingsDropdown::~SettingsDropdown()
{
    // Unregister child testIds
    OSCIL_UNREGISTER_CHILD_TEST_ID("settingsDropdown_statusBarToggle");
    OSCIL_UNREGISTER_CHILD_TEST_ID("settingsDropdown_layout1Btn");
    OSCIL_UNREGISTER_CHILD_TEST_ID("settingsDropdown_layout2Btn");
    OSCIL_UNREGISTER_CHILD_TEST_ID("settingsDropdown_layout3Btn");
    OSCIL_UNREGISTER_CHILD_TEST_ID("settingsDropdown_editThemeBtn");

    ThemeManager::getInstance().removeListener(this);
}

void SettingsDropdown::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

void SettingsDropdown::setupThemeSection()
{
    // Section label
    themeSectionLabel_ = std::make_unique<juce::Label>("", "THEME");
    themeSectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    addAndMakeVisible(themeSectionLabel_.get());

    // Theme Editor button
    editThemeButton_ = std::make_unique<OscilButton>("Theme Editor...");
    editThemeButton_->setVariant(ButtonVariant::Secondary);
    editThemeButton_->onClick = [this]() { notifyThemeEditRequested(); };
    addAndMakeVisible(editThemeButton_.get());
    OSCIL_REGISTER_CHILD_TEST_ID(*editThemeButton_, "settingsDropdown_editThemeBtn");

    // Populate theme list
    refreshThemeList();
}

void SettingsDropdown::setupLayoutSection()
{
    // Section label
    layoutSectionLabel_ = std::make_unique<juce::Label>("", "LAYOUT");
    layoutSectionLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    addAndMakeVisible(layoutSectionLabel_.get());

    // Create layout icon buttons
    for (int i = 1; i <= 3; ++i)
    {
        auto layout = static_cast<ColumnLayout>(i);
        auto button = std::make_unique<LayoutIconButton>(layout);
        button->setSelected(layout == currentLayout_);
        button->onClick = [this](ColumnLayout l) { selectLayout(l); };
        addAndMakeVisible(*button);

        // Register testId for each button
        juce::String testId = "settingsDropdown_layout" + juce::String(i) + "Btn";
        OSCIL_REGISTER_CHILD_TEST_ID(*button, testId);

        layoutButtons_.push_back(std::move(button));
    }
}

void SettingsDropdown::setupStatusBarSection()
{
    statusBarLabel_ = std::make_unique<juce::Label>("", "STATUS BAR");
    statusBarLabel_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    addAndMakeVisible(statusBarLabel_.get());

    statusBarToggle_ = std::make_unique<OscilToggle>();
    statusBarToggle_->setValue(statusBarVisible_, false);
    statusBarToggle_->onValueChanged = [this](bool value)
    {
        statusBarVisible_ = value;
        notifyStatusBarVisibilityChanged();
    };
    addAndMakeVisible(statusBarToggle_.get());
    OSCIL_REGISTER_CHILD_TEST_ID(*statusBarToggle_, "settingsDropdown_statusBarToggle");
}

void SettingsDropdown::refreshThemeList()
{
    // Clear existing items
    themeItems_.clear();

    auto& tm = ThemeManager::getInstance();
    auto themes = tm.getAvailableThemes();
    auto currentThemeName = tm.getCurrentTheme().name;

    for (const auto& themeName : themes)
    {
        auto item = std::make_unique<ThemeListItem>(themeName);
        item->setSelected(themeName == currentThemeName);
        item->onClick = [this](const juce::String& name) { selectTheme(name); };
        addAndMakeVisible(*item);
        themeItems_.push_back(std::move(item));
    }

    layoutContent();
}

void SettingsDropdown::selectTheme(const juce::String& themeName)
{
    // Update selection state
    for (auto& item : themeItems_)
    {
        item->setSelected(item->getThemeName() == themeName);
    }

    // Apply the theme
    ThemeManager::getInstance().setCurrentTheme(themeName);
}

void SettingsDropdown::selectLayout(ColumnLayout layout)
{
    if (currentLayout_ != layout)
    {
        currentLayout_ = layout;

        // Update button states
        for (auto& button : layoutButtons_)
        {
            button->setSelected(button->getLayout() == layout);
        }

        notifyLayoutModeChanged();
    }
}

void SettingsDropdown::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);

    // Draw section separators
    g.setColour(theme.controlBorder.withAlpha(0.3f));

    // Calculate separator positions based on layout
    int themeListHeight = static_cast<int>(themeItems_.size()) * THEME_ITEM_HEIGHT;
    int y1 = 24 + themeListHeight + 40;  // After theme section
    g.drawHorizontalLine(y1, 10.0f, static_cast<float>(getWidth() - 10));

    int y2 = y1 + 24 + LAYOUT_ICON_SIZE + 16;  // After layout section
    g.drawHorizontalLine(y2, 10.0f, static_cast<float>(getWidth() - 10));
}

void SettingsDropdown::resized()
{
    layoutContent();
}

void SettingsDropdown::layoutContent()
{
    auto bounds = getLocalBounds().reduced(10);
    const int spacing = 8;

    // Theme section header
    themeSectionLabel_->setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);

    // Theme list items
    for (auto& item : themeItems_)
    {
        item->setBounds(bounds.removeFromTop(THEME_ITEM_HEIGHT));
    }
    bounds.removeFromTop(spacing);

    // Theme Editor button
    editThemeButton_->setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(spacing * 2);

    // Layout section header
    layoutSectionLabel_->setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);

    // Layout icons - horizontal row
    auto layoutRow = bounds.removeFromTop(LAYOUT_ICON_SIZE);
    int buttonWidth = (layoutRow.getWidth() - 8) / 3;
    for (auto& button : layoutButtons_)
    {
        button->setBounds(layoutRow.removeFromLeft(buttonWidth));
        layoutRow.removeFromLeft(4);
    }
    bounds.removeFromTop(spacing * 2);

    // Status bar section header
    statusBarLabel_->setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);

    // Status bar toggle
    statusBarToggle_->setBounds(bounds.removeFromTop(28));
}

void SettingsDropdown::themeChanged(const ColorTheme& /*newTheme*/)
{
    // Update theme list selection
    auto currentThemeName = ThemeManager::getInstance().getCurrentTheme().name;
    for (auto& item : themeItems_)
    {
        item->setSelected(item->getThemeName() == currentThemeName);
    }

    // Update label colors
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    themeSectionLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
    layoutSectionLabel_->setColour(juce::Label::textColourId, theme.textSecondary);
    statusBarLabel_->setColour(juce::Label::textColourId, theme.textSecondary);

    // OscilButton and OscilToggle handle their own theme colors automatically

    repaint();
}

void SettingsDropdown::setStatusBarVisible(bool visible)
{
    statusBarVisible_ = visible;
    statusBarToggle_->setValue(visible, false);
}

void SettingsDropdown::setLayoutMode(ColumnLayout layout)
{
    currentLayout_ = layout;
    for (auto& button : layoutButtons_)
    {
        button->setSelected(button->getLayout() == layout);
    }
}

void SettingsDropdown::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void SettingsDropdown::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void SettingsDropdown::notifyLayoutModeChanged()
{
    listeners_.call([this](Listener& l) { l.layoutModeChanged(currentLayout_); });
}

void SettingsDropdown::notifyStatusBarVisibilityChanged()
{
    listeners_.call([this](Listener& l) { l.statusBarVisibilityChanged(statusBarVisible_); });
}

void SettingsDropdown::notifyThemeEditRequested()
{
    listeners_.call([](Listener& l) { l.themeEditRequested(); });
}

int SettingsDropdown::getPreferredHeight() const
{
    // Calculate based on actual content:
    // - Theme section: header (20) + spacing (4) + items (n * 28) + spacing (8) + button (28) + spacing (16)
    // - Layout section: header (20) + spacing (4) + icons (40) + spacing (16)
    // - Status bar section: header (20) + spacing (4) + toggle (28)
    // - Padding: reduced(10) = 20 total vertical padding
    const int numThemes = static_cast<int>(themeItems_.size());
    const int themeSection = 20 + 4 + (numThemes * THEME_ITEM_HEIGHT) + 8 + 28 + 16;
    const int layoutSection = 20 + 4 + LAYOUT_ICON_SIZE + 16;
    const int statusBarSection = 20 + 4 + 28;
    const int padding = 20;

    return themeSection + layoutSection + statusBarSection + padding;
}

// SettingsPopup implementation

SettingsPopup::SettingsPopup(OscilPluginProcessor& processor)
    : settings_(processor)
{
    addAndMakeVisible(settings_);
    // Calculate size based on actual content + 2 pixels for border
    setSize(280, settings_.getPreferredHeight() + 2);
}

void SettingsPopup::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);

    // Draw border
    g.setColour(theme.controlBorder);
    g.drawRect(getLocalBounds(), 1);
}

void SettingsPopup::resized()
{
    settings_.setBounds(getLocalBounds().reduced(1));
}

} // namespace oscil
