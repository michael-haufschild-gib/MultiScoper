#pragma once

#include "rendering/VisualConfiguration.h"

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace multiscoper
{

struct PresetInfo
{
    juce::String id;
    juce::String displayName;
    bool isBuiltIn = false;
};

class PresetManager
{
public:
    /// Create a preset manager using the default user data directory.
    PresetManager();

    /// Create a preset manager with an explicit presets directory.
    /// Enables test isolation — never use this to point into the real user
    /// data directory from tests or a partial data location that a user
    /// might share with other subsystems.
    explicit PresetManager(juce::File presetsDir);

    /// Get metadata for all built-in and user presets.
    std::vector<PresetInfo> getAvailablePresets() const;

    /// Load a visual configuration from a preset by ID.
    VisualConfiguration loadPreset(const juce::String& presetId) const;

    /// Save a visual configuration as a user preset. Returns the generated
    /// preset ID on success, std::nullopt on any failure (empty name, I/O).
    /// If another preset already has the same generated ID, it is overwritten;
    /// call presetExists() first if the caller needs a collision prompt.
    std::optional<juce::String> saveUserPreset(const juce::String& name, const VisualConfiguration& config);

    /// Delete a user preset by ID.
    bool deleteUserPreset(const juce::String& presetId);

    /// Rename a user preset. Fails if newName resolves to an existing user
    /// preset (other than the one being renamed). On I/O failure after writing
    /// the new file, the new file is removed to restore prior state.
    bool renameUserPreset(const juce::String& presetId, const juce::String& newName);

    /// Export a preset to an external file.
    bool exportPreset(const juce::String& presetId, const juce::File& destination) const;

    /// Import a preset from an external file. The resulting preset ID is
    /// re-derived from the file's displayName (or filename stem) so imports
    /// cannot shadow built-in presets by claiming a reserved ID. Fails if the
    /// import would overwrite an existing user preset.
    bool importPreset(const juce::File& source);

    /// Get the filesystem directory where user presets are stored.
    juce::File getPresetsDirectory() const;

    /// Check whether the given preset ID refers to a user-created preset.
    bool isUserPreset(const juce::String& presetId) const;

    /// Check whether any preset — user or built-in — with this ID exists.
    bool presetExists(const juce::String& presetId) const;

private:
    static std::optional<juce::ValueTree> parsePresetFile(const juce::File& file);
    static juce::String sanitizeFilename(const juce::String& name);
    static juce::String generatePresetId(const juce::String& name);
    static bool isBuiltInPresetId(const juce::String& presetId);
    juce::File getUserPresetFile(const juce::String& presetId) const;
    std::vector<juce::File> getUserPresetFiles() const;

    /// Invalidate the cached user-preset listing after any CRUD operation.
    void invalidateListingCache();

    juce::File presetsDir_;
    mutable std::optional<std::vector<PresetInfo>> listingCache_;
};

} // namespace multiscoper
