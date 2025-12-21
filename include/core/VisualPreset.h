/*
    Oscil - Visual Preset Entity
    Data structures for visual preset storage and management.
*/

#pragma once

#include "rendering/VisualConfiguration.h"
#include <juce_core/juce_core.h>

namespace oscil
{

// ============================================================================
// Preset Category Enum
// ============================================================================

/**
 * Category of a visual preset.
 */
enum class PresetCategory
{
    System,     // Built-in, read-only presets (default, vector_scope, etc.)
    User,       // User-created custom presets
    Factory     // Bundled preset packs (future extensibility)
};

/**
 * Convert PresetCategory to string for JSON serialization.
 */
inline juce::String presetCategoryToString(PresetCategory category)
{
    switch (category)
    {
        case PresetCategory::System:  return "system";
        case PresetCategory::User:    return "user";
        case PresetCategory::Factory: return "factory";
        default:                      return "user";
    }
}

/**
 * Convert string to PresetCategory for JSON deserialization.
 */
inline PresetCategory stringToPresetCategory(const juce::String& str)
{
    if (str == "system")  return PresetCategory::System;
    if (str == "factory") return PresetCategory::Factory;
    return PresetCategory::User;
}

// ============================================================================
// Result Type for Error Handling
// ============================================================================

/**
 * Result type for operations that can fail.
 * Provides clear success/failure indication with optional value or error message.
 */
template<typename T>
struct Result
{
    bool success = false;
    T value{};
    juce::String error;

    static Result<T> ok(const T& val)
    {
        Result<T> r;
        r.success = true;
        r.value = val;
        return r;
    }

    static Result<T> ok(T&& val)
    {
        Result<T> r;
        r.success = true;
        r.value = std::move(val);
        return r;
    }

    static Result<T> fail(const juce::String& errorMessage)
    {
        Result<T> r;
        r.success = false;
        r.error = errorMessage;
        return r;
    }
};

/**
 * Specialization for void operations that can fail.
 */
template<>
struct Result<void>
{
    bool success = false;
    juce::String error;

    static Result<void> ok()
    {
        Result<void> r;
        r.success = true;
        return r;
    }

    static Result<void> fail(const juce::String& errorMessage)
    {
        Result<void> r;
        r.success = false;
        r.error = errorMessage;
        return r;
    }
};

// ============================================================================
// Visual Preset Entity
// ============================================================================

/**
 * A complete visual preset with metadata and configuration.
 *
 * Fields:
 * - id: UUID for stable identification across sessions
 * - name: Display name (1-64 chars, unique per category)
 * - description: Optional description (0-256 chars)
 * - category: System, User, or Factory
 * - isFavorite: User-marked as favorite
 * - isReadOnly: True for system presets
 * - createdAt: Creation timestamp (ISO 8601 UTC)
 * - modifiedAt: Last modification timestamp (ISO 8601 UTC)
 * - version: Schema version for future migration
 * - configuration: Complete visual settings
 * - thumbnailData: Optional base64-encoded PNG preview
 */
struct VisualPreset
{
    juce::String id;
    juce::String name;
    juce::String description;
    PresetCategory category = PresetCategory::User;
    bool isFavorite = false;
    bool isReadOnly = false;
    juce::Time createdAt;
    juce::Time modifiedAt;
    int version = 1;
    VisualConfiguration configuration;
    juce::String thumbnailData;

    /**
     * Check if this preset is valid (has required fields).
     */
    [[nodiscard]] bool isValid() const
    {
        return id.isNotEmpty() && name.isNotEmpty();
    }

    /**
     * Serialize to JSON object.
     */
    [[nodiscard]] juce::var toJson() const;

    /**
     * Deserialize from JSON object.
     */
    static VisualPreset fromJson(const juce::var& json);

    /**
     * Generate a new UUID for preset identification.
     */
    static juce::String generateId()
    {
        return juce::Uuid().toString();
    }

    /**
     * Get current UTC timestamp.
     */
    static juce::Time now()
    {
        return juce::Time::getCurrentTime();
    }
};

// ============================================================================
// Preset Name Validation
// ============================================================================

/**
 * Validate a preset name according to PRD rules:
 * - Length: 1-64 characters
 * - Characters: Alphanumeric, spaces, hyphens, underscores
 * - Reserved names: "default", "none", "new"
 */
inline Result<void> validatePresetName(const juce::String& name)
{
    if (name.isEmpty())
        return Result<void>::fail("Preset name cannot be empty");

    if (name.length() > 64)
        return Result<void>::fail("Preset name cannot exceed 64 characters");

    // Check for valid characters
    for (int i = 0; i < name.length(); ++i)
    {
        juce::juce_wchar c = name[i];
        bool isValid = juce::CharacterFunctions::isLetterOrDigit(c) ||
                       c == ' ' || c == '-' || c == '_';
        if (!isValid)
            return Result<void>::fail("Preset name contains invalid characters. Use only letters, numbers, spaces, hyphens, and underscores.");
    }

    // Check reserved names
    juce::String lowerName = name.toLowerCase();
    if (lowerName == "default" || lowerName == "none" || lowerName == "new")
        return Result<void>::fail("'" + name + "' is a reserved name and cannot be used");

    return Result<void>::ok();
}

} // namespace oscil
