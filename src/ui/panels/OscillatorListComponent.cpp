/*
    Oscil - Oscillator List Component Implementation
*/

#include "ui/panels/OscillatorListComponent.h"

#include "core/OscilLog.h"
#include "ui/components/ComponentConstants.h"

#include <utility>

namespace oscil
{

OscillatorListComponent::OscillatorListComponent(ServiceContext& context)
    : OscillatorListComponent(context.themeService, context.instanceRegistry)
{
}

OscillatorListComponent::OscillatorListComponent(IThemeService& themeService, IInstanceRegistry& instanceRegistry)
    : ThemedComponent(themeService)
    , instanceRegistry_(instanceRegistry)
{
    setOpaque(true);

    setTestId("oscillatorList");

    toolbar_ = std::make_unique<OscillatorListToolbar>(getThemeService());
    toolbar_->addListener(this);
    addAndMakeVisible(toolbar_.get());

    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setComponentID("oscillatorListViewport");
    container_ = std::make_unique<juce::Component>();
    container_->setComponentID("oscillatorListContainer");
    viewport_->setViewedComponent(container_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    viewport_->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    addAndMakeVisible(viewport_.get());

    emptyStateLabel_ = std::make_unique<juce::Label>();
    emptyStateLabel_->setText("No oscillators yet.\nClick '+ Add Oscillator' above to get started.",
                              juce::dontSendNotification);
    emptyStateLabel_->setJustificationType(juce::Justification::centred);
    emptyStateLabel_->setColour(juce::Label::textColourId, getThemeService().getCurrentTheme().textSecondary);
    addChildComponent(emptyStateLabel_.get());
}

OscillatorListComponent::~OscillatorListComponent()
{
    removeAllChildren(); // Prevent double-free/UAF in base Component destructor

    toolbar_->removeListener(this);

    for (auto& item : items_)
    {
        if (item)
            item->removeListener(this);
    }
}

void OscillatorListComponent::registerTestId() { OSCIL_REGISTER_TEST_ID(testId_); }

void OscillatorListComponent::paint(juce::Graphics& g)
{
    const auto& theme = getThemeService().getCurrentTheme();
    g.fillAll(theme.backgroundPane.withAlpha(theme.panelAlpha));

    // Draw drag indicator line
    if (dragTargetIndex_ >= 0)
    {
        // Calculate Y position for indicator relative to this component
        // We need to account for viewport scroll position
        int yPos = 0;
        int const viewY = viewport_->getViewPositionY();

        // Sum heights up to target index
        for (int i = 0; i < dragTargetIndex_; ++i)
        {
            if (static_cast<size_t>(i) < items_.size())
                yPos += items_[static_cast<size_t>(i)]->getHeight();
        }

        // Adjust for scroll and relative positioning
        // container_ is inside viewport_, so yPos is relative to container
        // viewport_ is child of OscillatorListComponent
        int const relativeY = yPos - viewY + viewport_->getY();

        g.setColour(theme.controlActive);
        g.drawHorizontalLine(relativeY, 0.0f, static_cast<float>(getWidth()));
        g.fillEllipse(0.0f, static_cast<float>(relativeY - 3), 6.0f, 6.0f);
        g.fillEllipse(static_cast<float>(getWidth() - 6), static_cast<float>(relativeY - 3), 6.0f, 6.0f);
    }
}

void OscillatorListComponent::resized()
{
    auto bounds = getLocalBounds();

    toolbar_->setBounds(bounds.removeFromTop(OSCILLATOR_TOOLBAR_HEIGHT));

    if (items_.empty())
    {
        emptyStateLabel_->setBounds(bounds.reduced(ComponentLayout::SPACING_MD));
    }

    viewport_->setBounds(bounds);

    if (!items_.empty())
    {
        int contentHeight = 0;
        for (const auto& item : items_)
        {
            contentHeight += item->getPreferredHeight();
        }

        container_->setSize(viewport_->getWidth() - ComponentLayout::SPACING_MD, contentHeight);

        int y = 0;
        for (auto& item : items_)
        {
            int const itemHeight = item->getPreferredHeight();
            item->setBounds(0, y, container_->getWidth(), itemHeight);
            y += itemHeight;
        }
    }
}

void OscillatorListComponent::updateOscillatorCounts()
{
    if (toolbar_)
    {
        int const totalCount = static_cast<int>(allOscillators_.size());
        int visibleCount = 0;
        for (const auto& osc : allOscillators_)
        {
            if (osc.isVisible())
                visibleCount++;
        }
        toolbar_->setOscillatorCount(totalCount, visibleCount);
    }
}

std::vector<Oscillator> OscillatorListComponent::filterOscillators(const std::vector<Oscillator>& oscillators) const
{
    std::vector<Oscillator> filtered;
    filtered.reserve(oscillators.size());
    for (const auto& osc : oscillators)
    {
        bool include = false;
        switch (currentFilterMode_)
        {
            case OscillatorFilterMode::All:
                include = true;
                break;
            case OscillatorFilterMode::Visible:
                include = osc.isVisible();
                break;
            case OscillatorFilterMode::Hidden:
                include = !osc.isVisible();
                break;
        }
        if (include)
            filtered.push_back(osc);
    }
    return filtered;
}

void OscillatorListComponent::syncContainerChildren()
{
    if (static_cast<size_t>(container_->getNumChildComponents()) != items_.size())
    {
        container_->removeAllChildren();
        for (auto& item : items_)
            container_->addAndMakeVisible(*item);
        return;
    }

    for (int i = 0; std::cmp_less(i, items_.size()); ++i)
    {
        if (container_->getChildComponent(i) != items_[static_cast<size_t>(i)].get())
        {
            container_->removeAllChildren();
            for (auto& item : items_)
                container_->addAndMakeVisible(*item);
            return;
        }
    }
}

void OscillatorListComponent::rebuildItems(
    const std::vector<Oscillator>& filtered,
    std::unordered_map<juce::String, std::unique_ptr<OscillatorListItemComponent>>& reusedItems)
{
    for (const auto& osc : filtered)
    {
        std::unique_ptr<OscillatorListItemComponent> item;

        auto it = reusedItems.find(osc.getId().id);
        if (it != reusedItems.end())
        {
            item = std::move(it->second);
            reusedItems.erase(it);
            item->updateFromOscillator(osc);
            OSCIL_LOG(UI, "OscList: reused item for " << osc.getName() << " order=" << osc.getOrderIndex());
        }
        else
        {
            item = std::make_unique<OscillatorListItemComponent>(osc, instanceRegistry_, getThemeService());
            OSCIL_LOG(UI, "OscList: created new item for " << osc.getName() << " order=" << osc.getOrderIndex());
        }

        item->setSelected(osc.getId() == selectedOscillatorId_);
        item->addListener(this);

        if (item->getParentComponent() != container_.get())
            container_->addAndMakeVisible(*item);

        items_.push_back(std::move(item));
    }
}

void OscillatorListComponent::refreshList(const std::vector<Oscillator>& oscillators)
{
    allOscillators_ = oscillators;
    auto filtered = filterOscillators(oscillators);

    OSCIL_LOG(UI, "OscList: refreshList: " << oscillators.size() << " total, " << filtered.size() << " filtered, "
                                           << items_.size() << " existing items");

    // Map existing items by ID for reuse
    std::unordered_map<juce::String, std::unique_ptr<OscillatorListItemComponent>> reusedItems;
    for (auto& item : items_)
    {
        if (item)
        {
            item->removeListener(this);
            reusedItems[item->getOscillatorId().id] = std::move(item);
        }
    }
    items_.clear();

    rebuildItems(filtered, reusedItems);
    reusedItems.clear();

    // Assign contiguous test IDs based on position in the displayed (filtered) list
    for (size_t i = 0; i < items_.size(); ++i)
        items_[i]->setListIndex(static_cast<int>(i));

    syncContainerChildren();

    emptyStateLabel_->setVisible(filtered.empty());
    viewport_->setVisible(!filtered.empty());

    updateOscillatorCounts();
    resized();
}

void OscillatorListComponent::setSelectedOscillator(const OscillatorId& oscillatorId)
{
    selectedOscillatorId_ = oscillatorId;
    for (auto& item : items_)
    {
        if (item)
            item->setSelected(item->getOscillatorId() == selectedOscillatorId_);
    }

    // Layout needs to update because selected item expands
    resized();
}

// Listener forwarding, filter-mode, and drag-and-drop handlers are in
// OscillatorListComponentEvents.cpp.

} // namespace oscil