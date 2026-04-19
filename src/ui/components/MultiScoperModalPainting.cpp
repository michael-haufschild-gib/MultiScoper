/*
    MultiScoper - Modal Component Painting
    Rendering, backdrop, title bar, and layout bounds for MultiScoperModal
*/

#include "ui/components/ListItemIcons.h"
#include "ui/components/MultiScoperModal.h"
#include "ui/components/SurfacePainter.h"
#include "ui/theme/Typography.h"

namespace multiscoper
{

void MultiScoperModal::paint(juce::Graphics& g)
{
    float const alpha = showSpring_.position;
    if (alpha < 0.01f)
        return;

    paintBackdrop(g);
    paintModal(g, getModalBounds());
}

void MultiScoperModal::paintBackdrop(juce::Graphics& g)
{
    // Darken the content behind the modal. 60% black dim: strong enough
    // to foreground the modal, not so opaque the backdrop looks "turned off".
    float const alpha = showSpring_.position * 0.6f;
    g.setColour(juce::Colours::black.withAlpha(alpha));
    g.fillRect(getLocalBounds());
}

void MultiScoperModal::paintModal(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float const alpha = showSpring_.position;
    float const scale = scaleSpring_.position;
    const auto& glass = getSurface();

    // Apply scale transform around the center of the modal bounds
    auto scaledBounds = bounds.toFloat().withSizeKeepingCentre(static_cast<float>(bounds.getWidth()) * scale,
                                                               static_cast<float>(bounds.getHeight()) * scale);

    // Save graphics state for transform
    juce::Graphics::ScopedSaveState const saveState(g);
    g.setOpacity(alpha);

    // Glass panel background with full shadow treatment
    SurfacePainter::paintPanel(g, scaledBounds, glass, ComponentLayout::RADIUS_XL, BorderLevel::Default);

    g.setOpacity(1.0f);

    if (title_.isNotEmpty())
    {
        // Calculate title bounds relative to scaled modal
        auto titleBounds = scaledBounds.toNearestInt();
        titleBounds = titleBounds.removeFromTop(TITLE_BAR_HEIGHT);
        paintTitleBar(g, titleBounds);
    }
}

void MultiScoperModal::paintTitleBar(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float const alpha = showSpring_.position;
    const auto& glass = getSurface();

    // Flat titlebar: a slightly raised surface layer above the modal body,
    // separated by a 1px hairline. No gradient, no tinted overlay.
    // Animation alpha is honoured via withMultipliedAlpha so fade-in is clean.
    auto raisedBg = getTheme().backgroundRaised.withMultipliedAlpha(alpha);
    g.setColour(raisedBg);
    g.fillRect(bounds.reduced(1, 0).withBottom(bounds.getBottom()));

    auto textBounds = bounds.reduced(MODAL_PADDING, 0);
    if (showCloseButton_)
        textBounds.removeFromRight(CLOSE_BUTTON_SIZE + 8);

    // Title text: bright primary at typography 'heading' size/weight.
    g.setColour(getTheme().textPrimary.withMultipliedAlpha(alpha));
    g.setFont(Typography::heading());
    g.drawText(title_, textBounds, juce::Justification::centredLeft);

    if (showCloseButton_)
    {
        auto closeBounds = getCloseButtonBounds().toFloat();

        float const hoverAlpha = closeHoverSpring_.position;
        if (hoverAlpha > 0.01f)
        {
            // Close button hover — bgHover (multiply animation alpha into the
            // designed tint; see titlebar note above).
            g.setColour(glass.bgHover.withMultipliedAlpha(alpha * hoverAlpha));
            g.fillRect(closeBounds);
        }

        // No focus ring on hover: the ring uses an outward offset and was
        // visible as a halo behind the button. Keyboard focus state isn't
        // tracked separately for the close icon (it isn't a real Component),
        // so a focus indicator here would either be wrong or redundant.

        auto iconColor = getTheme().textSecondary.interpolatedWith(getTheme().textPrimary, hoverAlpha);
        g.setColour(iconColor.withAlpha(alpha));
        float const iconSize = 14.0f;
        auto iconPath = ListItemIcons::createCloseIcon(iconSize);

        auto iconBounds = iconPath.getBounds();
        float const offsetX = closeBounds.getCentreX() - iconBounds.getCentreX();
        float const offsetY = closeBounds.getCentreY() - iconBounds.getCentreY();
        iconPath.applyTransform(juce::AffineTransform::translation(offsetX, offsetY));

        // SVG icons ship as Lucide outline geometry — stroke the path, do not
        // fill it. Filling a stroke-only path produces an invisible or
        // mis-shapen blob (this caused the "wrong X" in dialog headers).
        constexpr float kIconStrokeWidth = 1.5f;
        g.strokePath(iconPath, juce::PathStrokeType(kIconStrokeWidth, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // 1px hairline divider between titlebar and modal body.
    g.setColour(getTheme().divider.withMultipliedAlpha(alpha));
    g.fillRect(bounds.getX() + 1, bounds.getBottom() - 1, bounds.getWidth() - 2, 1);
}

juce::Rectangle<int> MultiScoperModal::getModalBounds() const
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

juce::Rectangle<int> MultiScoperModal::getTitleBarBounds() const
{
    if (title_.isEmpty())
        return {};

    auto modalBounds = getModalBounds();
    return modalBounds.removeFromTop(TITLE_BAR_HEIGHT);
}

juce::Rectangle<int> MultiScoperModal::getContentBounds() const
{
    auto modalBounds = getModalBounds();

    if (title_.isNotEmpty())
        modalBounds.removeFromTop(TITLE_BAR_HEIGHT);

    return modalBounds.reduced(MODAL_PADDING);
}

juce::Rectangle<int> MultiScoperModal::getCloseButtonBounds() const
{
    auto titleBounds = getTitleBarBounds();
    return {titleBounds.getRight() - MODAL_PADDING - CLOSE_BUTTON_SIZE,
            titleBounds.getCentreY() - (CLOSE_BUTTON_SIZE / 2), CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE};
}

} // namespace multiscoper
