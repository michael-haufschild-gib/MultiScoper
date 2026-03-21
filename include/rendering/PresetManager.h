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
    PresetManager();

    std::vector<PresetInfo> getAvailablePresets() const;

    VisualConfiguration loadPreset(const juce::String& presetId) const;

    bool saveUserPreset(const juce::String& name,
                        const VisualConfiguration& config);

    bool deleteUserPreset(const juce::String& presetId);

    bool renameUserPreset(const juce::String& presetId,
                          const juce::String& newName);

    bool exportPreset(const juce::String& presetId,
                      const juce::File& destination) const;

    bool importPreset(const juce::File& source);

    juce::File getPresetsDirectory() const;

    bool isUserPreset(const juce::String& presetId) const;

private:
    static juce::String sanitizeFilename(const juce::String& name);
    static juce::String generatePresetId(const juce::String& name);
    juce::File getUserPresetFile(const juce::String& presetId) const;
    std::vector<juce::File> getUserPresetFiles() const;

    juce::File presetsDir_;
};

} // namespace oscil
