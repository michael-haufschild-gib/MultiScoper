/*
    MultiScoper - Visual Preset Manager Implementation
*/

#include "rendering/PresetManager.h"

#include <optional>

namespace multiscoper
{

static const juce::Identifier PRESET_WRAPPER_TYPE("MultiScoperPreset");
/// Current preset schema version. Bump when the on-disk format changes;
/// loaders reject any preset whose `version` property is newer.
static constexpr int PRESET_SCHEMA_VERSION = 1;

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

    // Reject preset files written by a newer schema. Missing `version` is
    // treated as v1 (the only version that ever existed).
    int const version = tree.getProperty("version", 1);
    if (version > PRESET_SCHEMA_VERSION)
        return std::nullopt;

    return tree;
}

namespace
{
juce::File defaultPresetsDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
        .getChildFile("Application Support")
#endif
        .getChildFile("MultiScoper")
        .getChildFile("presets");
}
} // namespace

PresetManager::PresetManager() : PresetManager(defaultPresetsDirectory()) {}

PresetManager::PresetManager(juce::File presetsDir) : presetsDir_(std::move(presetsDir))
{
    presetsDir_.createDirectory();
}

std::vector<PresetInfo> PresetManager::getAvailablePresets() const
{
    if (listingCache_.has_value())
        return *listingCache_;

    std::vector<PresetInfo> presets;

    auto builtIn = VisualConfiguration::getAvailablePresets();
    auto files = getUserPresetFiles();
    presets.reserve(builtIn.size() + files.size());
    for (const auto& [id, name] : builtIn)
    {
        presets.push_back({.id = id, .displayName = name, .isBuiltIn = true});
    }

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

    listingCache_ = presets;
    return presets;
}

void PresetManager::invalidateListingCache() { listingCache_.reset(); }

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

std::optional<juce::String> PresetManager::saveUserPreset(const juce::String& name, const VisualConfiguration& config)
{
    if (name.isEmpty())
        return std::nullopt;

    juce::String id = generatePresetId(name);
    if (id == "user_")
        return std::nullopt; // sanitizeFilename reduced name to nothing

    juce::ValueTree wrapper(PRESET_WRAPPER_TYPE);
    wrapper.setProperty("presetId", id, nullptr);
    wrapper.setProperty("displayName", name, nullptr);
    wrapper.setProperty("version", PRESET_SCHEMA_VERSION, nullptr);
    wrapper.addChild(config.toValueTree(), -1, nullptr);

    auto xml = wrapper.createXml();
    if (xml == nullptr)
    {
        DBG("PresetManager::saveUserPreset: failed to create XML for: " << name);
        return std::nullopt;
    }

    auto file = getUserPresetFile(id);
    if (!xml->writeTo(file))
    {
        DBG("PresetManager::saveUserPreset: failed to write: " << file.getFullPathName());
        return std::nullopt;
    }
    invalidateListingCache();
    return id;
}

bool PresetManager::deleteUserPreset(const juce::String& presetId)
{
    if (!isUserPreset(presetId))
        return false;

    auto file = getUserPresetFile(presetId);
    const bool deleted = file.deleteFile();
    if (deleted)
        invalidateListingCache();
    return deleted;
}

bool PresetManager::renameUserPreset(const juce::String& presetId, const juce::String& newName)
{
    if (newName.isEmpty())
        return false;

    auto file = getUserPresetFile(presetId);
    auto wrapper = parsePresetFile(file);
    if (!wrapper.has_value())
        return false;

    juce::String const newId = generatePresetId(newName);
    if (newId == "user_")
        return false;

    auto newFile = getUserPresetFile(newId);
    if (presetId != newId && newFile.existsAsFile())
        return false; // refuse to clobber a different user preset

    wrapper->setProperty("presetId", newId, nullptr);
    wrapper->setProperty("displayName", newName, nullptr);

    auto newXml = wrapper->createXml();
    if (newXml == nullptr)
        return false;

    if (!newXml->writeTo(newFile))
        return false;

    if (presetId != newId)
    {
        if (!file.deleteFile())
        {
            newFile.deleteFile(); // rollback so we don't leak a duplicate
            return false;
        }
    }

    invalidateListingCache();
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

    wrapper.setProperty("version", PRESET_SCHEMA_VERSION, nullptr);
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

    // Re-derive the preset ID from the displayName (or filename stem fallback)
    // so imported files cannot claim a reserved built-in ID or silently
    // overwrite a user preset that happens to share the encoded ID.
    juce::String displayName = wrapper->getProperty("displayName", "").toString();
    if (displayName.isEmpty())
        displayName = source.getFileNameWithoutExtension();
    if (displayName.isEmpty())
        return false;

    juce::String const id = generatePresetId(displayName);
    if (id == "user_")
        return false;
    if (isBuiltInPresetId(id))
        return false;
    if (getUserPresetFile(id).existsAsFile())
        return false; // refuse silent overwrite; caller may delete first

    wrapper->setProperty("presetId", id, nullptr);
    wrapper->setProperty("displayName", displayName, nullptr);

    auto xml = wrapper->createXml();
    if (xml == nullptr)
        return false;

    if (!xml->writeTo(getUserPresetFile(id)))
        return false;
    invalidateListingCache();
    return true;
}

juce::File PresetManager::getPresetsDirectory() const { return presetsDir_; }

bool PresetManager::isUserPreset(const juce::String& presetId) const
{
    return getUserPresetFile(presetId).existsAsFile();
}

bool PresetManager::presetExists(const juce::String& presetId) const
{
    if (isUserPreset(presetId))
        return true;
    return isBuiltInPresetId(presetId);
}

bool PresetManager::isBuiltInPresetId(const juce::String& presetId)
{
    for (const auto& [id, name] : VisualConfiguration::getAvailablePresets())
    {
        if (id == presetId)
            return true;
    }
    return false;
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
