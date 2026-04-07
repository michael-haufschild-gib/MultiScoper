/*
    Oscil - Modal Component Painting
    Rendering, backdrop, title bar, and layout bounds for OscilModal
*/

#include "ui/components/GlassPainter.h"
#include "ui/components/ListItemIcons.h"
#include "ui/components/OscilModal.h"

namespace oscil
{

void OscilModal::paint(juce::Graphics& g)
{
    float const alpha = showSpring_.position;
    if (alpha < 0.01f)
        return;

    paintBackdrop(g);
    paintModal(g, getModalBounds());
}

void OscilModal::paintBackdrop(juce::Graphics& g)
{
    float const alpha = showSpring_.position * 0.5f;
    g.setColour(getTheme().backgroundPrimary.withAlpha(alpha));
    g.fillRect(getLocalBounds());
}

void OscilModal::paintModal(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float const alpha = showSpring_.position;
    float const scale = scaleSpring_.position;
    const auto& glass = getGlass();

    // Apply scale transform around the center of the modal bounds
    auto scaledBounds = bounds.toFloat().withSizeKeepingCentre(static_cast<float>(bounds.getWidth()) * scale,
                                                               static_cast<float>(bounds.getHeight()) * scale);

    // Save graphics state for transform
    juce::Graphics::ScopedSaveState const saveState(g);
    g.setOpacity(alpha);

    // Glass panel background with full shadow treatment
    GlassPainter::paintGlassPanel(g, scaledBounds, glass, ComponentLayout::RADIUS_XL, BorderLevel::Default);

    g.setOpacity(1.0f);

    if (title_.isNotEmpty())
    {
        // Calculate title bounds relative to scaled modal
        auto titleBounds = scaledBounds.toNearestInt();
        titleBounds = titleBounds.removeFromTop(TITLE_BAR_HEIGHT);
        paintTitleBar(g, titleBounds);
    }
}

void OscilModal::paintTitleBar(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float const alpha = showSpring_.position;
    const auto& glass = getGlass();

    // Header background — bgHover tinted
    g.setColour(glass.bgHover.withAlpha(alpha));
    // Only fill top portion with rounded top corners (within the parent's rounded rect)
    g.fillRect(bounds.reduced(1, 0).withBottom(bounds.getBottom()));

    auto textBounds = bounds.reduced(MODAL_PADDING, 0);
    if (showCloseButton_)
        textBounds.removeFromRight(CLOSE_BUTTON_SIZE + 8);

    // Title text — textPrimary
    g.setColour(getTheme().textPrimary.withAlpha(alpha));
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f)).boldened());
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    if (showCloseButton_)
    {
        auto closeBounds = getCloseButtonBounds().toFloat();

        float const hoverAlpha = closeHoverSpring_.position;
        if (hoverAlpha > 0.01f)
        {
            // Close button hover — bgHover
            g.setColour(glass.bgHover.withAlpha(alpha * hoverAlpha));
            g.fillRoundedRectangle(closeBounds, ComponentLayout::RADIUS_SM);
        }

        // Focus ring on close button when hovered
        if (hoverAlpha > 0.5f)
        {
            GlassPainter::paintFocusRing(g, closeBounds, ComponentLayout::RADIUS_SM, glass.accent, 1.5f, 2.0f);
        }

        auto iconColor = getTheme().textSecondary.interpolatedWith(getTheme().textPrimary, hoverAlpha);
        g.setColour(iconColor.withAlpha(alpha));
        float const iconSize = 14.0f;
        auto iconPath = ListItemIcons::createCloseIcon(iconSize);

        auto iconBounds = iconPath.getBounds();
        float const offsetX = closeBounds.getCentreX() - iconBounds.getCentreX();
        float const offsetY = closeBounds.getCentreY() - iconBounds.getCentreY();
        iconPath.applyTransform(juce::AffineTransform::translation(offsetX, offsetY));

        g.fillPath(iconPath);
    }

    // Bottom border — borderDefault
    g.setColour(glass.borderDefault.withAlpha(alpha));
    g.fillRect(bounds.getX() + 1, bounds.getBottom() - 1, bounds.getWidth() - 2, 1);
}

juce::Rectangle<int> OscilModal::getModalBounds() const
{
    auto parentBounds = getLocalBounds();

    int width = 0;
    int height = 0;

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

        int const titleHeight = title_.isNotEmpty() ? TITLE_BAR_HEIGHT : 0;
        int const contentHeight = content_ ? contentPreferredHeight_ : 200;
        height = titleHeight + contentHeight + (MODAL_PADDING * 2);

        int const maxHeight = parentBounds.getHeight() - (MODAL_MARGIN * 2);
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
            titleBounds.getCentreY() - (CLOSE_BUTTON_SIZE / 2), CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE};
}

} // namespace oscil
