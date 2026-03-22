/*
    Oscil Test Harness - Element Registry Implementation
*/

#include "TestElementRegistry.h"

#include <iostream>

namespace oscil::test
{

TestElementRegistry& TestElementRegistry::getInstance()
{
    static TestElementRegistry instance;
    return instance;
}

void TestElementRegistry::registerElement(const juce::String& testId, juce::Component* component)
{
    std::scoped_lock lock(mutex_);
    elements_[testId] = component;
}

void TestElementRegistry::unregisterElement(juce::Component* component)
{
    std::scoped_lock lock(mutex_);
    for (auto it = elements_.begin(); it != elements_.end(); )
    {
        if (it->second == component)
            it = elements_.erase(it);
        else
            ++it;
    }
}

void TestElementRegistry::unregisterElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    elements_.erase(testId);
}

juce::Component* TestElementRegistry::findElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it != elements_.end())
        return it->second;
    return nullptr;
}

juce::Component* TestElementRegistry::findValidElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
        return nullptr;

    auto* comp = it->second;

    // Check the component is still part of a live hierarchy.
    // A component that has been removed from its parent (during editor
    // teardown) but not yet unregistered will have no parent and no peer.
    if (comp == nullptr)
    {
        elements_.erase(it);
        return nullptr;
    }

    // Top-level windows have no parent but are valid. Components inside
    // windows always have a parent. If a component has no parent and is
    // not currently on screen, it's likely stale.
    if (comp->getParentComponent() == nullptr && !comp->isOnDesktop())
    {
        // Stale entry — remove it
        elements_.erase(it);
        return nullptr;
    }

    return comp;
}

std::map<juce::String, juce::Component*> TestElementRegistry::getAllElements()
{
    std::scoped_lock lock(mutex_);
    return elements_;
}

void TestElementRegistry::clear()
{
    std::scoped_lock lock(mutex_);
    elements_.clear();
}

bool TestElementRegistry::hasElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    return elements_.find(testId) != elements_.end();
}

juce::Rectangle<int> TestElementRegistry::getElementScreenBounds(const juce::String& testId)
{
    if (auto* component = findValidElement(testId))
    {
        return component->getScreenBounds();
    }
    return {};
}

juce::Rectangle<int> TestElementRegistry::getElementBounds(const juce::String& testId)
{
    if (auto* component = findValidElement(testId))
    {
        // Get bounds relative to top-level parent
        auto bounds = component->getBounds();
        auto* parent = component->getParentComponent();

        while (parent != nullptr && parent->getParentComponent() != nullptr)
        {
            bounds = bounds.translated(parent->getX(), parent->getY());
            parent = parent->getParentComponent();
        }

        return bounds;
    }
    return {};
}

} // namespace oscil::test
