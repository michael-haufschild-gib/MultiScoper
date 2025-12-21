/*
    Oscil - Preset Card Component Implementation
*/

#include "ui/components/PresetCard.h"

namespace oscil
{

PresetCard::PresetCard(IThemeService& themeService, const juce::String& testId)
    : ThemedComponent(themeService)
    , hoverSpring_(SpringPresets::snappy())
    , selectSpring_(SpringPresets::snappy())
{
    setSize(CARD_WIDTH, CARD_HEIGHT);
    setWantsKeyboardFocus(true);

    if (testId.isNotEmpty())
    {
        OSCIL_REGISTER_TEST_ID(testId);
    }
}

PresetCard::~PresetCard()
{
    stopTimer();
}

void PresetCard::setPreset(const VisualPreset& preset)
{
    preset_ = preset;
    repaint();
}

void PresetCard::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        selectSpring_.setTarget(selected ? 1.0f : 0.0f);
        if (!isTimerRunning())
            startTimerHz(60);
        repaint();
    }
}

void PresetCard::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void PresetCard::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void PresetCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto thumbnailBounds = bounds.removeFromTop(THUMBNAIL_HEIGHT);
    auto nameBounds = bounds;

    float hoverValue = hoverSpring_.position;

    auto& theme = getTheme();

    // Background with selection state
    float selectValue = selectSpring_.position;
    auto bgColour = theme.backgroundPrimary.interpolatedWith(theme.controlActive.withAlpha(0.2f), selectValue);

    // Draw card background
    g.setColour(bgColour);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    // Border
    auto borderColour = selected_ ? theme.controlActive : theme.controlBorder;
    g.setColour(borderColour.withAlpha(0.5f + selectValue * 0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

    // Draw thumbnail area
    paintThumbnail(g, thumbnailBounds.reduced(4, 4));

    // Draw name
    paintName(g, nameBounds);

    // Draw favorite icon
    paintFavoriteIcon(g, thumbnailBounds);

    // Draw system badge if read-only
    if (preset_.isReadOnly)
        paintSystemBadge(g, thumbnailBounds);

    // Draw hover overlay with actions
    if (hoverValue > 0.01f)
        paintOverlay(g, thumbnailBounds.reduced(4, 4));
}

void PresetCard::paintThumbnail(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto& theme = getTheme();

    // Thumbnail background
    g.setColour(theme.backgroundSecondary);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // If we have thumbnail data, decode and draw it
    if (preset_.thumbnailData.isNotEmpty())
    {
        juce::MemoryBlock mb;
        if (mb.fromBase64Encoding(preset_.thumbnailData))
        {
            auto image = juce::ImageFileFormat::loadFrom(mb.getData(), mb.getSize());
            if (image.isValid())
            {
                g.drawImage(image, bounds.toFloat(),
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
                return;
            }
        }
    }

    // Default placeholder - draw a simple waveform preview based on shader type
    g.setColour(theme.textSecondary.withAlpha(0.3f));

    auto centerY = bounds.getCentreY();
    auto width = bounds.getWidth();
    auto startX = bounds.getX();

    juce::Path wavePath;
    wavePath.startNewSubPath(static_cast<float>(startX), static_cast<float>(centerY));

    for (int x = 0; x < width; x++)
    {
        float progress = static_cast<float>(x) / static_cast<float>(width);
        float amplitude = std::sin(progress * juce::MathConstants<float>::twoPi * 3.0f);
        amplitude *= 0.3f * static_cast<float>(bounds.getHeight());
        wavePath.lineTo(static_cast<float>(startX + x),
                       static_cast<float>(centerY) - amplitude);
    }

    g.strokePath(wavePath, juce::PathStrokeType(2.0f));
}

void PresetCard::paintName(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto& theme = getTheme();

    g.setColour(theme.textPrimary);
    auto font = juce::Font(juce::FontOptions().withHeight(12.0f));
    g.setFont(font);

    auto name = preset_.name;
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, name, 0.0f, 0.0f);
    auto textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();

    // Truncate with ellipsis if too long
    int maxWidth = bounds.getWidth() - 8;
    if (textWidth > maxWidth)
    {
        while (name.length() > 3)
        {
            juce::GlyphArrangement testGlyphs;
            testGlyphs.addLineOfText(font, name + "...", 0.0f, 0.0f);
            if (testGlyphs.getBoundingBox(0, -1, false).getWidth() <= maxWidth)
                break;
            name = name.dropLastCharacters(1);
        }
        name += "...";
    }

    g.drawText(name, bounds.reduced(4, 0), juce::Justification::centred, false);
}

void PresetCard::paintFavoriteIcon(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (!preset_.isFavorite && !isHovered_)
        return;

    auto& theme = getTheme();
    auto iconBounds = bounds.removeFromTop(FAVORITE_ICON_SIZE + 4)
                           .removeFromRight(FAVORITE_ICON_SIZE + 4)
                           .reduced(2);

    // Star icon
    juce::Path starPath;
    auto center = iconBounds.getCentre().toFloat();
    auto size = static_cast<float>(FAVORITE_ICON_SIZE) / 2.0f;

    for (int i = 0; i < 5; ++i)
    {
        float angle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 5.0f - juce::MathConstants<float>::halfPi;
        float outerX = center.x + size * std::cos(angle);
        float outerY = center.y + size * std::sin(angle);

        float innerAngle = angle + juce::MathConstants<float>::pi / 5.0f;
        float innerX = center.x + size * 0.4f * std::cos(innerAngle);
        float innerY = center.y + size * 0.4f * std::sin(innerAngle);

        if (i == 0)
            starPath.startNewSubPath(outerX, outerY);
        else
            starPath.lineTo(outerX, outerY);

        starPath.lineTo(innerX, innerY);
    }
    starPath.closeSubPath();

    if (preset_.isFavorite)
    {
        g.setColour(juce::Colours::gold);
        g.fillPath(starPath);
    }
    else
    {
        g.setColour(theme.textSecondary.withAlpha(0.5f));
        g.strokePath(starPath, juce::PathStrokeType(1.0f));
    }
}

void PresetCard::paintSystemBadge(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto& theme = getTheme();
    auto badgeBounds = bounds.removeFromTop(BADGE_SIZE + 4)
                            .removeFromLeft(BADGE_SIZE + 4)
                            .reduced(2);

    // Lock icon background
    g.setColour(theme.backgroundPrimary.withAlpha(0.8f));
    g.fillRoundedRectangle(badgeBounds.toFloat(), 3.0f);

    // Lock icon
    g.setColour(theme.textSecondary);
    auto lockBounds = badgeBounds.reduced(2).toFloat();
    auto lockCenter = lockBounds.getCentre();
    auto lockWidth = lockBounds.getWidth() * 0.6f;
    auto lockHeight = lockBounds.getHeight() * 0.4f;

    // Lock body
    g.fillRoundedRectangle(lockCenter.x - lockWidth / 2.0f,
                           lockCenter.y,
                           lockWidth, lockHeight, 2.0f);

    // Lock shackle
    juce::Path shackle;
    auto shackleWidth = lockWidth * 0.6f;
    auto shackleHeight = lockHeight * 0.8f;
    shackle.addArc(lockCenter.x - shackleWidth / 2.0f,
                   lockCenter.y - shackleHeight,
                   shackleWidth, shackleHeight * 2.0f,
                   juce::MathConstants<float>::pi, 0.0f, true);
    g.strokePath(shackle, juce::PathStrokeType(1.5f));
}

void PresetCard::paintOverlay(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto& theme = getTheme();
    float opacity = hoverSpring_.position;

    // Semi-transparent overlay
    g.setColour(theme.backgroundPrimary.withAlpha(0.8f * opacity));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Action buttons
    int numActions = preset_.isReadOnly ? 4 : 5; // No delete for system presets
    int totalWidth = numActions * ACTION_BUTTON_SIZE + (numActions - 1) * ACTION_BUTTON_SPACING;
    int startX = bounds.getCentreX() - totalWidth / 2;
    int buttonY = bounds.getCentreY() - ACTION_BUTTON_SIZE / 2;

    for (int i = 0; i < numActions; ++i)
    {
        int actionIndex = i;
        if (preset_.isReadOnly && i >= ActionDelete)
            actionIndex = i + 1; // Skip delete

        auto buttonBounds = juce::Rectangle<int>(
            startX + i * (ACTION_BUTTON_SIZE + ACTION_BUTTON_SPACING),
            buttonY,
            ACTION_BUTTON_SIZE,
            ACTION_BUTTON_SIZE
        );

        juce::String iconName;
        switch (actionIndex)
        {
            case ActionEdit:     iconName = "pencil"; break;
            case ActionDuplicate: iconName = "copy"; break;
            case ActionDelete:   iconName = "bin"; break;
            case ActionExport:   iconName = "upload"; break;
            case ActionFavorite: iconName = preset_.isFavorite ? "star-filled" : "star"; break;
        }

        paintActionButton(g, buttonBounds, iconName, hoveredActionIndex_ == actionIndex);
    }
}

void PresetCard::paintActionButton(juce::Graphics& g, juce::Rectangle<int> bounds,
                                    const juce::String& iconName, bool hovered)
{
    auto& theme = getTheme();

    // Button background
    auto bgColour = hovered ? theme.controlActive.withAlpha(0.2f) : theme.backgroundPrimary.withAlpha(0.5f);
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(hovered ? theme.controlActive : theme.controlBorder);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);

    // Icon placeholder (draw simple shapes based on icon name)
    g.setColour(hovered ? theme.controlActive : theme.textSecondary);
    auto iconBounds = bounds.reduced(5).toFloat();

    if (iconName == "pencil")
    {
        // Simple pencil icon
        juce::Path path;
        path.startNewSubPath(iconBounds.getX(), iconBounds.getBottom());
        path.lineTo(iconBounds.getRight() - 2, iconBounds.getY() + 2);
        path.lineTo(iconBounds.getRight(), iconBounds.getY());
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }
    else if (iconName == "copy")
    {
        // Two overlapping rectangles
        auto rect1 = iconBounds.reduced(1).translated(-2, 2);
        auto rect2 = iconBounds.reduced(1).translated(2, -2);
        g.drawRoundedRectangle(rect1, 2.0f, 1.0f);
        g.setColour(bgColour);
        g.fillRoundedRectangle(rect2, 2.0f);
        g.setColour(hovered ? theme.controlActive : theme.textSecondary);
        g.drawRoundedRectangle(rect2, 2.0f, 1.0f);
    }
    else if (iconName == "bin")
    {
        // Trash can icon
        auto topLine = iconBounds.removeFromTop(2);
        g.fillRect(topLine.reduced(2, 0));
        auto body = iconBounds.reduced(2, 0);
        g.drawRoundedRectangle(body, 1.0f, 1.0f);
    }
    else if (iconName == "upload")
    {
        // Arrow pointing up
        auto center = iconBounds.getCentre();
        juce::Path arrow;
        arrow.startNewSubPath(center.x, iconBounds.getY());
        arrow.lineTo(iconBounds.getX() + 2, center.y);
        arrow.startNewSubPath(center.x, iconBounds.getY());
        arrow.lineTo(iconBounds.getRight() - 2, center.y);
        arrow.startNewSubPath(center.x, iconBounds.getY());
        arrow.lineTo(center.x, iconBounds.getBottom());
        g.strokePath(arrow, juce::PathStrokeType(1.5f));
    }
    else if (iconName == "star" || iconName == "star-filled")
    {
        // Star icon
        juce::Path starPath;
        auto center = iconBounds.getCentre();
        auto size = iconBounds.getWidth() / 2.0f;

        for (int i = 0; i < 5; ++i)
        {
            float angle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 5.0f - juce::MathConstants<float>::halfPi;
            float outerX = center.x + size * std::cos(angle);
            float outerY = center.y + size * std::sin(angle);

            float innerAngle = angle + juce::MathConstants<float>::pi / 5.0f;
            float innerX = center.x + size * 0.4f * std::cos(innerAngle);
            float innerY = center.y + size * 0.4f * std::sin(innerAngle);

            if (i == 0)
                starPath.startNewSubPath(outerX, outerY);
            else
                starPath.lineTo(outerX, outerY);

            starPath.lineTo(innerX, innerY);
        }
        starPath.closeSubPath();

        if (iconName == "star-filled")
        {
            g.setColour(juce::Colours::gold);
            g.fillPath(starPath);
        }
        else
        {
            g.strokePath(starPath, juce::PathStrokeType(1.0f));
        }
    }
}

int PresetCard::getHoveredActionIndex(juce::Point<int> pos) const
{
    auto bounds = getLocalBounds().removeFromTop(THUMBNAIL_HEIGHT).reduced(4, 4);
    int numActions = preset_.isReadOnly ? 4 : 5;
    int totalWidth = numActions * ACTION_BUTTON_SIZE + (numActions - 1) * ACTION_BUTTON_SPACING;
    int startX = bounds.getCentreX() - totalWidth / 2;
    int buttonY = bounds.getCentreY() - ACTION_BUTTON_SIZE / 2;

    for (int i = 0; i < numActions; ++i)
    {
        auto buttonBounds = juce::Rectangle<int>(
            startX + i * (ACTION_BUTTON_SIZE + ACTION_BUTTON_SPACING),
            buttonY,
            ACTION_BUTTON_SIZE,
            ACTION_BUTTON_SIZE
        );

        if (buttonBounds.contains(pos))
        {
            int actionIndex = i;
            if (preset_.isReadOnly && i >= ActionDelete)
                actionIndex = i + 1; // Skip delete
            return actionIndex;
        }
    }

    return -1;
}

juce::Rectangle<int> PresetCard::getActionButtonBounds(int /*index*/) const
{
    // Currently unused - returns empty rectangle
    // Preserved for potential future extension
    return {};
}

void PresetCard::handleActionClick(int actionIndex)
{
    // Copy values to avoid capturing 'this' in lambdas - prevents use-after-free
    // if the component is destroyed while listener is processing the callback
    auto presetId = preset_.id;
    bool isFavorite = preset_.isFavorite;

    switch (actionIndex)
    {
        case ActionEdit:
            listeners_.call([presetId](Listener& l) { l.presetCardEditRequested(presetId); });
            break;
        case ActionDuplicate:
            listeners_.call([presetId](Listener& l) { l.presetCardDuplicateRequested(presetId); });
            break;
        case ActionDelete:
            listeners_.call([presetId](Listener& l) { l.presetCardDeleteRequested(presetId); });
            break;
        case ActionExport:
            listeners_.call([presetId](Listener& l) { l.presetCardExportRequested(presetId); });
            break;
        case ActionFavorite:
            listeners_.call([presetId, isFavorite](Listener& l) { l.presetCardFavoriteToggled(presetId, !isFavorite); });
            break;
    }
}

void PresetCard::resized()
{
    // No child components to layout
}

void PresetCard::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        // Context menu will be handled by parent
        return;
    }

    // Check for action button click
    if (isHovered_ && hoverSpring_.position > 0.5f)
    {
        int actionIndex = getHoveredActionIndex(e.getPosition());
        if (actionIndex >= 0)
        {
            handleActionClick(actionIndex);
            return;
        }
    }
}

void PresetCard::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() || e.mouseWasClicked())
    {
        // Only trigger click if not clicking an action button
        if (getHoveredActionIndex(e.getPosition()) < 0)
        {
            // Copy preset ID before calling listeners to avoid use-after-free
            // if a listener deletes this card during the callback
            auto presetId = preset_.id;
            listeners_.call([presetId](Listener& l) { l.presetCardClicked(presetId); });
        }
    }
}

void PresetCard::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (getHoveredActionIndex(e.getPosition()) < 0)
    {
        // Copy preset ID before calling listeners to avoid use-after-free
        // if a listener deletes this card during the callback
        auto presetId = preset_.id;
        listeners_.call([presetId](Listener& l) { l.presetCardDoubleClicked(presetId); });
    }
}

void PresetCard::mouseEnter(const juce::MouseEvent& /*e*/)
{
    isHovered_ = true;
    hoverSpring_.setTarget(1.0f);
    if (!isTimerRunning())
        startTimerHz(60);
    repaint();
}

void PresetCard::mouseExit(const juce::MouseEvent& /*e*/)
{
    isHovered_ = false;
    hoveredActionIndex_ = -1;
    hoverSpring_.setTarget(0.0f);
    if (!isTimerRunning())
        startTimerHz(60);
    repaint();
}

void PresetCard::mouseMove(const juce::MouseEvent& e)
{
    int newHoveredAction = getHoveredActionIndex(e.getPosition());
    if (newHoveredAction != hoveredActionIndex_)
    {
        hoveredActionIndex_ = newHoveredAction;
        repaint();
    }
}

void PresetCard::timerCallback()
{
    updateAnimations();
}

void PresetCard::updateAnimations()
{
    bool needsRepaint = false;

    float deltaTime = 1.0f / 60.0f; // 60 FPS
    hoverSpring_.update(deltaTime);
    selectSpring_.update(deltaTime);

    if (hoverSpring_.needsUpdate() || selectSpring_.needsUpdate())
        needsRepaint = true;

    if (needsRepaint)
        repaint();

    // Stop timer when animations complete
    if (hoverSpring_.isSettled() && selectSpring_.isSettled())
        stopTimer();
}

void PresetCard::registerTestId()
{
    // Test ID is set in constructor
}

} // namespace oscil
