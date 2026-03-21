/*
    Oscil - Visual Preset Manager Implementation
*/

#include "rendering/PresetManager.h"

namespace oscil
{

static const juce::Identifier PRESET_WRAPPER_TYPE("OscilPreset");

PresetManager::PresetManager()
{
    presetsDir_ = juce::File::getSpecialLocation(
                      juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                      .getChildFile("Application Support")
#endif
                      .getChildFile("OscilAudio")
                      .getChildFile("oscil4")
                      .getChildFile("presets");

    presetsDir_.createDirectory();
}

std::vector<PresetInfo> PresetManager::getAvailablePresets() const
{
    std::vector<PresetInfo> presets;

    // Built-in presets first
    auto builtIn = VisualConfiguration::getAvailablePresets();
    for (const auto& [id, name] : builtIn)
    {
        presets.push_back({id, name, true});
    }

    // User presets from disk
    auto files = getUserPresetFiles();
    for (const auto& file : files)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr)
            continue;

        auto tree = juce::ValueTree::fromXml(*xml);
        if (!tree.isValid() || tree.getType() != PRESET_WRAPPER_TYPE)
            continue;

        juce::String id = tree.getProperty("presetId", "");
        juce::String name = tree.getProperty("displayName", "");
        if (id.isEmpty())
            continue;

        presets.push_back({id, name.isEmpty() ? id : name, false});
    }

    return presets;
}

VisualConfiguration PresetManager::loadPreset(const juce::String& presetId) const
{
    // Check user presets first (allows overriding built-ins)
    auto file = getUserPresetFile(presetId);
    if (file.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr)
        {
            auto wrapper = juce::ValueTree::fromXml(*xml);
            if (wrapper.isValid() && wrapper.getType() == PRESET_WRAPPER_TYPE)
            {
                auto configTree = wrapper.getChildWithName("VisualConfiguration");
                if (configTree.isValid())
                {
                    return VisualConfiguration::fromValueTree(configTree);
                }
            }
        }
    }

    // Fall back to built-in
    return VisualConfiguration::getPreset(presetId);
}

bool PresetManager::saveUserPreset(const juce::String& name,
                                   const VisualConfiguration& config)
{
    if (name.isEmpty())
        return false;

    juce::String id = generatePresetId(name);

    // Build wrapper tree: { presetId, displayName, VisualConfiguration }
    juce::ValueTree wrapper(PRESET_WRAPPER_TYPE);
    wrapper.setProperty("presetId", id, nullptr);
    wrapper.setProperty("displayName", name, nullptr);
    wrapper.setProperty("version", 1, nullptr);
    wrapper.addChild(config.toValueTree(), -1, nullptr);

    auto xml = wrapper.createXml();
    if (xml == nullptr)
        return false;

    auto file = getUserPresetFile(id);
    return xml->writeTo(file);
}

bool PresetManager::deleteUserPreset(const juce::String& presetId)
{
    if (!isUserPreset(presetId))
        return false;

    auto file = getUserPresetFile(presetId);
    return file.deleteFile();
}

bool PresetManager::renameUserPreset(const juce::String& presetId,
                                     const juce::String& newName)
{
    auto file = getUserPresetFile(presetId);
    if (!file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;

    auto wrapper = juce::ValueTree::fromXml(*xml);
    if (!wrapper.isValid())
        return false;

    juce::String newId = generatePresetId(newName);
    wrapper.setProperty("presetId", newId, nullptr);
    wrapper.setProperty("displayName", newName, nullptr);

    auto newXml = wrapper.createXml();
    if (newXml == nullptr)
        return false;

    // Write to new file, delete old
    auto newFile = getUserPresetFile(newId);
    if (!newXml->writeTo(newFile))
        return false;

    if (presetId != newId)
        file.deleteFile();

    return true;
}

bool PresetManager::exportPreset(const juce::String& presetId,
                                 const juce::File& destination) const
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

    auto xml = juce::XmlDocument::parse(source);
    if (xml == nullptr)
        return false;

    auto wrapper = juce::ValueTree::fromXml(*xml);
    if (!wrapper.isValid() || wrapper.getType() != PRESET_WRAPPER_TYPE)
        return false;

    juce::String id = wrapper.getProperty("presetId", "");
    if (id.isEmpty())
        return false;

    // Copy to user presets directory
    auto destFile = getUserPresetFile(id);
    return xml->writeTo(destFile);
}

juce::File PresetManager::getPresetsDirectory() const
{
    return presetsDir_;
}

bool PresetManager::isUserPreset(const juce::String& presetId) const
{
    return getUserPresetFile(presetId).existsAsFile();
}

juce::String PresetManager::sanitizeFilename(const juce::String& name)
{
    return name.replaceCharacters(" /\\:*?\"<>|", "__________")
        .trimCharactersAtStart(".")
        .substring(0, 64);
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
    for (const auto& entry :
         juce::RangedDirectoryIterator(presetsDir_, false, "*.oscpreset"))
    {
        files.push_back(entry.getFile());
    }
    return files;
}

} // namespace oscil
