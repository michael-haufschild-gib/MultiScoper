#include "core/VisualPresetManager.h"
#include "core/presets/SystemPresetFactory.h"
#include <algorithm>
#include <map>
#include <unordered_map>

namespace oscil
{

// ============================================================================
// Hash support for juce::String in std::unordered_map
// ============================================================================

struct JuceStringHash
{
    std::size_t operator()(const juce::String& s) const noexcept
    {
        return static_cast<std::size_t>(s.hashCode());
    }
};

// ============================================================================
// VisualPresetManager Implementation (PIMPL)
// ============================================================================

class VisualPresetManager::Impl
{
public:
    std::vector<VisualPreset> presets_;
    std::unordered_map<juce::String, size_t, JuceStringHash> presetIdIndex_;  // PERFORMANCE: O(1) lookup by ID
    juce::ListenerList<Listener> listeners_;
    bool initialized_ = false;
    juce::File rootDirectory_;

    // PERFORMANCE: Rebuild index after bulk operations
    void rebuildIndex()
    {
        presetIdIndex_.clear();
        for (size_t i = 0; i < presets_.size(); ++i)
        {
            presetIdIndex_[presets_[i].id] = i;
        }
    }

    juce::File getPresetsDirectory() const
    {
        if (rootDirectory_ != juce::File())
            return rootDirectory_;
        return VisualPresetManager::getPresetsDirectory();
    }

    juce::File getSystemPresetsDirectory() const
    {
        return getPresetsDirectory().getChildFile("system");
    }

    juce::File getUserPresetsDirectory() const
    {
        return getPresetsDirectory().getChildFile("user");
    }

    void createDirectories()
    {
        auto presetsDir = getPresetsDirectory();
        auto systemDir = getSystemPresetsDirectory();
        auto userDir = getUserPresetsDirectory();

        // Create directories with error checking - failures indicate permission issues
        if (!presetsDir.createDirectory())
            juce::Logger::writeToLog("VisualPresetManager: Failed to create presets directory: " + presetsDir.getFullPathName());
        if (!systemDir.createDirectory())
            juce::Logger::writeToLog("VisualPresetManager: Failed to create system presets directory: " + systemDir.getFullPathName());
        if (!userDir.createDirectory())
            juce::Logger::writeToLog("VisualPresetManager: Failed to create user presets directory: " + userDir.getFullPathName());
    }

    void createSystemPresets()
    {
        auto systemDir = getSystemPresetsDirectory();
        auto systemPresets = SystemPresetFactory::getSystemPresets();
        auto now = juce::Time::getCurrentTime();

        for (const auto& def : systemPresets)
        {
            auto file = systemDir.getChildFile(juce::String(def.id) + ".json");

            // Create preset
            VisualPreset preset;
            preset.id = def.id;
            preset.name = def.name;
            preset.description = def.description;
            preset.category = PresetCategory::System;
            preset.isReadOnly = true;
            preset.isFavorite = false;
            preset.version = 1;
            preset.createdAt = now;
            preset.modifiedAt = now;

            // Set up configuration
            preset.configuration.presetId = def.id;
            def.setup(preset.configuration);

            // Write to file with error checking
            auto json = preset.toJson();
            auto jsonString = juce::JSON::toString(json, true);
            if (!file.replaceWithText(jsonString))
                juce::Logger::writeToLog("VisualPresetManager: Failed to write system preset: " + file.getFullPathName());
        }
    }

    void loadPresets()
    {
        presets_.clear();
        presetIdIndex_.clear();

        // Load system presets
        auto systemDir = getSystemPresetsDirectory();
        loadPresetsFromDirectory(systemDir, PresetCategory::System);

        // Load user presets
        auto userDir = getUserPresetsDirectory();
        loadPresetsFromDirectory(userDir, PresetCategory::User);

        // PERFORMANCE: Build hash index for O(1) lookup
        rebuildIndex();
    }

    void loadPresetsFromDirectory(const juce::File& dir, PresetCategory expectedCategory)
    {
        if (!dir.exists())
            return;

        for (const auto& file : dir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto preset = loadPresetFromFile(file);
            if (preset.isValid())
            {
                // Check for duplicate UUID - skip if already loaded
                bool isDuplicate = false;
                for (const auto& existing : presets_)
                {
                    if (existing.id == preset.id)
                    {
                        juce::Logger::writeToLog("VisualPresetManager: Duplicate preset ID '" +
                                                 preset.id + "' found in " + file.getFullPathName() +
                                                 ", skipping (already loaded from another file)");
                        isDuplicate = true;
                        break;
                    }
                }

                if (!isDuplicate)
                {
                    preset.category = expectedCategory;
                    preset.isReadOnly = (expectedCategory == PresetCategory::System);
                    presets_.push_back(preset);
                }
            }
        }
    }

    VisualPreset loadPresetFromFile(const juce::File& file)
    {
        VisualPreset preset;

        // Validate file exists and is readable
        if (!file.existsAsFile())
        {
            juce::Logger::writeToLog("VisualPresetManager: Preset file does not exist: " + file.getFullPathName());
            return preset;
        }

        // Check file size - reject suspiciously large files (>10MB)
        auto fileSize = file.getSize();
        if (fileSize > 10 * 1024 * 1024)
        {
            juce::Logger::writeToLog("VisualPresetManager: Preset file too large (" +
                                     juce::String(fileSize) + " bytes): " + file.getFullPathName());
            return preset;
        }

        auto jsonText = file.loadFileAsString();
        if (jsonText.isEmpty())
        {
            juce::Logger::writeToLog("VisualPresetManager: Empty or unreadable preset file: " + file.getFullPathName());
            return preset;
        }

        // Validate JSON structure
        juce::String parseError;
        auto json = juce::JSON::parse(jsonText);
        if (json.isVoid())
        {
            // Try to get more specific error information
            juce::var result;
            auto parseResult = juce::JSON::parse(jsonText, result);
            if (parseResult.failed())
            {
                juce::Logger::writeToLog("VisualPresetManager: Failed to parse preset JSON: " +
                                         file.getFullPathName() + " - " + parseResult.getErrorMessage());
            }
            else
            {
                juce::Logger::writeToLog("VisualPresetManager: Failed to parse preset JSON: " + file.getFullPathName());
            }
            return preset;
        }

        // Validate it's a JSON object (not an array or primitive)
        if (!json.getDynamicObject())
        {
            juce::Logger::writeToLog("VisualPresetManager: Preset JSON is not an object: " + file.getFullPathName());
            return preset;
        }

        preset = VisualPreset::fromJson(json);

        // Validate the loaded preset has minimum required data
        if (!preset.isValid())
        {
            juce::Logger::writeToLog("VisualPresetManager: Loaded preset is invalid (missing required fields): " + file.getFullPathName());
        }

        return preset;
    }

    bool savePresetToFile(const VisualPreset& preset, const juce::File& file)
    {
        // Validate preset before saving
        if (preset.id.isEmpty())
        {
            juce::Logger::writeToLog("VisualPresetManager: Cannot save preset with empty ID");
            return false;
        }
        if (preset.name.isEmpty())
        {
            juce::Logger::writeToLog("VisualPresetManager: Cannot save preset with empty name");
            return false;
        }

        auto json = preset.toJson();
        auto jsonString = juce::JSON::toString(json, true);

        // Validate JSON output isn't empty
        if (jsonString.isEmpty())
        {
            juce::Logger::writeToLog("VisualPresetManager: Failed to serialize preset to JSON: " + preset.name);
            return false;
        }

        // Ensure parent directory exists
        auto parentDir = file.getParentDirectory();
        if (!parentDir.exists() && !parentDir.createDirectory())
        {
            juce::Logger::writeToLog("VisualPresetManager: Failed to create directory for preset: " +
                                     parentDir.getFullPathName());
            return false;
        }

        // Save with error handling
        if (!file.replaceWithText(jsonString))
        {
            juce::Logger::writeToLog("VisualPresetManager: Failed to write preset file: " + file.getFullPathName());
            return false;
        }

        return true;
    }

    juce::File getPresetFile(const VisualPreset& preset) const
    {
        auto dir = (preset.category == PresetCategory::System)
            ? getSystemPresetsDirectory()
            : getUserPresetsDirectory();

        // Use preset ID as filename (sanitized)
        auto filename = preset.id.replaceCharacters("\\/:*?\"<>|", "_") + ".json";
        return dir.getChildFile(filename);
    }

    // PERFORMANCE: O(1) lookup using hash map instead of O(N) linear search
    VisualPreset* findPresetById(const juce::String& id)
    {
        auto it = presetIdIndex_.find(id);
        if (it != presetIdIndex_.end() && it->second < presets_.size())
            return &presets_[it->second];
        return nullptr;
    }

    const VisualPreset* findPresetById(const juce::String& id) const
    {
        auto it = presetIdIndex_.find(id);
        if (it != presetIdIndex_.end() && it->second < presets_.size())
            return &presets_[it->second];
        return nullptr;
    }
};

// ============================================================================
// VisualPresetManager Public Implementation
// ============================================================================

VisualPresetManager::VisualPresetManager()
    : impl_(std::make_unique<Impl>())
{
}

VisualPresetManager::VisualPresetManager(const juce::File& rootDirectory)
    : impl_(std::make_unique<Impl>())
{
    impl_->rootDirectory_ = rootDirectory;
}

VisualPresetManager::~VisualPresetManager() = default;

VisualPresetManager::VisualPresetManager(VisualPresetManager&&) noexcept = default;
VisualPresetManager& VisualPresetManager::operator=(VisualPresetManager&&) noexcept = default;

void VisualPresetManager::initialize()
{
    if (impl_->initialized_)
        return;

    impl_->createDirectories();

    // Check if system presets exist, create if not
    // Use impl's method to respect custom rootDirectory_
    auto systemDir = impl_->getSystemPresetsDirectory();
    auto defaultPreset = systemDir.getChildFile("default.json");
    if (!defaultPreset.exists())
    {
        impl_->createSystemPresets();
    }

    impl_->loadPresets();
    impl_->initialized_ = true;
}

void VisualPresetManager::shutdown()
{
    impl_->initialized_ = false;
}

std::vector<VisualPreset> VisualPresetManager::getAllPresets() const
{
    return impl_->presets_;
}

std::vector<VisualPreset> VisualPresetManager::getPresetsByCategory(PresetCategory category) const
{
    std::vector<VisualPreset> result;
    for (const auto& preset : impl_->presets_)
    {
        if (preset.category == category)
            result.push_back(preset);
    }
    return result;
}

std::vector<VisualPreset> VisualPresetManager::getFavoritePresets() const
{
    std::vector<VisualPreset> result;
    for (const auto& preset : impl_->presets_)
    {
        if (preset.isFavorite)
            result.push_back(preset);
    }
    return result;
}

std::optional<VisualPreset> VisualPresetManager::getPresetById(const juce::String& id) const
{
    if (auto* preset = impl_->findPresetById(id))
        return *preset;
    return std::nullopt;
}

std::optional<VisualPreset> VisualPresetManager::getPresetByName(const juce::String& name) const
{
    for (const auto& preset : impl_->presets_)
    {
        if (preset.name == name)
            return preset;
    }
    return std::nullopt;
}

int VisualPresetManager::getPresetCount() const
{
    return static_cast<int>(impl_->presets_.size());
}

Result<VisualPreset> VisualPresetManager::createPreset(const juce::String& name, const VisualConfiguration& config)
{
    return createPreset(name, "", config);
}

Result<VisualPreset> VisualPresetManager::createPreset(const juce::String& name, const juce::String& description, const VisualConfiguration& config)
{
    // Validate name
    auto nameResult = validatePresetName(name);
    if (!nameResult.success)
        return Result<VisualPreset>::fail(nameResult.error);

    // Check uniqueness
    if (!isNameUnique(name, PresetCategory::User))
        return Result<VisualPreset>::fail("A preset with the name '" + name + "' already exists");

    // Create preset
    VisualPreset preset;
    preset.id = VisualPreset::generateId();
    preset.name = name;
    preset.description = description;
    preset.category = PresetCategory::User;
    preset.isReadOnly = false;
    preset.isFavorite = false;
    preset.version = 1;
    preset.createdAt = VisualPreset::now();
    preset.modifiedAt = preset.createdAt;
    preset.configuration = config;
    preset.configuration.presetId = preset.id;

    // Save to file
    auto file = impl_->getPresetFile(preset);
    if (!impl_->savePresetToFile(preset, file))
        return Result<VisualPreset>::fail("Failed to save preset to file");

    // Add to list and update index
    impl_->presetIdIndex_[preset.id] = impl_->presets_.size();
    impl_->presets_.push_back(preset);

    // Notify listeners
    impl_->listeners_.call([&preset](Listener& l) { l.presetAdded(preset); });

    return Result<VisualPreset>::ok(preset);
}

Result<void> VisualPresetManager::updatePreset(const juce::String& id, const VisualConfiguration& config)
{
    auto* preset = impl_->findPresetById(id);
    if (!preset)
        return Result<void>::fail("Preset not found: " + id);

    if (preset->isReadOnly)
        return Result<void>::fail("Cannot modify system preset");

    // Update configuration
    preset->configuration = config;
    preset->configuration.presetId = id;
    preset->modifiedAt = VisualPreset::now();

    // Save to file
    auto file = impl_->getPresetFile(*preset);
    if (!impl_->savePresetToFile(*preset, file))
        return Result<void>::fail("Failed to save preset to file");

    // Notify listeners - make a copy to avoid dangling pointer if listener modifies vector
    VisualPreset presetCopy = *preset;
    impl_->listeners_.call([&presetCopy](Listener& l) { l.presetUpdated(presetCopy); });

    return Result<void>::ok();
}

Result<void> VisualPresetManager::renamePreset(const juce::String& id, const juce::String& newName)
{
    auto* preset = impl_->findPresetById(id);
    if (!preset)
        return Result<void>::fail("Preset not found: " + id);

    if (preset->isReadOnly)
        return Result<void>::fail("Cannot rename system preset");

    // Validate name
    auto nameResult = validatePresetName(newName);
    if (!nameResult.success)
        return Result<void>::fail(nameResult.error);

    // Check uniqueness (excluding current preset)
    if (!isNameUnique(newName, preset->category, id))
        return Result<void>::fail("A preset with the name '" + newName + "' already exists");

    // Update name
    preset->name = newName;
    preset->modifiedAt = VisualPreset::now();

    // Save to file
    auto file = impl_->getPresetFile(*preset);
    if (!impl_->savePresetToFile(*preset, file))
        return Result<void>::fail("Failed to save preset to file");

    // Notify listeners - make a copy to avoid dangling pointer if listener modifies vector
    VisualPreset presetCopy = *preset;
    impl_->listeners_.call([&presetCopy](Listener& l) { l.presetUpdated(presetCopy); });

    return Result<void>::ok();
}

Result<void> VisualPresetManager::setDescription(const juce::String& id, const juce::String& description)
{
    auto* preset = impl_->findPresetById(id);
    if (!preset)
        return Result<void>::fail("Preset not found: " + id);

    if (preset->isReadOnly)
        return Result<void>::fail("Cannot modify system preset");

    if (description.length() > 256)
        return Result<void>::fail("Description cannot exceed 256 characters");

    // Update description
    preset->description = description;
    preset->modifiedAt = VisualPreset::now();

    // Save to file
    auto file = impl_->getPresetFile(*preset);
    if (!impl_->savePresetToFile(*preset, file))
        return Result<void>::fail("Failed to save preset to file");

    // Notify listeners - make a copy to avoid dangling pointer if listener modifies vector
    VisualPreset presetCopy = *preset;
    impl_->listeners_.call([&presetCopy](Listener& l) { l.presetUpdated(presetCopy); });

    return Result<void>::ok();
}

Result<void> VisualPresetManager::deletePreset(const juce::String& id)
{
    // PERFORMANCE: Use hash index for O(1) lookup
    auto indexIt = impl_->presetIdIndex_.find(id);
    if (indexIt == impl_->presetIdIndex_.end() || indexIt->second >= impl_->presets_.size())
        return Result<void>::fail("Preset not found: " + id);

    auto& preset = impl_->presets_[indexIt->second];
    if (preset.isReadOnly)
        return Result<void>::fail("Cannot delete system preset");

    // Delete file
    auto file = impl_->getPresetFile(preset);
    if (file.exists() && !file.deleteFile())
        return Result<void>::fail("Failed to delete preset file");

    // Remove from list and rebuild index (erase invalidates indices)
    impl_->presets_.erase(impl_->presets_.begin() + static_cast<ptrdiff_t>(indexIt->second));
    impl_->rebuildIndex();

    // Notify listeners
    impl_->listeners_.call([&id](Listener& l) { l.presetRemoved(id); });

    return Result<void>::ok();
}

Result<VisualPreset> VisualPresetManager::duplicatePreset(const juce::String& id, const juce::String& newName)
{
    auto original = getPresetById(id);
    if (!original)
        return Result<VisualPreset>::fail("Preset not found: " + id);

    return createPreset(newName, original->description, original->configuration);
}

Result<void> VisualPresetManager::setFavorite(const juce::String& id, bool isFavorite)
{
    auto* preset = impl_->findPresetById(id);
    if (!preset)
        return Result<void>::fail("Preset not found: " + id);

    // Update favorite status
    preset->isFavorite = isFavorite;
    preset->modifiedAt = VisualPreset::now();

    // Save to file (only for user presets)
    if (!preset->isReadOnly)
    {
        auto file = impl_->getPresetFile(*preset);
        if (!impl_->savePresetToFile(*preset, file))
            return Result<void>::fail("Failed to save preset to file");
    }

    // Notify listeners - make a copy to avoid dangling pointer if listener modifies vector
    VisualPreset presetCopy = *preset;
    impl_->listeners_.call([&presetCopy](Listener& l) { l.presetUpdated(presetCopy); });

    return Result<void>::ok();
}

Result<VisualPreset> VisualPresetManager::importPreset(const juce::File& file)
{
    if (!file.exists())
        return Result<VisualPreset>::fail("File not found: " + file.getFullPathName());

    auto preset = impl_->loadPresetFromFile(file);
    if (!preset.isValid())
        return Result<VisualPreset>::fail("Failed to parse preset file");

    // Generate new ID to avoid conflicts
    preset.id = VisualPreset::generateId();
    preset.configuration.presetId = preset.id;
    preset.category = PresetCategory::User;
    preset.isReadOnly = false;

    // Check for name collision and generate unique name if needed
    juce::String uniqueName = preset.name;
    int suffix = 1;
    while (!isNameUnique(uniqueName, PresetCategory::User))
    {
        uniqueName = preset.name + " (" + juce::String(suffix++) + ")";
    }
    preset.name = uniqueName;

    // Save to user presets directory
    auto destFile = impl_->getPresetFile(preset);
    if (!impl_->savePresetToFile(preset, destFile))
        return Result<VisualPreset>::fail("Failed to save imported preset");

    // Add to list and update index
    impl_->presetIdIndex_[preset.id] = impl_->presets_.size();
    impl_->presets_.push_back(preset);

    // Notify listeners
    impl_->listeners_.call([&preset](Listener& l) { l.presetAdded(preset); });

    return Result<VisualPreset>::ok(preset);
}

Result<void> VisualPresetManager::exportPreset(const juce::String& id, const juce::File& destination)
{
    auto preset = getPresetById(id);
    if (!preset)
        return Result<void>::fail("Preset not found: " + id);

    if (!impl_->savePresetToFile(*preset, destination))
        return Result<void>::fail("Failed to export preset to file");

    return Result<void>::ok();
}

void VisualPresetManager::addListener(Listener* listener)
{
    impl_->listeners_.add(listener);
}

void VisualPresetManager::removeListener(Listener* listener)
{
    impl_->listeners_.remove(listener);
}

juce::File VisualPresetManager::getPresetsDirectory()
{
#if JUCE_MAC
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Application Support")
        .getChildFile("MultiScoper")
        .getChildFile("Presets");
#elif JUCE_WINDOWS
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("MultiScoper")
        .getChildFile("Presets");
#else
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile(".config")
        .getChildFile("MultiScoper")
        .getChildFile("Presets");
#endif
}

juce::File VisualPresetManager::getSystemPresetsDirectory()
{
    return getPresetsDirectory().getChildFile("system");
}

juce::File VisualPresetManager::getUserPresetsDirectory()
{
    return getPresetsDirectory().getChildFile("user");
}

bool VisualPresetManager::isNameUnique(const juce::String& name, PresetCategory category, const juce::String& excludeId) const
{
    for (const auto& preset : impl_->presets_)
    {
        if (preset.category == category && preset.name == name && preset.id != excludeId)
            return false;
    }
    return true;
}

void VisualPresetManager::reloadPresets()
{
    impl_->loadPresets();
    impl_->listeners_.call([](Listener& l) { l.presetsReloaded(); });
}

} // namespace oscil
