/*
    MultiScoper - Accordion Component Implementation
    (MultiScoperAccordionSection is in MultiScoperAccordionSection.cpp)
*/

#include "ui/components/MultiScoperAccordion.h"

#include <utility>

namespace multiscoper
{
MultiScoperAccordion::MultiScoperAccordion(IThemeService& themeService) : ThemedComponent(themeService) {}

MultiScoperAccordion::~MultiScoperAccordion() {}

MultiScoperAccordionSection* MultiScoperAccordion::addSection(const juce::String& title)
{
    auto section = std::make_unique<MultiScoperAccordionSection>(getThemeService(), title);
    auto* sectionPtr = section.get();

    section->onExpandedChanged = [this, sectionPtr](bool expanded) {
        int currentIndex = -1;
        for (size_t i = 0; i < sections_.size(); ++i)
        {
            if (sections_[i].get() == sectionPtr)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        if (currentIndex >= 0)
            handleSectionExpanded(currentIndex, expanded);
    };
    section->onHeightChanged = [this]() { updateContentHeight(); };

    addAndMakeVisible(*section);
    sections_.push_back(std::move(section));

    resized();
    return sectionPtr;
}

MultiScoperAccordionSection* MultiScoperAccordion::addSection(const juce::String& title, juce::Component* content)
{
    auto* section = addSection(title);
    section->setContent(content);
    return section;
}

void MultiScoperAccordion::removeSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
    {
        sections_.erase(sections_.begin() + index);
        resized();
    }
}

void MultiScoperAccordion::clearSections()
{
    sections_.clear();
    resized();
}

MultiScoperAccordionSection* MultiScoperAccordion::getSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        return sections_[static_cast<size_t>(index)].get();
    return nullptr;
}

void MultiScoperAccordion::setAllowMultiExpand(bool allow)
{
    if (allowMultiExpand_ != allow)
    {
        allowMultiExpand_ = allow;

        // If disabling multi-expand and multiple sections are open,
        // keep only the first expanded one
        if (!allowMultiExpand_)
        {
            bool foundExpanded = false;
            for (auto& section : sections_)
            {
                if (section->isExpanded())
                {
                    if (foundExpanded)
                        section->setExpanded(false);
                    else
                        foundExpanded = true;
                }
            }
        }
    }
}

void MultiScoperAccordion::expandSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        sections_[static_cast<size_t>(index)]->setExpanded(true);
}

void MultiScoperAccordion::collapseSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        sections_[static_cast<size_t>(index)]->setExpanded(false);
}

void MultiScoperAccordion::expandAll()
{
    if (!allowMultiExpand_)
        return;

    for (auto& section : sections_)
        section->setExpanded(true);
}

void MultiScoperAccordion::collapseAll()
{
    for (auto& section : sections_)
        section->setExpanded(false);
}

void MultiScoperAccordion::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        resized();
    }
}

int MultiScoperAccordion::getPreferredHeight() const
{
    int totalHeight = 0;

    for (size_t i = 0; i < sections_.size(); ++i)
    {
        totalHeight += sections_[i]->getPreferredHeight();
        if (i > 0)
            totalHeight += spacing_;
    }

    return totalHeight;
}

void MultiScoperAccordion::updateContentHeight()
{
    int const preferredHeight = getPreferredHeight();
    if (getHeight() != preferredHeight)
        setSize(getWidth(), preferredHeight);

    layoutSections();
}

void MultiScoperAccordion::resized() { layoutSections(); }

void MultiScoperAccordion::handleSectionExpanded(int index, bool expanded)
{
    // If not allowing multi-expand and a section was just expanded,
    // collapse all other sections
    if (!allowMultiExpand_ && expanded)
    {
        for (size_t i = 0; i < sections_.size(); ++i)
        {
            if (std::cmp_not_equal(i, index) && sections_[i]->isExpanded())
                sections_[i]->setExpanded(false);
        }
    }
}

void MultiScoperAccordion::layoutSections()
{
    auto bounds = getLocalBounds();
    int y = 0;

    for (const auto& section : sections_)
    {
        int const height = section->getPreferredHeight();
        section->setBounds(0, y, bounds.getWidth(), height);

        y += height + spacing_;
    }
}

} // namespace multiscoper
