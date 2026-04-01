/*
    Oscil Test Harness - Element Registry Implementation
*/

#include "TestElementRegistry.h"

#ifdef TEST_HARNESS
    #include "TestDAW.h"
#endif

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
    auto& entries = elements_[testId];

    // Avoid duplicate registrations of the same component
    for (auto& sp : entries)
    {
        if (sp.getComponent() == component)
            return;
    }

    entries.emplace_back(component);
}

void TestElementRegistry::unregisterElement(juce::Component* component)
{
    std::scoped_lock lock(mutex_);
    for (auto it = elements_.begin(); it != elements_.end();)
    {
        auto& entries = it->second;
        std::erase_if(entries,
                      [component](auto& sp) { return sp.getComponent() == component || sp.getComponent() == nullptr; });

        if (entries.empty())
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

void TestElementRegistry::unregisterElement(const juce::String& testId, juce::Component* component)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
        return;

    auto& entries = it->second;
    std::erase_if(entries,
                  [component](auto& sp) { return sp.getComponent() == component || sp.getComponent() == nullptr; });

    if (entries.empty())
        elements_.erase(it);
}

void TestElementRegistry::pruneStaleEntries(std::vector<juce::Component::SafePointer<juce::Component>>& entries)
{
    std::erase_if(entries, [](auto& sp) { return sp.getComponent() == nullptr; });
}

juce::Component*
TestElementRegistry::findFirstValid(std::vector<juce::Component::SafePointer<juce::Component>>& entries)
{
    pruneStaleEntries(entries);
    for (auto& sp : entries)
    {
        auto* comp = sp.getComponent();
        if (comp != nullptr)
            return comp;
    }
    return nullptr;
}

bool TestElementRegistry::isComponentInWindow(juce::Component* comp, juce::Component* window)
{
    if (comp == nullptr || window == nullptr)
        return false;

    auto* current = comp;
    while (current != nullptr)
    {
        if (current == window)
            return true;
        // Check if current's top-level component is the window's content
        if (current->getParentComponent() == nullptr)
        {
            // Reached a root — check if it's inside the window
            return current == window;
        }
        current = current->getParentComponent();
    }
    return false;
}

juce::Component* TestElementRegistry::findElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
        return nullptr;

    return findFirstValid(it->second);
}

juce::Component* TestElementRegistry::findValidElement(const juce::String& testId)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
    {
        if (testId.contains("item_0_delete"))
            juce::Logger::writeToLog("[Registry] findValidElement(" + testId + "): NOT FOUND in map (size=" +
                                     juce::String(static_cast<int>(elements_.size())) + ")");
        return nullptr;
    }

    auto& entries = it->second;
    pruneStaleEntries(entries);

    for (auto& sp : entries)
    {
        auto* comp = sp.getComponent();
        if (comp == nullptr)
            continue;

        // Top-level windows have no parent but are valid. Components inside
        // windows always have a parent. If a component has no parent and is
        // not currently on screen, it's likely stale.
        if (comp->getParentComponent() == nullptr && !comp->isOnDesktop())
        {
            if (testId.contains("item_0_delete"))
                juce::Logger::writeToLog("[Registry] findValidElement(" + testId + "): no parent, skipping stale");
            continue;
        }

        return comp;
    }

    if (testId.contains("item_0_delete"))
        juce::Logger::writeToLog("[Registry] findValidElement(" + testId + "): all entries stale");

    // All entries were stale — clean up
    if (entries.empty())
        elements_.erase(it);

    return nullptr;
}

#ifdef TEST_HARNESS

juce::Component* TestElementRegistry::findElementForTrack(const juce::String& testId, int trackIndex, TestDAW& daw)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
        return nullptr;

    auto* track = daw.getTrack(trackIndex);
    if (track == nullptr || track->getEditor() == nullptr)
        return nullptr;

    // The editor's top-level component is the DocumentWindow that contains it
    auto* editorTopLevel = track->getEditor()->getTopLevelComponent();

    auto& entries = it->second;
    pruneStaleEntries(entries);

    for (auto& sp : entries)
    {
        auto* comp = sp.getComponent();
        if (comp == nullptr)
            continue;

        if (comp->getTopLevelComponent() == editorTopLevel)
            return comp;
    }

    return nullptr;
}

juce::Component* TestElementRegistry::findValidElementForTrack(const juce::String& testId, int trackIndex, TestDAW& daw)
{
    std::scoped_lock lock(mutex_);
    auto it = elements_.find(testId);
    if (it == elements_.end())
        return nullptr;

    auto* track = daw.getTrack(trackIndex);
    if (track == nullptr || track->getEditor() == nullptr)
        return nullptr;

    auto* editorTopLevel = track->getEditor()->getTopLevelComponent();

    auto& entries = it->second;
    pruneStaleEntries(entries);

    for (auto& sp : entries)
    {
        auto* comp = sp.getComponent();
        if (comp == nullptr)
            continue;

        // Stale check
        if (comp->getParentComponent() == nullptr && !comp->isOnDesktop())
            continue;

        if (comp->getTopLevelComponent() == editorTopLevel)
            return comp;
    }

    return nullptr;
}

std::map<juce::String, juce::Component*> TestElementRegistry::getAllElements()
{
    std::scoped_lock lock(mutex_);
    std::map<juce::String, juce::Component*> result;
    for (auto it = elements_.begin(); it != elements_.end();)
    {
        pruneStaleEntries(it->second);
        auto* comp = findFirstValid(it->second);
        if (comp != nullptr)
        {
            result[it->first] = comp;
            ++it;
        }
        else
        {
            it = elements_.erase(it);
        }
    }
    return result;
}

std::map<juce::String, juce::Component*> TestElementRegistry::getAllElementsForTrack(int trackIndex, TestDAW& daw)
{
    std::scoped_lock lock(mutex_);
    std::map<juce::String, juce::Component*> result;

    auto* track = daw.getTrack(trackIndex);
    if (track == nullptr || track->getEditor() == nullptr)
        return result;

    auto* editorTopLevel = track->getEditor()->getTopLevelComponent();

    for (auto& [testId, entries] : elements_)
    {
        pruneStaleEntries(entries);
        for (auto& sp : entries)
        {
            auto* comp = sp.getComponent();
            if (comp != nullptr && comp->getTopLevelComponent() == editorTopLevel)
            {
                result[testId] = comp;
                break;
            }
        }
    }

    return result;
}

#endif // TEST_HARNESS

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

    pruneStaleEntries(it->second);
    if (it->second.empty())
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
