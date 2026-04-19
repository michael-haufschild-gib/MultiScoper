/*
    MultiScoper - Visual Preset Manager Implementation
*/

#include "rendering/PresetManager.h"

#include <optional>

namespace multiscoper
{

static const juce::Identifier PRESET_WRAPPER_TYPE("MultiScoperPreset");

std::optional<juce::ValueTree> PresetManager::parsePresetFile(const juce::File& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return std::nullopt;

    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.isValid() || tree.getType() != PRESET_WRAPPER_TYPE)
        return std::nullopt;

    if (!tree.getChildWithName("VisualConfiguration").isValid())
        return std::nullopt;

    return tree;
}

PresetManager::PresetManager()
{
    presetsDir_ = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                      .getChildFile("Application Support")
#endif
                      .getChildFile("MultiScoper")
                      .getChildFile("MultiScoper")
                      .getChildFile("presets");

    presetsDir_.createDirectory();
}

std::vector<PresetInfo> PresetManager::getAvailablePresets() const
{
    std::vector<PresetInfo> presets;

    // Built-in presets first
    auto builtIn = VisualConfiguration::getAvailablePresets();
    auto files = getUserPresetFiles();
    presets.reserve(builtIn.size() + files.size());
    for (const auto& [id, name] : builtIn)
    {
        presets.push_back({.id = id, .displayName = name, .isBuiltIn = true});
    }

    // User presets from disk
    for (const auto& file : files)
    {
        auto tree = parsePresetFile(file);
        if (!tree.has_value())
            continue;

        juce::String const id = tree->getProperty("presetId", "");
        if (id.isEmpty())
            continue;

        juce::String const name = tree->getProperty("displayName", "");
        presets.push_back({.id = id, .displayName = name.isEmpty() ? id : name, .isBuiltIn = false});
    }

    return presets;
}

VisualConfiguration PresetManager::loadPreset(const juce::String& presetId) const
{
    // Check user presets first (allows overriding built-ins)
    auto file = getUserPresetFile(presetId);
    if (file.existsAsFile())
    {
        auto wrapper = parsePresetFile(file);
        if (wrapper.has_value())
        {
            auto configTree = wrapper->getChildWithName("VisualConfiguration");
            if (configTree.isValid())
                return VisualConfiguration::fromValueTree(configTree);
        }
    }

    // Fall back to built-in
    return VisualConfiguration::getPreset(presetId);
}

bool PresetManager::saveUserPreset(const juce::String& name, const VisualConfiguration& config)
{
    if (name.isEmpty())
        return false;

    juce::String const id = generatePresetId(name);

    // Build wrapper tree: { presetId, displayName, VisualConfiguration }
    juce::ValueTree wrapper(PRESET_WRAPPER_TYPE);
    wrapper.setProperty("presetId", id, nullptr);
    wrapper.setProperty("displayName", name, nullptr);
    wrapper.setProperty("version", 1, nullptr);
    wrapper.addChild(config.toValueTree(), -1, nullptr);

    auto xml = wrapper.createXml();
    if (xml == nullptr)
    {
        DBG("PresetManager::saveUserPreset: failed to create XML for: " << name);
        return false;
    }

    auto file = getUserPresetFile(id);
    if (!xml->writeTo(file))
    {
        DBG("PresetManager::saveUserPreset: failed to write: " << file.getFullPathName());
        return false;
    }
    return true;
}

bool PresetManager::deleteUserPreset(const juce::String& presetId)
{
    if (!isUserPreset(presetId))
        return false;

    auto file = getUserPresetFile(presetId);
    return file.deleteFile();
}

bool PresetManager::renameUserPreset(const juce::String& presetId, const juce::String& newName)
{
    auto file = getUserPresetFile(presetId);
    auto wrapper = parsePresetFile(file);
    if (!wrapper.has_value())
        return false;

    juce::String const newId = generatePresetId(newName);
    wrapper->setProperty("presetId", newId, nullptr);
    wrapper->setProperty("displayName", newName, nullptr);

    auto newXml = wrapper->createXml();
    if (newXml == nullptr)
        return false;

    auto newFile = getUserPresetFile(newId);
    if (!newXml->writeTo(newFile))
        return false;

    if (presetId != newId)
        file.deleteFile();

    return true;
}

bool PresetManager::exportPreset(const juce::String& presetId, const juce::File& destination) const
{
    // Try user preset first
    auto file = getUserPresetFile(presetId);
    if (file.existsAsFile())
        return file.copyFileTo(destination);

    // For built-in presets, serialize them
    auto config = VisualConfiguration::getPreset(presetId);

    juce::ValueTree wrapper(PRESET_WRAPPER_TYPE);
    wrapper.setProperty("presetId", presetId, nullptr);

    auto builtInPresets = VisualConfiguration::getAvailablePresets();
    for (const auto& [id, name] : builtInPresets)
    {
        if (id == presetId)
        {
            wrapper.setProperty("displayName", name, nullptr);
            break;
        }
    }

    wrapper.setProperty("version", 1, nullptr);
    wrapper.addChild(config.toValueTree(), -1, nullptr);

    auto xml = wrapper.createXml();
    if (xml == nullptr)
        return false;

    return xml->writeTo(destination);
}

bool PresetManager::importPreset(const juce::File& source)
{
    if (!source.existsAsFile())
        return false;

    auto wrapper = parsePresetFile(source);
    if (!wrapper.has_value())
        return false;

    juce::String const id = wrapper->getProperty("presetId", "");
    if (id.isEmpty())
        return false;

    // Re-serialize the validated tree to the user presets directory
    auto xml = wrapper->createXml();
    if (xml == nullptr)
        return false;

    return xml->writeTo(getUserPresetFile(id));
}

juce::File PresetManager::getPresetsDirectory() const { return presetsDir_; }

bool PresetManager::isUserPreset(const juce::String& presetId) const
{
    return getUserPresetFile(presetId).existsAsFile();
}

juce::String PresetManager::sanitizeFilename(const juce::String& name)
{
    return name.replaceCharacters(" /\\:*?\"<>|", "__________").trimCharactersAtStart(".").substring(0, 64);
}

juce::String PresetManager::generatePresetId(const juce::String& name)
{
    return "user_" + sanitizeFilename(name.toLowerCase());
}

juce::File PresetManager::getUserPresetFile(const juce::String& presetId) const
{
    return presetsDir_.getChildFile(sanitizeFilename(presetId) + ".oscpreset");
}

std::vector<juce::File> PresetManager::getUserPresetFiles() const
{
    std::vector<juce::File> files;
    for (const auto& entry : juce::RangedDirectoryIterator(presetsDir_, false, "*.oscpreset"))
    {
        files.push_back(entry.getFile());
    }
    return files;
}

} // namespace multiscoper
