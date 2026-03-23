#pragma once

#include "rendering/VisualConfiguration.h"

#include <juce_core/juce_core.h>

#include <vector>

namespace oscil
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
    /// Create a preset manager and ensure the presets directory exists.
    PresetManager();

    /// Get metadata for all built-in and user presets.
    std::vector<PresetInfo> getAvailablePresets() const;

    /// Load a visual configuration from a preset by ID.
    VisualConfiguration loadPreset(const juce::String& presetId) const;

    /// Save a visual configuration as a user preset.
    bool saveUserPreset(const juce::String& name, const VisualConfiguration& config);

    /// Delete a user preset by ID.
    bool deleteUserPreset(const juce::String& presetId);

    /// Rename a user preset.
    bool renameUserPreset(const juce::String& presetId, const juce::String& newName);

    /// Export a preset to an external file.
    bool exportPreset(const juce::String& presetId, const juce::File& destination) const;

    /// Import a preset from an external file.
    bool importPreset(const juce::File& source);

    /// Get the filesystem directory where user presets are stored.
    juce::File getPresetsDirectory() const;

    /// Check whether the given preset ID refers to a user-created preset.
    bool isUserPreset(const juce::String& presetId) const;

private:
    static juce::String sanitizeFilename(const juce::String& name);
    static juce::String generatePresetId(const juce::String& name);
    juce::File getUserPresetFile(const juce::String& presetId) const;
    std::vector<juce::File> getUserPresetFiles() const;

    juce::File presetsDir_;
};

} // namespace oscil
