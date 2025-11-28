/*
    Oscil - Source Selector Component Implementation
    Rich dropdown for selecting signal sources with search, icons, badges
*/

#include "ui/SourceSelectorComponent.h"

namespace oscil
{

//==============================================================================
// SourceListItem implementation
//==============================================================================

SourceListItem::SourceListItem(const SourceInfo& source)
    : sourceId_(source.sourceId)
    , name_(source.name.isEmpty() ? source.sourceId.id : source.name)
    , channelCount_(source.channelCount)
    , isActive_(source.active.load())
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void SourceListItem::paint(juce::Graphics& g)
{
    if (!matchesFilter_)
        return;

    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background
    if (selected_)
    {
        g.setColour(theme.controlActive.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(4, 2), 4.0f);
    }
    else if (hovered_)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(4, 2), 4.0f);
    }

    bounds = bounds.reduced(8, 4);

    // Source icon (left)
    auto iconBounds = bounds.removeFromLeft(24);
    drawSourceIcon(g, iconBounds);

    bounds.removeFromLeft(8);

    // Activity indicator (right) - green dot if active
    auto activityBounds = bounds.removeFromRight(16);
    if (isActive_)
    {
        g.setColour(theme.statusActive);
        float dotSize = 8.0f;
        g.fillEllipse(activityBounds.getCentreX() - dotSize / 2,
                      activityBounds.getCentreY() - dotSize / 2,
                      dotSize, dotSize);
    }

    bounds.removeFromRight(4);

    // Channel badge (right)
    auto badgeBounds = bounds.removeFromRight(50);
    juce::String badgeText = channelCount_ == 2 ? "Stereo" : "Mono";
    juce::Colour badgeColor = channelCount_ == 2 ? theme.controlActive : theme.textSecondary;

    g.setColour(badgeColor.withAlpha(0.2f));
    g.fillRoundedRectangle(badgeBounds.reduced(2, 6), 8.0f);
    g.setColour(badgeColor);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(badgeText, badgeBounds, juce::Justification::centred);

    bounds.removeFromRight(8);

    // Name (remaining space)
    g.setColour(selected_ ? theme.textHighlight : theme.textPrimary);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(name_, bounds.toNearestInt(), juce::Justification::centredLeft, true);
}

void SourceListItem::drawSourceIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.setColour(theme.textSecondary);
    g.setFont(juce::FontOptions(16.0f));

    // Use emoji/unicode icons based on source type detection
    juce::String icon = getIconForSource();
    g.drawText(icon, bounds.toNearestInt(), juce::Justification::centred);
}

juce::String SourceListItem::getIconForSource() const
{
    // Detect source type from name and return appropriate icon
    auto nameLower = name_.toLowerCase();

    if (nameLower.contains("vocal") || nameLower.contains("voice") || nameLower.contains("mic"))
        return juce::String::charToString(0x1F3A4);  // Microphone
    if (nameLower.contains("drum") || nameLower.contains("perc"))
        return juce::String::charToString(0x1F941);  // Drum
    if (nameLower.contains("bass"))
        return juce::String::charToString(0x1F3B8);  // Guitar (bass)
    if (nameLower.contains("guitar") || nameLower.contains("gtr"))
        return juce::String::charToString(0x1F3B8);  // Guitar
    if (nameLower.contains("synth") || nameLower.contains("keys") || nameLower.contains("piano"))
        return juce::String::charToString(0x1F3B9);  // Keyboard
    if (nameLower.contains("master") || nameLower.contains("main") || nameLower.contains("mix"))
        return juce::String::charToString(0x1F50A);  // Speaker

    // Default audio icon
    return juce::String::charToString(0x1F3B5);  // Musical note
}

void SourceListItem::mouseEnter(const juce::MouseEvent&)
{
    hovered_ = true;
    repaint();
}

void SourceListItem::mouseExit(const juce::MouseEvent&)
{
    hovered_ = false;
    repaint();
}

void SourceListItem::mouseUp(const juce::MouseEvent& e)
{
    if (matchesFilter_ && getLocalBounds().contains(e.getPosition()) && onClick)
    {
        onClick(sourceId_);
    }
}

void SourceListItem::setSelected(bool selected)
{
    if (selected_ != selected)
    {
        selected_ = selected;
        repaint();
    }
}

void SourceListItem::setFilterMatch(bool matches)
{
    if (matchesFilter_ != matches)
    {
        matchesFilter_ = matches;
        setVisible(matches);
        repaint();
    }
}

bool SourceListItem::matchesFilter(const juce::String& filter) const
{
    if (filter.isEmpty())
        return true;
    return name_.containsIgnoreCase(filter);
}

//==============================================================================
// NoSourceItem implementation
//==============================================================================

NoSourceItem::NoSourceItem()
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void NoSourceItem::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background on hover
    if (hovered_)
    {
        g.setColour(theme.controlHighlight.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(4, 2), 4.0f);
    }

    // Separator line above
    g.setColour(theme.controlBorder.withAlpha(0.3f));
    g.drawHorizontalLine(2, 8.0f, bounds.getWidth() - 8.0f);

    // Icon and text
    g.setColour(theme.textSecondary);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(juce::String::charToString(0x26D4) + "  No Source (Disconnect)",
               bounds.reduced(12, 0).toNearestInt(),
               juce::Justification::centredLeft);
}

void NoSourceItem::mouseEnter(const juce::MouseEvent&)
{
    hovered_ = true;
    repaint();
}

void NoSourceItem::mouseExit(const juce::MouseEvent&)
{
    hovered_ = false;
    repaint();
}

void NoSourceItem::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()) && onClick)
    {
        onClick();
    }
}

//==============================================================================
// SourceSelectorPopup implementation
//==============================================================================

SourceSelectorPopup::SourceSelectorPopup()
{
    // Search input with magnifier icon
    searchInput_ = std::make_unique<OscilTextField>(TextFieldVariant::Search);
    searchInput_->setPlaceholder("Search sources...");
    searchInput_->onTextChanged = [this](const juce::String& /*text*/) { handleFilterChange(); };
    addAndMakeVisible(*searchInput_);

    // Section header
    sectionHeader_ = std::make_unique<juce::Label>("", "AVAILABLE SOURCES");
    sectionHeader_->setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    addAndMakeVisible(*sectionHeader_);

    // List container and viewport
    listContainer_ = std::make_unique<juce::Component>();
    listViewport_ = std::make_unique<juce::Viewport>();
    listViewport_->setViewedComponent(listContainer_.get(), false);
    listViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*listViewport_);

    // No Source option
    noSourceItem_ = std::make_unique<NoSourceItem>();
    noSourceItem_->onClick = [this]()
    {
        if (onDisconnect)
            onDisconnect();
    };
    addAndMakeVisible(*noSourceItem_);

    // Register listeners
    InstanceRegistry::getInstance().addListener(this);
    ThemeManager::getInstance().addListener(this);

    // Initial population
    refreshSources();
    themeChanged(ThemeManager::getInstance().getCurrentTheme());
}

SourceSelectorPopup::~SourceSelectorPopup()
{
    InstanceRegistry::getInstance().removeListener(this);
    ThemeManager::getInstance().removeListener(this);
}

void SourceSelectorPopup::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    g.fillAll(theme.backgroundPrimary);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRect(getLocalBounds(), 1);
}

void SourceSelectorPopup::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Search input at top
    searchInput_->setBounds(bounds.removeFromTop(SEARCH_HEIGHT));
    bounds.removeFromTop(4);

    // Section header
    sectionHeader_->setBounds(bounds.removeFromTop(HEADER_HEIGHT));
    bounds.removeFromTop(4);

    // No Source at bottom
    noSourceItem_->setBounds(bounds.removeFromBottom(NoSourceItem::ITEM_HEIGHT));
    bounds.removeFromBottom(4);

    // List viewport in remaining space
    listViewport_->setBounds(bounds);

    // Layout list items in container
    int listHeight = 0;
    int itemWidth = bounds.getWidth() - 16; // Account for scrollbar

    for (auto& item : sourceItems_)
    {
        if (item->isVisible())
        {
            item->setBounds(0, listHeight, itemWidth, SourceListItem::ITEM_HEIGHT);
            listHeight += SourceListItem::ITEM_HEIGHT;
        }
    }

    listContainer_->setSize(itemWidth, std::max(listHeight, bounds.getHeight()));
}

void SourceSelectorPopup::setSelectedSourceId(const SourceId& sourceId)
{
    selectedSourceId_ = sourceId;
    for (auto& item : sourceItems_)
    {
        item->setSelected(item->getSourceId() == sourceId);
    }
}

void SourceSelectorPopup::refreshSources()
{
    // Clear existing items
    sourceItems_.clear();

    auto sources = InstanceRegistry::getInstance().getAllSources();

    for (const auto& source : sources)
    {
        auto item = std::make_unique<SourceListItem>(source);
        item->setSelected(source.sourceId == selectedSourceId_);
        item->onClick = [this](const SourceId& id)
        {
            if (onSourceSelected)
                onSourceSelected(id);
        };
        listContainer_->addAndMakeVisible(*item);
        sourceItems_.push_back(std::move(item));
    }

    resized();
}

void SourceSelectorPopup::handleFilterChange()
{
    currentFilter_ = searchInput_->getText();
    applyFilter();
}

void SourceSelectorPopup::applyFilter()
{
    for (auto& item : sourceItems_)
    {
        item->setFilterMatch(item->matchesFilter(currentFilter_));
    }
    resized();
}

void SourceSelectorPopup::sourceAdded(const SourceId&)
{
    juce::Component::SafePointer<SourceSelectorPopup> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis != nullptr)
            safeThis->refreshSources();
    });
}

void SourceSelectorPopup::sourceRemoved(const SourceId&)
{
    juce::Component::SafePointer<SourceSelectorPopup> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis != nullptr)
            safeThis->refreshSources();
    });
}

void SourceSelectorPopup::sourceUpdated(const SourceId&)
{
    juce::Component::SafePointer<SourceSelectorPopup> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis != nullptr)
            safeThis->refreshSources();
    });
}

void SourceSelectorPopup::themeChanged(const ColorTheme& newTheme)
{
    // OscilTextField handles its own theme colors automatically

    // Style header
    sectionHeader_->setColour(juce::Label::textColourId, newTheme.textSecondary);

    repaint();
}

int SourceSelectorPopup::getPreferredHeight() const
{
    int numItems = static_cast<int>(sourceItems_.size());
    int listHeight = numItems * SourceListItem::ITEM_HEIGHT;
    int minListHeight = 100;
    int maxListHeight = 300;

    listHeight = std::max(minListHeight, std::min(listHeight, maxListHeight));

    return 16 + SEARCH_HEIGHT + 4 + HEADER_HEIGHT + 4 + listHeight + 4 + NoSourceItem::ITEM_HEIGHT + 16;
}

//==============================================================================
// SourceSelectorComponent implementation
//==============================================================================

SourceSelectorComponent::SourceSelectorComponent()
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    // Register as listener for source changes
    InstanceRegistry::getInstance().addListener(this);

    // Initial state
    updateDisplayText();
}

SourceSelectorComponent::~SourceSelectorComponent()
{
    InstanceRegistry::getInstance().removeListener(this);
}

void SourceSelectorComponent::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance().getCurrentTheme();
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(theme.controlBackground);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(theme.controlBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    bounds = bounds.reduced(8, 4);

    // Dropdown arrow (right)
    auto arrowBounds = bounds.removeFromRight(16);
    g.setColour(theme.textSecondary);
    juce::Path arrow;
    float arrowX = arrowBounds.getCentreX();
    float arrowY = arrowBounds.getCentreY();
    arrow.addTriangle(arrowX - 4, arrowY - 2, arrowX + 4, arrowY - 2, arrowX, arrowY + 3);
    g.fillPath(arrow);

    bounds.removeFromRight(4);

    // Channel badge if source selected
    if (selectedSourceId_.isValid() && channelCount_ > 0)
    {
        auto badgeBounds = bounds.removeFromRight(50);
        juce::String badgeText = channelCount_ == 2 ? "Stereo" : "Mono";
        juce::Colour badgeColor = channelCount_ == 2 ? theme.controlActive : theme.textSecondary;

        g.setColour(badgeColor.withAlpha(0.2f));
        g.fillRoundedRectangle(badgeBounds.reduced(2, 4), 8.0f);
        g.setColour(badgeColor);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(badgeText, badgeBounds, juce::Justification::centred);

        bounds.removeFromRight(4);
    }

    // Display text
    g.setColour(theme.textPrimary);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(displayText_, bounds.toNearestInt(), juce::Justification::centredLeft, true);
}

void SourceSelectorComponent::resized()
{
    // No child components to layout
}

void SourceSelectorComponent::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()))
    {
        showPopup();
    }
}

void SourceSelectorComponent::showPopup()
{
    // Use unique_ptr immediately to prevent memory leak if setup throws
    auto popup = std::make_unique<SourceSelectorPopup>();
    popup->setSelectedSourceId(selectedSourceId_);
    popup->setSize(280, popup->getPreferredHeight());

    // Set up callbacks
    juce::Component::SafePointer<SourceSelectorComponent> safeThis(this);

    popup->onSourceSelected = [safeThis](const SourceId& id)
    {
        if (safeThis != nullptr)
        {
            safeThis->setSelectedSourceId(id);
            if (safeThis->selectionChangedCallback_)
                safeThis->selectionChangedCallback_(id);
        }
    };

    popup->onDisconnect = [safeThis]()
    {
        if (safeThis != nullptr)
        {
            safeThis->setSelectedSourceId(SourceId::invalid());
            if (safeThis->selectionChangedCallback_)
                safeThis->selectionChangedCallback_(SourceId::invalid());
        }
    };

    juce::CallOutBox::launchAsynchronously(
        std::move(popup),
        getScreenBounds(),
        nullptr);
}

void SourceSelectorComponent::setSelectedSourceId(const SourceId& sourceId)
{
    selectedSourceId_ = sourceId;
    updateDisplayText();
    repaint();
}

void SourceSelectorComponent::updateDisplayText()
{
    if (!selectedSourceId_.isValid())
    {
        displayText_ = "Select Source...";
        channelCount_ = 0;
        return;
    }

    auto sources = InstanceRegistry::getInstance().getAllSources();
    for (const auto& source : sources)
    {
        if (source.sourceId == selectedSourceId_)
        {
            displayText_ = source.name.isEmpty() ? source.sourceId.id : source.name;
            channelCount_ = source.channelCount;
            return;
        }
    }

    // Source not found
    displayText_ = selectedSourceId_.id;
    channelCount_ = 0;
}

void SourceSelectorComponent::refreshSources()
{
    updateDisplayText();
    repaint();
}

void SourceSelectorComponent::sourceAdded(const SourceId&)
{
    juce::Component::SafePointer<SourceSelectorComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis != nullptr)
            safeThis->refreshSources();
    });
}

void SourceSelectorComponent::sourceRemoved(const SourceId&)
{
    juce::Component::SafePointer<SourceSelectorComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis != nullptr)
            safeThis->refreshSources();
    });
}

void SourceSelectorComponent::sourceUpdated(const SourceId&)
{
    juce::Component::SafePointer<SourceSelectorComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis != nullptr)
            safeThis->refreshSources();
    });
}

} // namespace oscil
