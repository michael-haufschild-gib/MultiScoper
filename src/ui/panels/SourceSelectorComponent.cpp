/*
    Oscil - Source Selector Component Implementation
    (SourceListItem implementation is in SourceListItem.cpp)
*/

#include "ui/panels/SourceSelectorComponent.h"

namespace oscil
{
SourceSelectorComponent::SourceSelectorComponent(IThemeService& themeService, IInstanceRegistry& instanceRegistry)
    : themeService_(themeService)
    , instanceRegistry_(instanceRegistry)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    // Register as listener for source changes
    instanceRegistry_.addListener(this);

    // Initial state
    updateDisplayText();
}

SourceSelectorComponent::~SourceSelectorComponent() { instanceRegistry_.removeListener(this); }

void SourceSelectorComponent::paint(juce::Graphics& g)
{
    auto& theme = themeService_.getCurrentTheme();
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
    auto popup = std::make_unique<SourceSelectorPopup>(themeService_, instanceRegistry_);
    popup->setSelectedSourceId(selectedSourceId_);
    popup->setSize(280, popup->getPreferredHeight());

    // Set up callbacks
    juce::Component::SafePointer<SourceSelectorComponent> safeThis(this);

    popup->onSourceSelected = [safeThis](const SourceId& id) {
        if (safeThis != nullptr)
        {
            safeThis->setSelectedSourceId(id);
            if (safeThis->selectionChangedCallback_)
                safeThis->selectionChangedCallback_(id);
        }
    };

    popup->onDisconnect = [safeThis]() {
        if (safeThis != nullptr)
        {
            safeThis->setSelectedSourceId(SourceId::noSource());
            if (safeThis->selectionChangedCallback_)
                safeThis->selectionChangedCallback_(SourceId::noSource());
        }
    };

    juce::CallOutBox::launchAsynchronously(std::move(popup), getScreenBounds(), nullptr);
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

    auto sources = instanceRegistry_.getAllSources();
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
