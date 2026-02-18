/*
    Oscil - Animation Service Implementation
*/

#include "ui/animation/OscilAnimationService.h"
#include <cmath>

namespace oscil
{

OscilAnimationService::OscilAnimationService(juce::Component* topLevelComponent)
    : topLevelComponent_(topLevelComponent)
{
    if (topLevelComponent_ != nullptr)
        updater_ = std::make_unique<juce::VBlankAnimatorUpdater>(topLevelComponent_);
}

OscilAnimationService::~OscilAnimationService()
{
    stopAll();
}

void OscilAnimationService::stopAll()
{
    // H47 FIX: Reset the VBlank updater to stop all running animations
    // This ensures no callbacks fire after the service is destroyed
    updater_.reset();
    topLevelComponent_ = nullptr;
}

void OscilAnimationService::addAnimator(const juce::Animator& animator)
{
    if (!updater_)
        return;

    size_t currentCount = animatorCount_.load();

    // Check queue limit - log warning if exceeded (JUCE manages cleanup internally)
    if (currentCount >= MAX_QUEUED_ANIMATIONS)
    {
        juce::Logger::writeToLog("OscilAnimationService: Animation queue full (" +
            juce::String(MAX_QUEUED_ANIMATIONS) + "), oldest animations will complete naturally");
    }

    // Check hard limit - reject if exceeded
    if (currentCount >= HARD_LIMIT_ANIMATORS)
    {
        juce::Logger::writeToLog("OscilAnimationService: Hard limit of " +
            juce::String(HARD_LIMIT_ANIMATORS) + " animators reached, rejecting new animator");
        return;
    }

    // Warn if approaching limit (but only once per session to avoid log spam)
    if (currentCount >= MAX_ANIMATORS && !hasWarnedAboutLimit_)
    {
        juce::Logger::writeToLog("OscilAnimationService: Warning - " +
            juce::String(currentCount) + " animators active, approaching limit of " +
            juce::String(HARD_LIMIT_ANIMATORS));
        hasWarnedAboutLimit_ = true;
    }

    updater_->addAnimator(animator);
    animatorCount_.fetch_add(1);
}

void OscilAnimationService::removeAnimator(const juce::Animator& animator)
{
    if (!updater_)
        return;

    updater_->removeAnimator(animator);

    // Decrement count, ensuring we don't underflow
    size_t expected = animatorCount_.load();
    while (expected > 0 && !animatorCount_.compare_exchange_weak(expected, expected - 1))
    {
        // Retry with updated expected value
    }

    // Reset warning flag if we're back under the limit
    if (animatorCount_.load() < MAX_ANIMATORS)
        hasWarnedAboutLimit_ = false;
}

bool OscilAnimationService::shouldAnimate()
{
    return !AnimationSettings::prefersReducedMotion();
}

double OscilAnimationService::getAdjustedDuration(double normalDurationMs)
{
    return shouldAnimate() ? normalDurationMs : 0.0;
}

juce::Animator OscilAnimationService::createFadeIn(juce::Component& target, double durationMs)
{
    // Validate and sanitize duration
    const double sanitizedDuration = AnimationSettings::sanitizeDuration(durationMs, AnimationPresets::MODAL_DURATION_MS);
    const double adjustedDuration = getAdjustedDuration(sanitizedDuration);

    // Use SafePointer to prevent dangling reference if component is destroyed during animation
    juce::Component::SafePointer<juce::Component> safeTarget(&target);

    auto animator = juce::ValueAnimatorBuilder{}
        .withDurationMs(adjustedDuration)
        .withEasing(juce::Easings::createEaseOut())
        .withValueChangedCallback([safeTarget](float progress) {
            if (auto* comp = safeTarget.getComponent())
            {
                // Use AnimationHelper::clampProgress for consistent clamping
                float clampedProgress = AnimationHelper::clampProgress(progress);
                comp->setAlpha(clampedProgress);
                comp->repaint();
            }
        })
        .build();

    // Use queue-managed addAnimator
    addAnimator(animator);

    return animator;
}

juce::Animator OscilAnimationService::createFadeOut(juce::Component& target, double durationMs)
{
    // Validate and sanitize duration
    const double sanitizedDuration = AnimationSettings::sanitizeDuration(durationMs, AnimationPresets::MODAL_DURATION_MS);
    const double adjustedDuration = getAdjustedDuration(sanitizedDuration);

    // Use SafePointer to prevent dangling reference if component is destroyed during animation
    juce::Component::SafePointer<juce::Component> safeTarget(&target);

    auto animator = juce::ValueAnimatorBuilder{}
        .withDurationMs(adjustedDuration)
        .withEasing(juce::Easings::createEaseOut())
        .withValueChangedCallback([safeTarget](float progress) {
            if (auto* comp = safeTarget.getComponent())
            {
                // Use AnimationHelper::clampProgress for consistent clamping
                float clampedProgress = AnimationHelper::clampProgress(progress);
                comp->setAlpha(1.0f - clampedProgress);
                comp->repaint();
            }
        })
        .build();

    // Use queue-managed addAnimator
    addAnimator(animator);

    return animator;
}

juce::Animator OscilAnimationService::createValueAnimation(
    double durationMs,
    std::function<void(float)> onValueChanged,
    std::function<float(float)> easing,
    std::function<void()> onComplete)
{
    // Validate and sanitize duration
    const double sanitizedDuration = AnimationSettings::sanitizeDuration(durationMs, 0.0);
    const double adjustedDuration = getAdjustedDuration(sanitizedDuration);

    // Validate and get safe easing function
    auto safeEasingFn = AnimationSettings::safeEasing(std::move(easing));

    // Wrap the callback to ensure progress is clamped and callback is valid
    auto safeValueCallback = [callback = std::move(onValueChanged)](float progress) {
        if (!callback)
            return;

        // Use AnimationHelper::clampProgress for consistent clamping with NaN handling
        float clampedProgress = AnimationHelper::clampProgress(progress);

        callback(clampedProgress);
    };

    // Wrap completion callback for null safety
    auto safeCompleteCallback = [callback = std::move(onComplete)]() {
        if (callback)
            callback();
    };

    auto builder = juce::ValueAnimatorBuilder{}
        .withDurationMs(adjustedDuration)
        .withEasing(std::move(safeEasingFn))
        .withValueChangedCallback(std::move(safeValueCallback))
        .withOnCompleteCallback(std::move(safeCompleteCallback));

    auto animator = builder.build();

    // Use the queue-managed addAnimator
    addAnimator(animator);

    return animator;
}

juce::Animator OscilAnimationService::createModalShow(
    juce::Component& target,
    std::function<void()> onComplete)
{
    const double adjustedDuration = getAdjustedDuration(AnimationPresets::MODAL_DURATION_MS);

    // Start state
    target.setAlpha(0.0f);
    target.setTransform(juce::AffineTransform::scale(0.95f));

    // Use SafePointer to prevent dangling reference if component is destroyed during animation
    juce::Component::SafePointer<juce::Component> safeTarget(&target);

    auto builder = juce::ValueAnimatorBuilder{}
        .withDurationMs(adjustedDuration)
        .withEasing(juce::Easings::createEaseOut())
        .withValueChangedCallback([safeTarget](float progress) {
            if (auto* comp = safeTarget.getComponent())
            {
                // Use AnimationHelper::clampProgress for consistent clamping
                float clampedProgress = AnimationHelper::clampProgress(progress);
                comp->setAlpha(clampedProgress);
                const float scale = 0.95f + 0.05f * clampedProgress;
                comp->setTransform(juce::AffineTransform::scale(scale));
                comp->repaint();
            }
        });

    // Wrap completion callback for null safety
    builder = builder.withOnCompleteCallback([safeTarget, callback = std::move(onComplete)]() {
        if (auto* comp = safeTarget.getComponent())
        {
            comp->setAlpha(1.0f);
            comp->setTransform({});
        }
        if (callback)
            callback();
    });

    auto animator = builder.build();

    // Use queue-managed addAnimator
    addAnimator(animator);

    return animator;
}

juce::Animator OscilAnimationService::createModalHide(
    juce::Component& target,
    std::function<void()> onComplete)
{
    const double adjustedDuration = getAdjustedDuration(AnimationPresets::MODAL_DURATION_MS);

    // Use SafePointer to prevent dangling reference if component is destroyed during animation
    juce::Component::SafePointer<juce::Component> safeTarget(&target);

    auto builder = juce::ValueAnimatorBuilder{}
        .withDurationMs(adjustedDuration)
        .withEasing(juce::Easings::createEaseOut())
        .withValueChangedCallback([safeTarget](float progress) {
            if (auto* comp = safeTarget.getComponent())
            {
                // Use AnimationHelper::clampProgress for consistent clamping
                float clampedProgress = AnimationHelper::clampProgress(progress);
                comp->setAlpha(1.0f - clampedProgress);
                const float scale = 1.0f - 0.05f * clampedProgress;
                comp->setTransform(juce::AffineTransform::scale(scale));
                comp->repaint();
            }
        });

    // Wrap completion callback for null safety
    builder = builder.withOnCompleteCallback([callback = std::move(onComplete)]() {
        if (callback)
            callback();
    });

    auto animator = builder.build();

    // Use queue-managed addAnimator
    addAnimator(animator);

    return animator;
}

juce::Animator OscilAnimationService::createHoverAnimation(
    std::function<void(float)> onValueChanged,
    std::function<void()> onComplete)
{
    return createValueAnimation(
        AnimationPresets::HOVER_DURATION_MS,
        std::move(onValueChanged),
        juce::Easings::createEase(),
        std::move(onComplete));
}

juce::Animator OscilAnimationService::createExpandAnimation(
    std::function<void(float)> onValueChanged,
    std::function<void()> onComplete)
{
    return createValueAnimation(
        AnimationPresets::EXPAND_DURATION_MS,
        std::move(onValueChanged),
        juce::Easings::createEaseOut(),
        std::move(onComplete));
}

juce::Animator OscilAnimationService::createTabAnimation(
    std::function<void(float)> onValueChanged,
    std::function<void()> onComplete)
{
    return createValueAnimation(
        AnimationPresets::TAB_DURATION_MS,
        std::move(onValueChanged),
        juce::Easings::createEaseInOutCubic(),
        std::move(onComplete));
}

OscilAnimationService* findAnimationService(juce::Component* component)
{
    if (component == nullptr)
        return nullptr;
    
    // Walk up the component tree looking for an animation service provider.
    // The PluginEditor stores a pointer in its properties under "oscilAnimationService".
    juce::Component* current = component;
    while (current != nullptr)
    {
        const auto& props = current->getProperties();
        if (props.contains("oscilAnimationService"))
        {
            // The pointer is stored as an int64 (pointer value)
            auto ptrValue = static_cast<int64_t>(props["oscilAnimationService"]);
            if (ptrValue != 0)
                return reinterpret_cast<OscilAnimationService*>(ptrValue);
        }
        current = current->getParentComponent();
    }
    
    return nullptr;
}

} // namespace oscil

