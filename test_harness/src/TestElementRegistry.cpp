/*
    Oscil Test Harness - Element Registry Implementation
*/

#include "TestElementRegistry.h"

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
        if (it->second.getComponent() == component)
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
    {
        auto* comp = it->second.getComponent();
        if (comp == nullptr)
        {
            // SafePointer detected destruction — clean up the stale entry
            elements_.erase(it);
            return nullptr;
        }
        return comp;
    }
    return nullptr;
}

juce::Component* TestElementRegistry::findValidElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
    {
        if (testId.contains("item_0_delete"))
            juce::Logger::writeToLog("[Registry] findValidElement(" + testId + "): NOT FOUND in map (size=" + juce::String(static_cast<int>(elements_.size())) + ")");
        return nullptr;
    }

    auto* comp = it->second.getComponent();

    // SafePointer returns nullptr if the component was destroyed
    if (comp == nullptr)
    {
        if (testId.contains("item_0_delete"))
            juce::Logger::writeToLog("[Registry] findValidElement(" + testId + "): SafePointer null (destroyed)");
        elements_.erase(it);
        return nullptr;
    }

    // Top-level windows have no parent but are valid. Components inside
    // windows always have a parent. If a component has no parent and is
    // not currently on screen, it's likely stale.
    if (comp->getParentComponent() == nullptr && !comp->isOnDesktop())
    {
        if (testId.contains("item_0_delete"))
            juce::Logger::writeToLog("[Registry] findValidElement(" + testId + "): no parent, erasing as stale");
        elements_.erase(it);
        return nullptr;
    }

    return comp;
}

std::map<juce::String, juce::Component*> TestElementRegistry::getAllElements()
{
    std::scoped_lock lock(mutex_);
    std::map<juce::String, juce::Component*> result;
    for (auto it = elements_.begin(); it != elements_.end(); )
    {
        auto* comp = it->second.getComponent();
        if (comp != nullptr)
        {
            result[it->first] = comp;
            ++it;
        }
        else
        {
            // Clean up entries where SafePointer detected destruction
            it = elements_.erase(it);
        }
    }
    return result;
}

std::vector<juce::String> TestElementRegistry::getAllTestIds()
{
    std::scoped_lock lock(mutex_);
    std::vector<juce::String> ids;
    ids.reserve(elements_.size());
    for (const auto& [id, _] : elements_)
        ids.push_back(id);
    return ids;
}

void TestElementRegistry::clear()
{
    std::scoped_lock lock(mutex_);
    elements_.clear();
}

bool TestElementRegistry::hasElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
        return false;
    if (it->second.getComponent() == nullptr)
    {
        elements_.erase(it);
        return false;
    }
    return true;
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
