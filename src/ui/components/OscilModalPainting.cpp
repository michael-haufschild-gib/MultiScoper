/*
    Oscil - Modal Component Painting
    Rendering, backdrop, title bar, and layout bounds for OscilModal
*/

#include "ui/components/ListItemIcons.h"
#include "ui/components/OscilModal.h"

namespace oscil
{

void OscilModal::paint(juce::Graphics& g)
{
    float alpha = showSpring_.position;
    if (alpha < 0.01f)
        return;

    paintBackdrop(g);
    paintModal(g, getModalBounds());
}

void OscilModal::paintBackdrop(juce::Graphics& g)
{
    float alpha = showSpring_.position * 0.5f;
    g.setColour(juce::Colours::black.withAlpha(alpha));
    g.fillRect(getLocalBounds());
}

void OscilModal::paintModal(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float alpha = showSpring_.position;

    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.3f * alpha), 20, {0, 4});
    shadow.drawForRectangle(g, bounds);

    g.setColour(getTheme().backgroundPrimary.withAlpha(alpha));
    g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_LG);

    g.setColour(getTheme().controlBorder.withAlpha(alpha * 0.3f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), ComponentLayout::RADIUS_LG, 1.0f);

    if (title_.isNotEmpty())
    {
        auto titleBounds = bounds.removeFromTop(TITLE_BAR_HEIGHT);
        paintTitleBar(g, titleBounds);
    }
}

void OscilModal::paintTitleBar(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float alpha = showSpring_.position;

    auto textBounds = bounds.reduced(MODAL_PADDING, 0);
    if (showCloseButton_)
        textBounds.removeFromRight(CLOSE_BUTTON_SIZE + 8);

    g.setColour(getTheme().textPrimary.withAlpha(alpha));
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f)).boldened());
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    if (showCloseButton_)
    {
        auto closeBounds = getCloseButtonBounds().toFloat();

        float hoverAlpha = closeHoverSpring_.position;
        if (hoverAlpha > 0.01f)
        {
            g.setColour(getTheme().backgroundSecondary.withAlpha(alpha * hoverAlpha));
            g.fillRoundedRectangle(closeBounds, ComponentLayout::RADIUS_SM);
        }

        auto iconColor = getTheme().textSecondary.interpolatedWith(getTheme().textPrimary, hoverAlpha);
        g.setColour(iconColor.withAlpha(alpha));
        float iconSize = 14.0f;
        auto iconPath = ListItemIcons::createCloseIcon(iconSize);

        auto iconBounds = iconPath.getBounds();
        float offsetX = closeBounds.getCentreX() - iconBounds.getCentreX();
        float offsetY = closeBounds.getCentreY() - iconBounds.getCentreY();
        iconPath.applyTransform(juce::AffineTransform::translation(offsetX, offsetY));

        g.fillPath(iconPath);
    }

    g.setColour(getTheme().controlBorder.withAlpha(alpha * 0.3f));
    g.fillRect(bounds.getX() + 1, bounds.getBottom() - 1, bounds.getWidth() - 2, 1);
}

juce::Rectangle<int> OscilModal::getModalBounds() const
{
    auto parentBounds = getLocalBounds();

    int width = 0, height = 0;

    if (customWidth_ > 0 && customHeight_ > 0)
    {
        width = customWidth_;
        height = customHeight_;
    }
    else
    {
        switch (modalSize_)
        {
            case ModalSize::Small:
                width = SIZE_SMALL;
                break;
            case ModalSize::Medium:
                width = SIZE_MEDIUM;
                break;
            case ModalSize::Large:
                width = SIZE_LARGE;
                break;
            case ModalSize::FullScreen:
                return parentBounds.reduced(MODAL_MARGIN);
            case ModalSize::Auto:
                width = SIZE_MEDIUM;
                break;
        }

        int titleHeight = title_.isNotEmpty() ? TITLE_BAR_HEIGHT : 0;
        int contentHeight = content_ ? contentPreferredHeight_ : 200;
        height = titleHeight + contentHeight + MODAL_PADDING * 2;

        int maxHeight = parentBounds.getHeight() - MODAL_MARGIN * 2;
        height = std::min(height, maxHeight);
    }

    return parentBounds.withSizeKeepingCentre(width, height);
}

juce::Rectangle<int> OscilModal::getTitleBarBounds() const
{
    if (title_.isEmpty())
        return {};

    auto modalBounds = getModalBounds();
    return modalBounds.removeFromTop(TITLE_BAR_HEIGHT);
}

juce::Rectangle<int> OscilModal::getContentBounds() const
{
    auto modalBounds = getModalBounds();

    if (title_.isNotEmpty())
        modalBounds.removeFromTop(TITLE_BAR_HEIGHT);

    return modalBounds.reduced(MODAL_PADDING);
}

juce::Rectangle<int> OscilModal::getCloseButtonBounds() const
{
    auto titleBounds = getTitleBarBounds();
    return {titleBounds.getRight() - MODAL_PADDING - CLOSE_BUTTON_SIZE,
            titleBounds.getCentreY() - CLOSE_BUTTON_SIZE / 2, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE};
}

} // namespace oscil
