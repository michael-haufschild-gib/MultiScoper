/*
    Oscil - Accordion Component Implementation
    (OscilAccordionSection is in OscilAccordionSection.cpp)
*/

#include "ui/components/OscilAccordion.h"

namespace oscil
{
OscilAccordion::OscilAccordion(IThemeService& themeService)
    : ThemedComponent(themeService)
{
}

OscilAccordion::~OscilAccordion()
{
}

OscilAccordionSection* OscilAccordion::addSection(const juce::String& title)
{
    auto section = std::make_unique<OscilAccordionSection>(getThemeService(), title);
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
    section->onHeightChanged = [this]() {
        updateContentHeight();
    };

    addAndMakeVisible(*section);
    sections_.push_back(std::move(section));

    resized();
    return sectionPtr;
}

OscilAccordionSection* OscilAccordion::addSection(const juce::String& title, juce::Component* content)
{
    auto* section = addSection(title);
    section->setContent(content);
    return section;
}

void OscilAccordion::removeSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
    {
        sections_.erase(sections_.begin() + index);
        resized();
    }
}

void OscilAccordion::clearSections()
{
    sections_.clear();
    resized();
}

OscilAccordionSection* OscilAccordion::getSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        return sections_[static_cast<size_t>(index)].get();
    return nullptr;
}

void OscilAccordion::setAllowMultiExpand(bool allow)
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

void OscilAccordion::expandSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        sections_[static_cast<size_t>(index)]->setExpanded(true);
}

void OscilAccordion::collapseSection(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < sections_.size())
        sections_[static_cast<size_t>(index)]->setExpanded(false);
}

void OscilAccordion::expandAll()
{
    if (!allowMultiExpand_)
        return;

    for (auto& section : sections_)
        section->setExpanded(true);
}

void OscilAccordion::collapseAll()
{
    for (auto& section : sections_)
        section->setExpanded(false);
}

void OscilAccordion::setSpacing(int spacing)
{
    if (spacing_ != spacing)
    {
        spacing_ = spacing;
        resized();
    }
}

int OscilAccordion::getPreferredHeight() const
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

void OscilAccordion::updateContentHeight()
{
    int preferredHeight = getPreferredHeight();
    if (getHeight() != preferredHeight)
        setSize(getWidth(), preferredHeight);
    
    layoutSections();
}

void OscilAccordion::resized()
{
    layoutSections();
}

void OscilAccordion::handleSectionExpanded(int index, bool expanded)
{
    // If not allowing multi-expand and a section was just expanded,
    // collapse all other sections
    if (!allowMultiExpand_ && expanded)
    {
        for (size_t i = 0; i < sections_.size(); ++i)
        {
            if (static_cast<int>(i) != index && sections_[i]->isExpanded())
                sections_[i]->setExpanded(false);
        }
    }
}

void OscilAccordion::layoutSections()
{
    auto bounds = getLocalBounds();
    int y = 0;

    for (size_t i = 0; i < sections_.size(); ++i)
    {
        int height = sections_[i]->getPreferredHeight();
        sections_[i]->setBounds(0, y, bounds.getWidth(), height);

        y += height + spacing_;
    }
}

} // namespace oscil
