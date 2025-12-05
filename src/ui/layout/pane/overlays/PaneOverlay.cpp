/*
    Oscil - Pane Overlay Base Class Implementation
*/

#include "ui/layout/pane/overlays/PaneOverlay.h"
#include "ui/theme/ThemeManager.h"

namespace oscil
{

// Helper timer class for fade animation
class FadeAnimationTimer : public juce::Timer
{
public:
    explicit FadeAnimationTimer(std::function<void()> callback)
        : callback_(std::move(callback)) {}

    void timerCallback() override
    {
        if (callback_)
            callback_();
    }

private:
    std::function<void()> callback_;
};

PaneOverlay::PaneOverlay(IThemeService& themeService)
    : themeService_(themeService)
{
    setOpaque(false);
    setInterceptsMouseClicks(false, false);  // Default click-through

    themeService_.addListener(this);
}

PaneOverlay::PaneOverlay(IThemeService& themeService, const juce::String& testId)
    : PaneOverlay(themeService)
{
#if defined(TEST_HARNESS) || defined(OSCIL_ENABLE_TEST_IDS)
    OSCIL_REGISTER_TEST_ID(testId.toRawUTF8());
#else
    juce::ignoreUnused(testId);
#endif
}

PaneOverlay::~PaneOverlay()
{
    if (fadeTimer_)
        fadeTimer_->stopTimer();

    themeService_.removeListener(this);
}

void PaneOverlay::setPosition(Position pos)
{
    if (position_ != pos)
    {
        position_ = pos;
        if (auto* parent = getParentComponent())
            updatePositionInParent(parent->getLocalBounds());
    }
}

void PaneOverlay::setVisibleAnimated(bool shouldBeVisible)
{
    if (shouldBeVisible == isVisible() && !isAnimating_)
        return;

    targetOpacity_ = shouldBeVisible ? 1.0f : 0.0f;

    if (shouldBeVisible)
    {
        // Make visible immediately, then fade in
        setVisible(true);
        startFadeAnimation(true);
    }
    else
    {
        // Fade out, then hide
        startFadeAnimation(false);
    }
}

void PaneOverlay::startFadeAnimation(bool fadeIn)
{
    fadeDirection_ = fadeIn;
    isAnimating_ = true;

    if (!fadeTimer_)
    {
        fadeTimer_ = std::make_unique<FadeAnimationTimer>([this]() {
            updateFadeAnimation();
        });
    }

    fadeTimer_->startTimer(FADE_TIMER_INTERVAL_MS);
}

void PaneOverlay::updateFadeAnimation()
{
    const float fadeStep = static_cast<float>(FADE_TIMER_INTERVAL_MS) / FADE_DURATION_MS;

    if (fadeDirection_)
    {
        currentOpacity_ = juce::jmin(1.0f, currentOpacity_ + fadeStep);
        if (currentOpacity_ >= 1.0f)
        {
            currentOpacity_ = 1.0f;
            isAnimating_ = false;
            fadeTimer_->stopTimer();
        }
    }
    else
    {
        currentOpacity_ = juce::jmax(0.0f, currentOpacity_ - fadeStep);
        if (currentOpacity_ <= 0.0f)
        {
            currentOpacity_ = 0.0f;
            isAnimating_ = false;
            fadeTimer_->stopTimer();
            setVisible(false);
        }
    }

    repaint();
}

void PaneOverlay::paint(juce::Graphics& g)
{
    if (currentOpacity_ <= 0.0f)
        return;

    // Apply current animation opacity
    g.setOpacity(currentOpacity_);
    paintOverlayBackground(g);
}

void PaneOverlay::paintOverlayBackground(juce::Graphics& g)
{
    const auto& theme = themeService_.getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Semi-transparent background
    g.setColour(theme.backgroundSecondary.withAlpha(backgroundOpacity_));
    g.fillRoundedRectangle(bounds, CORNER_RADIUS);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), CORNER_RADIUS, 1.0f);
}

bool PaneOverlay::hitTest(int x, int y)
{
    if (clickThrough_)
        return false;

    return getLocalBounds().contains(x, y);
}

void PaneOverlay::themeChanged(const ColorTheme& /*newTheme*/)
{
    repaint();
}

juce::Rectangle<int> PaneOverlay::getContentBounds() const
{
    return getLocalBounds().reduced(CONTENT_PADDING);
}

juce::Rectangle<int> PaneOverlay::getPreferredBounds() const
{
    auto contentSize = getPreferredContentSize();
    return contentSize.withSizeKeepingCentre(
        contentSize.getWidth() + 2 * CONTENT_PADDING,
        contentSize.getHeight() + 2 * CONTENT_PADDING
    );
}

void PaneOverlay::updatePositionInParent(juce::Rectangle<int> parentBounds)
{
    auto preferredBounds = getPreferredBounds();
    int width = preferredBounds.getWidth();
    int height = preferredBounds.getHeight();

    int x = 0, y = 0;

    switch (position_)
    {
        case Position::TopLeft:
            x = parentBounds.getX() + margin_;
            y = parentBounds.getY() + margin_;
            break;

        case Position::TopRight:
            x = parentBounds.getRight() - width - margin_;
            y = parentBounds.getY() + margin_;
            break;

        case Position::BottomLeft:
            x = parentBounds.getX() + margin_;
            y = parentBounds.getBottom() - height - margin_;
            break;

        case Position::BottomRight:
            x = parentBounds.getRight() - width - margin_;
            y = parentBounds.getBottom() - height - margin_;
            break;
    }

    setBounds(x, y, width, height);
}

} // namespace oscil
