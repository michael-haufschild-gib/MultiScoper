/*
    Oscil - Preset Card Component
    Visual card displaying a preset thumbnail with actions overlay
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include "ui/components/ThemedComponent.h"
#include "ui/components/TestId.h"
#include "ui/animation/OscilAnimationService.h"
#include "core/VisualPreset.h"

namespace oscil
{

/**
 * Displays a visual preset as a card with thumbnail and action buttons.
 *
 * Features:
 * - 120x100 card with 120x80 thumbnail and 20px name label
 * - Favorite star icon
 * - System badge (lock) for read-only presets
 * - Hover overlay with action buttons (Edit, Duplicate, Delete, Export, Favorite)
 * - Selection state
 * - Double-click to apply
 */
class PresetCard : public ThemedComponent,
                   public TestIdSupport
{
public:
    /**
     * Listener interface for preset card events.
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void presetCardClicked(const juce::String& /*presetId*/) {}
        virtual void presetCardDoubleClicked(const juce::String& /*presetId*/) {}
        virtual void presetCardEditRequested(const juce::String& /*presetId*/) {}
        virtual void presetCardDuplicateRequested(const juce::String& /*presetId*/) {}
        virtual void presetCardDeleteRequested(const juce::String& /*presetId*/) {}
        virtual void presetCardExportRequested(const juce::String& /*presetId*/) {}
        virtual void presetCardFavoriteToggled(const juce::String& /*presetId*/, bool /*isFavorite*/) {}
    };

    PresetCard(IThemeService& themeService, const juce::String& testId = "");
    ~PresetCard() override;

    // Data
    void setPreset(const VisualPreset& preset);
    const VisualPreset& getPreset() const { return preset_; }
    juce::String getPresetId() const { return preset_.id; }

    // State
    void setSelected(bool selected);
    bool isSelected() const { return selected_; }

    // Listeners
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Size
    static constexpr int CARD_WIDTH = 120;
    static constexpr int CARD_HEIGHT = 100;
    static constexpr int THUMBNAIL_HEIGHT = 80;
    static constexpr int NAME_HEIGHT = 20;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    void parentHierarchyChanged() override;
    void updateAnimations();

    void paintThumbnail(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintName(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintFavoriteIcon(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintSystemBadge(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintOverlay(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintActionButton(juce::Graphics& g, juce::Rectangle<int> bounds,
                           const juce::String& iconName, bool hovered);

    int getHoveredActionIndex(juce::Point<int> pos) const;
    juce::Rectangle<int> getActionButtonBounds(int index) const;
    void handleActionClick(int actionIndex);

    VisualPreset preset_;
    bool selected_ = false;
    bool isHovered_ = false;
    int hoveredActionIndex_ = -1;

    // Animation
    ScopedAnimator hoverAnimator_;
    ScopedAnimator selectAnimator_;
    float currentHoverValue_ = 0.0f;
    float currentSelectValue_ = 0.0f;
    OscilAnimationService* animService_ = nullptr;

    juce::ListenerList<Listener> listeners_;

    // Action button indices
    enum ActionIndex
    {
        ActionEdit = 0,
        ActionDuplicate = 1,
        ActionDelete = 2,
        ActionExport = 3,
        ActionFavorite = 4,
        NumActions = 5
    };

    static constexpr int ACTION_BUTTON_SIZE = 24;
    static constexpr int ACTION_BUTTON_SPACING = 4;
    static constexpr int FAVORITE_ICON_SIZE = 14;
    static constexpr int BADGE_SIZE = 14;

    // TestIdSupport
    void registerTestId() override;
    OSCIL_TESTABLE();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetCard)
};

} // namespace oscil
