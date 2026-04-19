/*
    MultiScoper - Section Header Implementation

    Small-caps titled row with optional prev/next chevrons and a hairline
    divider beneath. Accent colour, when set, tints chevrons and softens the
    divider for colour-coded subsections.
*/

#include "ui/components/SectionHeader.h"

#include "ui/components/ComponentConstants.h"

namespace multiscoper
{

namespace
{
// Visual constants local to SectionHeader — WHY: tuned for 22px row height
// alongside ComponentLayout caption typography.
constexpr int kChevronHitWidth = 20;
constexpr int kChevronGlyphSize = 8;
constexpr int kTitleInsetNoChevrons = 4;
constexpr float kDividerAccentAlpha = 0.35f;
constexpr float kTitleKerning = 0.08f;
} // namespace

SectionHeader::SectionHeader(IThemeService& themeService, juce::String title)
    : ThemedComponent(themeService)
    , title_(std::move(title))
{
    setInterceptsMouseClicks(true, false);
    // Keyboard users (and assistive tech) need a way to invoke onPrev/onNext.
    // Accept focus whenever the component exists so keyPressed() can fire;
    // focus is further gated on chevronsVisible_ inside the handler.
    setWantsKeyboardFocus(true);
}

void SectionHeader::setTitle(const juce::String& title)
{
    if (title_ != title)
    {
        title_ = title;
        repaint();
    }
}

void SectionHeader::setChevronsVisible(bool visible)
{
    if (chevronsVisible_ != visible)
    {
        chevronsVisible_ = visible;
        resized();
        repaint();
    }
}

void SectionHeader::setAccentColour(juce::Colour accent)
{
    if (accent_ == accent)
        return;
    accent_ = accent;
    repaint();
}

void SectionHeader::resized()
{
    const auto bounds = getLocalBounds();

    if (chevronsVisible_)
    {
        prevBounds_ = bounds.withWidth(kChevronHitWidth);
        nextBounds_ = bounds.withTrimmedLeft(bounds.getWidth() - kChevronHitWidth);
    }
    else
    {
        prevBounds_ = {};
        nextBounds_ = {};
    }
}

void SectionHeader::paint(juce::Graphics& g)
{
    const auto& theme = getTheme();
    const bool hasAccent = !accent_.isTransparent();

    const juce::Colour titleColour = hasAccent ? accent_ : theme.textPrimary;
    const juce::Colour chevronColour = hasAccent ? accent_ : theme.textSecondary;
    const juce::Colour dividerColour = hasAccent ? accent_.withAlpha(kDividerAccentAlpha) : theme.divider;

    const auto bounds = getLocalBounds();

    // Title region — centered between chevrons when visible, otherwise full width
    // with a small inset. Using Justification::centred for consistent vertical centering.
    juce::Rectangle<int> titleRect;
    juce::Justification titleJustification = juce::Justification::centred;

    if (chevronsVisible_)
    {
        titleRect = bounds.withTrimmedLeft(kChevronHitWidth).withTrimmedRight(kChevronHitWidth);
    }
    else
    {
        titleRect = bounds.reduced(kTitleInsetNoChevrons, 0);
        titleJustification = juce::Justification::centredLeft;
    }

    g.setColour(titleColour);
    g.setFont(ComponentLayout::captionFont().boldened().withExtraKerningFactor(kTitleKerning));
    g.drawText(title_, titleRect, titleJustification, /*useEllipsesIfTooBig*/ true);

    if (chevronsVisible_)
    {
        g.setColour(chevronColour);

        const auto drawTriangle = [&](juce::Rectangle<int> hit, bool pointLeft) {
            const auto center = hit.getCentre().toFloat();
            const float half = static_cast<float>(kChevronGlyphSize) * 0.5f;

            juce::Path tri;
            if (pointLeft)
            {
                tri.startNewSubPath(center.x - half, center.y);
                tri.lineTo(center.x + half, center.y - half);
                tri.lineTo(center.x + half, center.y + half);
            }
            else
            {
                tri.startNewSubPath(center.x + half, center.y);
                tri.lineTo(center.x - half, center.y - half);
                tri.lineTo(center.x - half, center.y + half);
            }
            tri.closeSubPath();
            g.fillPath(tri);
        };

        drawTriangle(prevBounds_, /*pointLeft*/ true);
        drawTriangle(nextBounds_, /*pointLeft*/ false);
    }

    // Bottom 1px hairline spanning full width.
    g.setColour(dividerColour);
    g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
}

void SectionHeader::mouseDown(const juce::MouseEvent& e)
{
    if (!chevronsVisible_)
        return;

    const auto pos = e.getPosition();
    if (prevBounds_.contains(pos))
    {
        if (onPrev)
            onPrev();
        return;
    }
    if (nextBounds_.contains(pos))
    {
        if (onNext)
            onNext();
    }
}

bool SectionHeader::keyPressed(const juce::KeyPress& key)
{
    if (!chevronsVisible_)
        return false;

    // Map the standard navigation set onto the chevrons so keyboard users
    // and assistive tech can invoke them:
    //   ← / PageUp / Home   → onPrev
    //   → / PageDown / End  → onNext
    // We do not consume Tab or Enter; those stay with JUCE focus traversal
    // and default activation so the component plays nicely inside forms.
    const auto code = key.getKeyCode();
    const bool isPrev =
        code == juce::KeyPress::leftKey || code == juce::KeyPress::pageUpKey || code == juce::KeyPress::homeKey;
    const bool isNext =
        code == juce::KeyPress::rightKey || code == juce::KeyPress::pageDownKey || code == juce::KeyPress::endKey;

    if (isPrev)
    {
        if (onPrev)
            onPrev();
        return true;
    }
    if (isNext)
    {
        if (onNext)
            onNext();
        return true;
    }
    return false;
}

} // namespace multiscoper
