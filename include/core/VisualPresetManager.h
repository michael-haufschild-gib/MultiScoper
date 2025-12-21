/*
    Oscil - Visual Preset Manager
    Manages visual preset storage, CRUD operations, and file I/O.
*/

#pragma once

#include "core/VisualPreset.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <optional>
#include <memory>

namespace oscil
{

/**
 * Manages visual presets with persistent file storage.
 *
 * Features:
 * - System presets (read-only, shipped with app)
 * - User presets (custom, stored in user app data)
 * - CRUD operations with validation
 * - Import/Export to/from files
 * - Favorites management
 * - Listener interface for change notifications
 *
 * Storage locations:
 * - macOS: ~/Library/Application Support/Oscil/Presets/
 * - Windows: %APPDATA%\Oscil\Presets\
 * - Linux: ~/.config/Oscil/Presets/
 */
class VisualPresetManager
{
public:
    // ========================================================================
    // Listener Interface
    // ========================================================================

    /**
     * Listener interface for preset change notifications.
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when a new preset is added. */
        virtual void presetAdded(const VisualPreset& preset) { juce::ignoreUnused(preset); }

        /** Called when an existing preset is updated. */
        virtual void presetUpdated(const VisualPreset& preset) { juce::ignoreUnused(preset); }

        /** Called when a preset is removed. */
        virtual void presetRemoved(const juce::String& presetId) { juce::ignoreUnused(presetId); }

        /** Called when the entire preset list is reloaded. */
        virtual void presetsReloaded() {}
    };

    // ========================================================================
    // Lifecycle
    // ========================================================================

    VisualPresetManager();
    
    /**
     * Constructor for testing/custom storage location.
     * @param rootDirectory Directory where presets will be stored.
     */
    explicit VisualPresetManager(const juce::File& rootDirectory);

    ~VisualPresetManager();

    // Non-copyable, movable
    VisualPresetManager(const VisualPresetManager&) = delete;
    VisualPresetManager& operator=(const VisualPresetManager&) = delete;
    VisualPresetManager(VisualPresetManager&&) noexcept;
    VisualPresetManager& operator=(VisualPresetManager&&) noexcept;

    /**
     * Initialize the manager, create directories, and load presets.
     * Must be called before any other operations.
     */
    void initialize();

    /**
     * Shutdown the manager and save any pending changes.
     */
    void shutdown();

    // ========================================================================
    // Read Operations
    // ========================================================================

    /**
     * Get all available presets (system + user).
     */
    [[nodiscard]] std::vector<VisualPreset> getAllPresets() const;

    /**
     * Get presets by category.
     */
    [[nodiscard]] std::vector<VisualPreset> getPresetsByCategory(PresetCategory category) const;

    /**
     * Get all presets marked as favorite.
     */
    [[nodiscard]] std::vector<VisualPreset> getFavoritePresets() const;

    /**
     * Get a preset by its unique ID.
     */
    [[nodiscard]] std::optional<VisualPreset> getPresetById(const juce::String& id) const;

    /**
     * Get a preset by its name (first match).
     */
    [[nodiscard]] std::optional<VisualPreset> getPresetByName(const juce::String& name) const;

    /**
     * Get the count of all presets.
     */
    [[nodiscard]] int getPresetCount() const;

    // ========================================================================
    // Write Operations
    // ========================================================================

    /**
     * Create a new user preset from a configuration.
     * @param name Display name for the preset
     * @param config Visual configuration to save
     * @return The created preset on success, or error message on failure
     */
    Result<VisualPreset> createPreset(const juce::String& name, const VisualConfiguration& config);

    /**
     * Create a new user preset with description.
     * @param name Display name for the preset
     * @param description Optional description
     * @param config Visual configuration to save
     * @return The created preset on success, or error message on failure
     */
    Result<VisualPreset> createPreset(const juce::String& name, const juce::String& description, const VisualConfiguration& config);

    /**
     * Update an existing preset's configuration.
     * @param id Preset ID to update
     * @param config New visual configuration
     * @return Success or error message
     */
    Result<void> updatePreset(const juce::String& id, const VisualConfiguration& config);

    /**
     * Rename an existing preset.
     * @param id Preset ID to rename
     * @param newName New display name
     * @return Success or error message
     */
    Result<void> renamePreset(const juce::String& id, const juce::String& newName);

    /**
     * Update preset description.
     * @param id Preset ID to update
     * @param description New description
     * @return Success or error message
     */
    Result<void> setDescription(const juce::String& id, const juce::String& description);

    /**
     * Delete a preset.
     * @param id Preset ID to delete
     * @return Success or error message
     */
    Result<void> deletePreset(const juce::String& id);

    /**
     * Duplicate an existing preset with a new name.
     * @param id Preset ID to duplicate
     * @param newName Name for the duplicate
     * @return The duplicated preset on success, or error message on failure
     */
    Result<VisualPreset> duplicatePreset(const juce::String& id, const juce::String& newName);

    // ========================================================================
    // Favorites
    // ========================================================================

    /**
     * Set or unset a preset as favorite.
     * @param id Preset ID
     * @param isFavorite True to mark as favorite
     * @return Success or error message
     */
    Result<void> setFavorite(const juce::String& id, bool isFavorite);

    // ========================================================================
    // Import/Export
    // ========================================================================

    /**
     * Import a preset from a JSON file.
     * @param file Source file
     * @return The imported preset on success, or error message on failure
     */
    Result<VisualPreset> importPreset(const juce::File& file);

    /**
     * Export a preset to a JSON file.
     * @param id Preset ID to export
     * @param destination Target file
     * @return Success or error message
     */
    Result<void> exportPreset(const juce::String& id, const juce::File& destination);

    // ========================================================================
    // Listener Management
    // ========================================================================

    /**
     * Add a listener for preset change notifications.
     */
    void addListener(Listener* listener);

    /**
     * Remove a listener.
     */
    void removeListener(Listener* listener);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * Get the presets directory for this platform.
     */
    [[nodiscard]] static juce::File getPresetsDirectory();

    /**
     * Get the system presets directory.
     */
    [[nodiscard]] static juce::File getSystemPresetsDirectory();

    /**
     * Get the user presets directory.
     */
    [[nodiscard]] static juce::File getUserPresetsDirectory();

    /**
     * Check if a preset name is unique within its category.
     * @param name Name to check
     * @param category Category to check within
     * @param excludeId Optional ID to exclude from check (for rename operations)
     */
    [[nodiscard]] bool isNameUnique(const juce::String& name, PresetCategory category,
                                     const juce::String& excludeId = {}) const;

    /**
     * Reload all presets from disk.
     */
    void reloadPresets();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace oscil
