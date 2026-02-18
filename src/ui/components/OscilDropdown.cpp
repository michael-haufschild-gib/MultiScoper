/*
    Oscil - Dropdown Component Implementation
    Migrated to JUCE Animator for VBlank-synced animations
*/

#include "ui/components/OscilDropdown.h"
#include "plugin/PluginEditor.h"

namespace oscil
{

//==============================================================================
// OscilDropdownPopup Implementation
//==============================================================================

class OscilDropdownPopup::ItemList : public juce::Component
{
public:
    explicit ItemList(OscilDropdownPopup& owner) : owner_(owner)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics& g) override
    {
        float alpha = owner_.showProgress_;
        if (alpha < 0.01f) return;

        int start = g.getClipBounds().getY() / ITEM_HEIGHT;
        int end = (g.getClipBounds().getBottom() + ITEM_HEIGHT) / ITEM_HEIGHT;

        start = std::max(0, start);
        end = std::min(end, (int)owner_.filteredIndices_.size());

        for (int i = start; i < end; ++i)
        {
            int itemIndex = owner_.filteredIndices_[static_cast<size_t>(i)];
            const auto& item = owner_.items_[static_cast<size_t>(itemIndex)];
            auto itemBounds = juce::Rectangle<int>(0, i * ITEM_HEIGHT, getWidth(), ITEM_HEIGHT);

            bool isHovered = (itemIndex == owner_.hoveredIndex_);
            bool isSelected = owner_.selectedIndices_.count(itemIndex) > 0;
            bool isFocused = (i == owner_.focusedIndex_);

            paintItem(g, item, itemBounds, isHovered, isSelected, isFocused, alpha);
        }
    }

    void paintItem(juce::Graphics& g, const DropdownItem& item,
                  juce::Rectangle<int> bounds, bool isHovered, bool isSelected, bool isFocused, float alpha)
    {
        const auto& theme = owner_.getTheme();

        if (item.isSeparator)
        {
            g.setColour(theme.controlBorder.withAlpha(alpha * 0.5f));
            g.fillRect(bounds.reduced(8, ITEM_HEIGHT / 2 - 1).withHeight(1));
            return;
        }

        float opacity = item.enabled ? 1.0f : ComponentLayout::DISABLED_OPACITY;

        if (isSelected)
        {
            g.setColour(theme.controlActive.withAlpha(alpha * 0.15f));
            g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);
        }
        else if (isFocused)
        {
            g.setColour(theme.controlActive.withAlpha(alpha * 0.1f));
            g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);
        }
        
        if (isHovered && item.enabled && !isSelected)
        {
            g.setColour(theme.backgroundSecondary.withAlpha(alpha * 0.8f));
            g.fillRoundedRectangle(bounds.toFloat(), ComponentLayout::RADIUS_SM);
        }

        auto contentBounds = bounds.reduced(8, 0);

        if (item.icon.isValid())
        {
            auto iconBounds = contentBounds.removeFromLeft(20).toFloat();
            g.setOpacity(alpha * opacity);
            g.drawImage(item.icon,
                iconBounds.withSizeKeepingCentre(16, 16),
                juce::RectanglePlacement::centred);
            contentBounds.removeFromLeft(4);
        }

        if (owner_.multiSelect_)
        {
            auto checkBounds = contentBounds.removeFromLeft(20).toFloat()
                .withSizeKeepingCentre(16, 16);

            g.setColour(theme.backgroundSecondary.withAlpha(alpha * opacity));
            g.fillRoundedRectangle(checkBounds, 3.0f);

            g.setColour((isSelected ? theme.controlActive : theme.controlBorder)
                .withAlpha(alpha * opacity));
            g.drawRoundedRectangle(checkBounds.reduced(0.5f), 3.0f, 1.0f);

            if (isSelected)
            {
                g.setColour(theme.controlActive.withAlpha(alpha * opacity));
                float cx = checkBounds.getCentreX();
                float cy = checkBounds.getCentreY();
                juce::Path checkPath;
                checkPath.startNewSubPath(cx - 4, cy);
                checkPath.lineTo(cx - 1, cy + 3);
                checkPath.lineTo(cx + 4, cy - 2);
                g.strokePath(checkPath, juce::PathStrokeType(1.5f));
            }

            contentBounds.removeFromLeft(4);
        }

        g.setColour((isSelected ? theme.controlActive : theme.textPrimary)
            .withAlpha(alpha * opacity));
            
        static const juce::Font itemFont(juce::FontOptions().withHeight(13.0f));
        g.setFont(itemFont);

        if (item.description.isNotEmpty())
        {
            auto labelBounds = contentBounds.removeFromTop(ITEM_HEIGHT / 2 + 2);
            g.drawText(item.label, labelBounds, juce::Justification::centredLeft);

            g.setColour(theme.textSecondary.withAlpha(alpha * opacity * 0.8f));
            static const juce::Font descFont(juce::FontOptions().withHeight(11.0f));
            g.setFont(descFont);
            g.drawText(item.description, contentBounds, juce::Justification::centredLeft);
        }
        else
        {
            g.drawText(item.label, contentBounds, juce::Justification::centredLeft);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        int index = e.y / ITEM_HEIGHT;
        if (index >= 0 && index < static_cast<int>(owner_.filteredIndices_.size()))
        {
            int itemIndex = owner_.filteredIndices_[static_cast<size_t>(index)];
            if (owner_.hoveredIndex_ != itemIndex)
            {
                owner_.hoveredIndex_ = itemIndex;
                repaint();
            }
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (owner_.hoveredIndex_ != -1)
        {
            owner_.hoveredIndex_ = -1;
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int index = e.y / ITEM_HEIGHT;
        if (index >= 0 && index < static_cast<int>(owner_.filteredIndices_.size()))
        {
            int itemIndex = owner_.filteredIndices_[static_cast<size_t>(index)];
            const auto& item = owner_.items_[static_cast<size_t>(itemIndex)];

            if (!item.isSeparator && item.enabled)
            {
                if (owner_.onItemClicked)
                    owner_.onItemClicked(itemIndex);

                if (!owner_.multiSelect_)
                    owner_.dismiss();
            }
        }
    }

private:
    OscilDropdownPopup& owner_;
};

OscilDropdownPopup::OscilDropdownPopup(IThemeService& themeService)
    : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
    getProperties().set("isOscilPopup", true);

    showProgress_ = 0.0f;

    listComponent_ = std::make_unique<ItemList>(*this);
    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setViewedComponent(listComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    viewport_->setScrollBarThickness(6);
    addAndMakeVisible(*viewport_);
}

OscilDropdownPopup::~OscilDropdownPopup()
{
    if (searchField_)
        searchField_->onTextChange = nullptr;
    showAnimator_.reset();
}

void OscilDropdownPopup::setItems(const std::vector<DropdownItem>& items)
{
    items_ = items;
    updateFilteredItems();
}

void OscilDropdownPopup::setSelectedIndices(const std::set<int>& indices)
{
    selectedIndices_ = indices;
    repaint();
}

class SearchField : public juce::TextEditor
{
public:
    SearchField(OscilDropdownPopup& owner) : owner_(owner) {}
    
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::upKey || 
            key == juce::KeyPress::downKey ||
            key == juce::KeyPress::escapeKey) 
        {
            return owner_.keyPressed(key);
        }
        
        if (key == juce::KeyPress::returnKey)
        {
            if (owner_.keyPressed(key)) return true;
        }
        
        return juce::TextEditor::keyPressed(key);
    }
private:
    OscilDropdownPopup& owner_;
};

void OscilDropdownPopup::setSearchable(bool searchable)
{
    searchable_ = searchable;

    if (searchable && !searchField_)
    {
        searchField_ = std::make_unique<SearchField>(*this);
        searchField_->setMultiLine(false);
        searchField_->setTextToShowWhenEmpty("Search...", getTheme().textSecondary);
        searchField_->onTextChange = [this]() {
            searchText_ = searchField_->getText();
            updateFilteredItems();
            repaint();
        };
        addAndMakeVisible(*searchField_);
    }
    else if (!searchable && searchField_)
    {
        searchField_.reset();
    }
}

void OscilDropdownPopup::parentHierarchyChanged()
{
    ThemedComponent::parentHierarchyChanged();
    if (auto* service = findAnimationService(this))
    {
        animService_ = service;
    }
}

void OscilDropdownPopup::show(juce::Component* parent, juce::Rectangle<int> buttonBounds)
{
    if (!parent) return;

    // Try to get animation service
    if (auto* editor = dynamic_cast<OscilPluginEditor*>(parent->getTopLevelComponent()))
        animService_ = &editor->getAnimationService();

    updateFilteredItems();

    int contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
    int maxViewportHeight = MAX_VISIBLE_ITEMS * ITEM_HEIGHT;
    int viewportHeight = std::min(contentHeight, maxViewportHeight);
    
    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int totalHeight = viewportHeight + searchHeight + POPUP_PADDING * 2;
    int width = buttonBounds.getWidth();

    auto screenBounds = parent->getScreenBounds();
    auto* display = juce::Desktop::getInstance().getDisplays()
        .getDisplayForPoint(screenBounds.getCentre());
    if (display == nullptr)
        display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (display == nullptr)
        return;
    auto displayBounds = display->userArea;

    int x = screenBounds.getX() + buttonBounds.getX();
    int y = screenBounds.getY() + buttonBounds.getBottom();

    if (y + totalHeight > displayBounds.getBottom())
        y = screenBounds.getY() + buttonBounds.getY() - totalHeight;

    setBounds(x, y, width, totalHeight);
    addToDesktop(juce::ComponentPeer::windowIsTemporary |
                 juce::ComponentPeer::windowHasDropShadow);

    setVisible(true);
    
    if (listComponent_)
    {
        int listWidth = juce::jmax(1, width - POPUP_PADDING * 2);
        listComponent_->setSize(listWidth, contentHeight);
    }
        
    resized();
    grabKeyboardFocus();
    
    if (!selectedIndices_.empty())
    {
         for (size_t i = 0; i < filteredIndices_.size(); ++i)
         {
             if (selectedIndices_.count(filteredIndices_[i]))
             {
                 focusedIndex_ = static_cast<int>(i);
                 ensureItemVisible(focusedIndex_);
                 break;
             }
         }
    }

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        showProgress_ = 0.0f;
        auto animator = animService_->createValueAnimation(
            AnimationPresets::EXPAND_DURATION_MS,
            [safeThis = juce::Component::SafePointer<OscilDropdownPopup>(this)](float progress) {
                if (auto* self = safeThis.getComponent())
                {
                    self->showProgress_ = progress;
                    if (self->listComponent_) self->listComponent_->repaint();
                    self->repaint();
                }
            },
            juce::Easings::createEaseOut());

        showAnimator_.set(std::move(animator));
        showAnimator_.start();
    }
    else
    {
        showProgress_ = 1.0f;
    }

    if (searchField_)
        searchField_->grabKeyboardFocus();
}

void OscilDropdownPopup::dismiss()
{
    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startValue = showProgress_;
        auto animator = animService_->createValueAnimation(
            AnimationPresets::EXPAND_DURATION_MS,
            [safeThis = juce::Component::SafePointer<OscilDropdownPopup>(this), startValue](float progress) {
                if (auto* self = safeThis.getComponent())
                {
                    self->showProgress_ = startValue * (1.0f - progress);
                    if (self->listComponent_) self->listComponent_->repaint();
                    self->repaint();
                }
            },
            juce::Easings::createEaseOut(),
            [safeThis = juce::Component::SafePointer<OscilDropdownPopup>(this)]() {
                if (auto* self = safeThis.getComponent())
                {
                    self->setVisible(false);
                    if (self->onDismiss) self->onDismiss();
                }
            });

        showAnimator_.set(std::move(animator));
        showAnimator_.start();
    }
    else
    {
        setVisible(false);
        if (onDismiss) onDismiss();
    }
}

void OscilDropdownPopup::updateFilteredItems()
{
    filteredIndices_.clear();

    for (size_t i = 0; i < items_.size(); ++i)
    {
        const auto& item = items_[i];

        if (item.isSeparator)
        {
            filteredIndices_.push_back(static_cast<int>(i));
            continue;
        }

        if (searchText_.isEmpty() ||
            item.label.containsIgnoreCase(searchText_) ||
            item.description.containsIgnoreCase(searchText_))
        {
            filteredIndices_.push_back(static_cast<int>(i));
        }
    }
    
    if (listComponent_)
    {
        int contentHeight = std::max(ITEM_HEIGHT, static_cast<int>(filteredIndices_.size() * ITEM_HEIGHT));
        listComponent_->setSize(listComponent_->getWidth(), contentHeight);
    }
    
    if (focusedIndex_ >= static_cast<int>(filteredIndices_.size()))
        focusedIndex_ = static_cast<int>(filteredIndices_.size()) - 1;
    if (focusedIndex_ < 0 && !filteredIndices_.empty())
        focusedIndex_ = 0;
}

void OscilDropdownPopup::paint(juce::Graphics& g)
{
    float alpha = showProgress_;
    if (alpha < 0.01f) return;

    auto bounds = getLocalBounds().toFloat();

    g.setColour(getTheme().backgroundPrimary.withAlpha(alpha * 0.98f));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_MD);

    g.setColour(getTheme().controlBorder.withAlpha(alpha * 0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_MD, 1.0f);
}

void OscilDropdownPopup::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    int searchHeight = searchable_ ? SEARCH_HEIGHT : 0;
    int y = POPUP_PADDING;
    
    if (searchField_)
    {
        searchField_->setBounds(POPUP_PADDING, POPUP_PADDING,
            getWidth() - POPUP_PADDING * 2, SEARCH_HEIGHT - POPUP_PADDING);
        y += searchHeight;
    }
    
    if (viewport_)
    {
        viewport_->setBounds(POPUP_PADDING, y, 
                             getWidth() - POPUP_PADDING * 2, 
                             getHeight() - y - POPUP_PADDING);
        
        if (listComponent_)
             listComponent_->setSize(viewport_->getWidth(), listComponent_->getHeight());
    }
}

void OscilDropdownPopup::mouseDown(const juce::MouseEvent&)
{
    dismiss();
}

bool OscilDropdownPopup::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }

    if (key == juce::KeyPress::upKey)
    {
        focusedIndex_ = std::max(0, focusedIndex_ - 1);
        ensureItemVisible(focusedIndex_);
        if (listComponent_) listComponent_->repaint();
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        focusedIndex_ = std::min(static_cast<int>(filteredIndices_.size()) - 1,
                                  focusedIndex_ + 1);
        ensureItemVisible(focusedIndex_);
        if (listComponent_) listComponent_->repaint();
        return true;
    }

    if (key == juce::KeyPress::returnKey && focusedIndex_ >= 0)
    {
        if (focusedIndex_ < static_cast<int>(filteredIndices_.size()))
        {
            int itemIndex = filteredIndices_[static_cast<size_t>(focusedIndex_)];
            const auto& item = items_[static_cast<size_t>(itemIndex)];

            if (!item.isSeparator && item.enabled && onItemClicked)
            {
                onItemClicked(itemIndex);
                if (!multiSelect_)
                    dismiss();
            }
        }
        return true;
    }

    return false;
}

void OscilDropdownPopup::focusLost(FocusChangeType)
{
    auto* focusedComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusedComp != this && !isParentOf(focusedComp))
        dismiss();
}

void OscilDropdownPopup::ensureItemVisible(int index)
{
    if (!viewport_ || index < 0) return;
    
    int itemY = index * ITEM_HEIGHT;
    int itemBottom = itemY + ITEM_HEIGHT;
    
    int viewY = viewport_->getViewPositionY();
    int viewH = viewport_->getViewHeight();
    int viewBottom = viewY + viewH;
    
    if (itemY < viewY)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemY);
    else if (itemBottom > viewBottom)
        viewport_->setViewPosition(viewport_->getViewPositionX(), itemBottom - viewH);
}

int OscilDropdownPopup::getItemAtPosition(juce::Point<int>) const
{
    return -1;
}

//==============================================================================
// OscilDropdown Implementation
//==============================================================================

OscilDropdown::OscilDropdown(IThemeService& themeService)
    : ThemedComponent(themeService)
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

OscilDropdown::OscilDropdown(IThemeService& themeService, const juce::String& placeholder)
    : OscilDropdown(themeService)
{
    placeholder_ = placeholder;
}

OscilDropdown::OscilDropdown(IThemeService& themeService, const juce::String& placeholder, const juce::String& testId)
    : OscilDropdown(themeService)
{
    placeholder_ = placeholder;
    setTestId(testId);
}

void OscilDropdown::registerTestId()
{
    OSCIL_REGISTER_TEST_ID(testId_);
}

OscilDropdown::~OscilDropdown()
{
    hoverAnimator_.reset();
    chevronAnimator_.reset();
}

void OscilDropdown::addItem(const juce::String& label, const juce::String& id)
{
    DropdownItem item;
    item.id = id.isEmpty() ? label : id;
    item.label = label;
    items_.push_back(item);
}

void OscilDropdown::addItem(const DropdownItem& item)
{
    items_.push_back(item);
}

void OscilDropdown::addItems(const std::vector<juce::String>& labels)
{
    for (const auto& label : labels)
        addItem(label);
}

void OscilDropdown::addItems(const std::vector<DropdownItem>& items)
{
    for (const auto& item : items)
        addItem(item);
}

void OscilDropdown::clearItems()
{
    items_.clear();
    selectedIndices_.clear();
    updateDisplayText();
    repaint();
}

void OscilDropdown::setSelectedIndex(int index, bool notify)
{
    std::set<int> newSelection;
    if (index >= 0 && index < static_cast<int>(items_.size()))
        newSelection.insert(index);

    setSelectedIndices(newSelection, notify);
}

void OscilDropdown::setSelectedId(const juce::String& id, bool notify)
{
    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i].id == id)
        {
            setSelectedIndex(static_cast<int>(i), notify);
            return;
        }
    }
    setSelectedIndex(-1, notify);
}

int OscilDropdown::getSelectedIndex() const
{
    return selectedIndices_.empty() ? -1 : *selectedIndices_.begin();
}

juce::String OscilDropdown::getSelectedId() const
{
    int index = getSelectedIndex();
    return (index >= 0 && index < static_cast<int>(items_.size()))
        ? items_[static_cast<size_t>(index)].id : juce::String();
}

juce::String OscilDropdown::getSelectedLabel() const
{
    int index = getSelectedIndex();
    return (index >= 0 && index < static_cast<int>(items_.size()))
        ? items_[static_cast<size_t>(index)].label : juce::String();
}

void OscilDropdown::setSelectedIndices(const std::set<int>& indices, bool notify)
{
    if (selectedIndices_ == indices)
        return;

    selectedIndices_ = indices;
    updateDisplayText();
    repaint();

    if (notify)
    {
        if (!multiSelect_ && onSelectionChanged)
            onSelectionChanged(getSelectedIndex());

        if (multiSelect_ && onMultiSelectionChanged)
            onMultiSelectionChanged(selectedIndices_);

        if (onSelectionChangedId)
            onSelectionChangedId(getSelectedId());
    }
}

std::vector<juce::String> OscilDropdown::getSelectedIds() const
{
    std::vector<juce::String> result;
    for (int index : selectedIndices_)
        if (index >= 0 && index < static_cast<int>(items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].id);
    return result;
}

std::vector<juce::String> OscilDropdown::getSelectedLabels() const
{
    std::vector<juce::String> result;
    for (int index : selectedIndices_)
        if (index >= 0 && index < static_cast<int>(items_.size()))
            result.push_back(items_[static_cast<size_t>(index)].label);
    return result;
}

void OscilDropdown::setPlaceholder(const juce::String& placeholder)
{
    placeholder_ = placeholder;
    updateDisplayText();
    repaint();
}

void OscilDropdown::setMultiSelect(bool multiSelect)
{
    if (multiSelect_ != multiSelect)
    {
        multiSelect_ = multiSelect;
        if (!multiSelect && selectedIndices_.size() > 1)
        {
            int first = *selectedIndices_.begin();
            selectedIndices_.clear();
            selectedIndices_.insert(first);
            updateDisplayText();
        }
        repaint();
    }
}

void OscilDropdown::setSearchable(bool searchable)
{
    searchable_ = searchable;
}

void OscilDropdown::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        setMouseCursor(enabled ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OscilDropdown::showPopup()
{
    if (!enabled_ || items_.empty())
        return;

    // Try to get animation service
    if (animService_ == nullptr)
    {
        if (auto* editor = dynamic_cast<OscilPluginEditor*>(getTopLevelComponent()))
            animService_ = &editor->getAnimationService();
    }

    popupVisible_ = true;

    popup_ = std::make_unique<OscilDropdownPopup>(getThemeService());
    popup_->setItems(items_);
    popup_->setSelectedIndices(selectedIndices_);
    popup_->setMultiSelect(multiSelect_);
    popup_->setSearchable(searchable_);

    popup_->onItemClicked = [this](int index) {
        handleItemClicked(index);
    };

    popup_->onDismiss = [this]() {
        popupVisible_ = false;
        popup_.reset();
        
        // Animate chevron back
        if (animService_)
        {
            float startValue = chevronProgress_;
            auto safeThis = juce::Component::SafePointer<OscilDropdown>(this);
            auto animator = animService_->createValueAnimation(
                AnimationPresets::SNAPPY_DURATION_MS,
                [safeThis, startValue](float progress) {
                    if (safeThis) {
                        safeThis->chevronProgress_ = startValue * (1.0f - progress);
                        safeThis->repaint();
                    }
                },
                juce::Easings::createEaseOut());

            chevronAnimator_.set(std::move(animator));
            chevronAnimator_.start();
        }
        else
        {
            chevronProgress_ = 0.0f;
        }
        repaint();
    };

    popup_->show(this, getLocalBounds());

    // Animate chevron open
    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startValue = chevronProgress_;
        auto safeThis = juce::Component::SafePointer<OscilDropdown>(this);
        auto animator = animService_->createValueAnimation(
            AnimationPresets::SNAPPY_DURATION_MS,
            [safeThis, startValue](float progress) {
                if (safeThis) {
                    safeThis->chevronProgress_ = startValue + (1.0f - startValue) * progress;
                    safeThis->repaint();
                }
            },
            juce::Easings::createEaseOut());

        chevronAnimator_.set(std::move(animator));
        chevronAnimator_.start();
    }
    else
    {
        chevronProgress_ = 1.0f;
    }

    repaint();
}

void OscilDropdown::hidePopup()
{
    if (popup_)
        popup_->dismiss();
}

void OscilDropdown::handleItemClicked(int index)
{
    if (multiSelect_)
    {
        if (selectedIndices_.count(index))
            selectedIndices_.erase(index);
        else
            selectedIndices_.insert(index);

        updateDisplayText();

        if (popup_)
            popup_->setSelectedIndices(selectedIndices_);

        if (onMultiSelectionChanged)
            onMultiSelectionChanged(selectedIndices_);
    }
    else
    {
        setSelectedIndex(index);
    }

    repaint();
}

void OscilDropdown::updateDisplayText()
{
    if (selectedIndices_.empty())
    {
        displayText_ = placeholder_;
    }
    else if (selectedIndices_.size() == 1)
    {
        int index = *selectedIndices_.begin();
        if (index >= 0 && index < static_cast<int>(items_.size()))
            displayText_ = items_[static_cast<size_t>(index)].label;
        else
            displayText_ = placeholder_;
    }
    else
    {
        displayText_ = juce::String(selectedIndices_.size()) + " selected";
    }
}

int OscilDropdown::getPreferredWidth() const
{
    int maxWidth = 100;

    for (const auto& item : items_)
    {
        auto font = juce::Font(juce::FontOptions().withHeight(13.0f));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, item.label, 0, 0);
        int width = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth());
        maxWidth = std::max(maxWidth, width);
    }

    return maxWidth + PADDING_H * 2 + CHEVRON_SIZE + 8;
}

int OscilDropdown::getPreferredHeight() const
{
    return ComponentLayout::INPUT_HEIGHT;
}

void OscilDropdown::paint(juce::Graphics& g)
{
    // Guard against zero-size painting
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    auto bounds = getLocalBounds().toFloat();
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float hoverAmount = hoverProgress_;

    auto bgColour = getTheme().backgroundSecondary;
    if (hoverAmount > 0.01f)
        bgColour = bgColour.brighter(0.05f * hoverAmount);

    g.setColour(bgColour.withAlpha(opacity));
    g.fillRoundedRectangle(bounds, ComponentLayout::RADIUS_SM);

    auto borderColour = popupVisible_ ? getTheme().controlActive
                      : hasFocus_ ? getTheme().controlActive
                      : getTheme().controlBorder;

    g.setColour(borderColour.withAlpha(opacity));
    g.drawRoundedRectangle(bounds.reduced(0.5f), ComponentLayout::RADIUS_SM, 1.0f);

    if (hasFocus_ && enabled_)
    {
        g.setColour(getTheme().controlActive.withAlpha(ComponentLayout::FOCUS_RING_ALPHA));
        g.drawRoundedRectangle(
            bounds.expanded(ComponentLayout::FOCUS_RING_OFFSET),
            ComponentLayout::RADIUS_SM + ComponentLayout::FOCUS_RING_OFFSET,
            ComponentLayout::FOCUS_RING_WIDTH
        );
    }

    auto textBounds = bounds.reduced(PADDING_H, 0);
    textBounds.removeFromRight(CHEVRON_SIZE + 8);

    bool isPlaceholder = selectedIndices_.empty();
    g.setColour((isPlaceholder ? getTheme().textSecondary : getTheme().textPrimary)
        .withAlpha(opacity));
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
    g.drawText(displayText_, textBounds, juce::Justification::centredLeft);

    auto chevronBounds = bounds.removeFromRight(CHEVRON_SIZE + PADDING_H)
        .withSizeKeepingCentre(CHEVRON_SIZE, CHEVRON_SIZE);

    paintChevron(g, chevronBounds);
}

void OscilDropdown::paintChevron(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float opacity = enabled_ ? 1.0f : ComponentLayout::DISABLED_OPACITY;
    float rotation = chevronProgress_ * juce::MathConstants<float>::pi;

    g.setColour(getTheme().textSecondary.withAlpha(opacity));

    juce::Path chevron;
    float size = bounds.getWidth() * 0.4f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    chevron.startNewSubPath(cx - size, cy - size * 0.3f);
    chevron.lineTo(cx, cy + size * 0.3f);
    chevron.lineTo(cx + size, cy - size * 0.3f);

    chevron.applyTransform(
        juce::AffineTransform::rotation(rotation, cx, cy)
    );

    g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void OscilDropdown::resized()
{
    // Guard against zero or negative dimensions
    if (getWidth() <= 0 || getHeight() <= 0)
    {
        juce::Logger::writeToLog("OscilDropdown::resized() - Invalid dimensions: "
            + juce::String(getWidth()) + "x" + juce::String(getHeight()));
        return;
    }
}

void OscilDropdown::mouseDown(const juce::MouseEvent&)
{
    if (!enabled_) return;

    if (popupVisible_)
        hidePopup();
    else
        showPopup();
}

void OscilDropdown::mouseEnter(const juce::MouseEvent&)
{
    if (!enabled_) return;

    isHovered_ = true;

    // Get animation service if needed
    if (animService_ == nullptr)
    {
        if (auto* editor = dynamic_cast<OscilPluginEditor*>(getTopLevelComponent()))
            animService_ = &editor->getAnimationService();
    }

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startValue = hoverProgress_;
        auto safeThis = juce::Component::SafePointer<OscilDropdown>(this);
        auto animator = animService_->createValueAnimation(
            AnimationPresets::HOVER_DURATION_MS,
            [safeThis, startValue](float progress) {
                if (safeThis) {
                    safeThis->hoverProgress_ = startValue + (1.0f - startValue) * progress;
                    safeThis->repaint();
                }
            },
            juce::Easings::createEase());

        hoverAnimator_.set(std::move(animator));
        hoverAnimator_.start();
    }
    else
    {
        hoverProgress_ = 1.0f;
        repaint();
    }
}

void OscilDropdown::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;

    if (AnimationSettings::shouldUseSpringAnimations() && animService_)
    {
        float startValue = hoverProgress_;
        auto safeThis = juce::Component::SafePointer<OscilDropdown>(this);
        auto animator = animService_->createValueAnimation(
            AnimationPresets::HOVER_DURATION_MS,
            [safeThis, startValue](float progress) {
                if (safeThis) {
                    safeThis->hoverProgress_ = startValue * (1.0f - progress);
                    safeThis->repaint();
                }
            },
            juce::Easings::createEase());

        hoverAnimator_.set(std::move(animator));
        hoverAnimator_.start();
    }
    else
    {
        hoverProgress_ = 0.0f;
        repaint();
    }
}

bool OscilDropdown::keyPressed(const juce::KeyPress& key)
{
    if (!enabled_) return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        if (popupVisible_)
            hidePopup();
        else
            showPopup();
        return true;
    }

    if (key == juce::KeyPress::escapeKey && popupVisible_)
    {
        hidePopup();
        return true;
    }

    if (!popupVisible_ && !items_.empty())
    {
        int current = getSelectedIndex();

        if (key == juce::KeyPress::upKey)
        {
            setSelectedIndex(std::max(0, current - 1));
            return true;
        }

        if (key == juce::KeyPress::downKey)
        {
            setSelectedIndex(std::min(static_cast<int>(items_.size()) - 1,
                                       current + 1));
            return true;
        }
    }

    return false;
}

void OscilDropdown::focusGained(FocusChangeType)
{
    hasFocus_ = true;
    repaint();
}

void OscilDropdown::focusLost(FocusChangeType)
{
    hasFocus_ = false;
    repaint();
}

// Accessibility handler
class OscilDropdownAccessibilityHandler : public juce::AccessibilityHandler
{
public:
    explicit OscilDropdownAccessibilityHandler(OscilDropdown& dropdown)
        : juce::AccessibilityHandler(dropdown, juce::AccessibilityRole::comboBox,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press, [&dropdown] { if (dropdown.isEnabled()) dropdown.showPopup(); })
                .addAction(juce::AccessibilityActionType::showMenu, [&dropdown] { if (dropdown.isEnabled()) dropdown.showPopup(); })
        )
        , dropdown_(dropdown)
    {
    }

    juce::String getTitle() const override
    {
        return dropdown_.getPlaceholder();
    }

    juce::String getDescription() const override
    {
        juce::String selection = dropdown_.getSelectedLabel();
        if (selection.isEmpty())
            return "No selection";

        int numItems = dropdown_.getNumItems();
        return "Selected: " + selection + " (" + juce::String(numItems) + " options available)";
    }

    juce::String getHelp() const override
    {
        return "Press Space or Enter to open dropdown. Use arrow keys to navigate.";
    }

private:
    OscilDropdown& dropdown_;
};

std::unique_ptr<juce::AccessibilityHandler> OscilDropdown::createAccessibilityHandler()
{
    return std::make_unique<OscilDropdownAccessibilityHandler>(*this);
}

} // namespace oscil
