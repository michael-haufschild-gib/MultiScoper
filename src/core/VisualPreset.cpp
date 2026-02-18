/*
    Oscil - Visual Preset Entity Implementation
*/

#include "core/VisualPreset.h"

namespace oscil
{

// ============================================================================
// JSON Key Constants
// ============================================================================

namespace JsonKeys
{
    static const char* const SCHEMA_VERSION = "schema_version";
    static const char* const ID = "id";
    static const char* const NAME = "name";
    static const char* const DESCRIPTION = "description";
    static const char* const CATEGORY = "category";
    static const char* const IS_FAVORITE = "is_favorite";
    static const char* const CREATED_AT = "created_at";
    static const char* const MODIFIED_AT = "modified_at";
    static const char* const CONFIGURATION = "configuration";
    static const char* const THUMBNAIL_DATA = "thumbnail_data";
}

// ============================================================================
// UUID Format Validation Helper
// ============================================================================

/**
 * Validates that a string is in valid UUID format (8-4-4-4-12 hex digits).
 * Accepts both lowercase and uppercase hex, with or without braces.
 * Examples: "550e8400-e29b-41d4-a716-446655440000" or "{550e8400-e29b-41d4-a716-446655440000}"
 */
static bool isValidUuidFormat(const juce::String& str)
{
    if (str.isEmpty())
        return false;

    juce::String uuid = str;

    // Handle optional braces
    if (uuid.startsWithChar('{') && uuid.endsWithChar('}'))
        uuid = uuid.substring(1, uuid.length() - 1);

    // UUID should be 36 characters: 8-4-4-4-12 with hyphens
    if (uuid.length() != 36)
        return false;

    // Check hyphen positions
    if (uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-')
        return false;

    // Check that all other characters are hex digits
    for (int i = 0; i < uuid.length(); ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
            continue;  // Skip hyphens

        juce::juce_wchar c = uuid[i];
        bool isHex = (c >= '0' && c <= '9') ||
                     (c >= 'a' && c <= 'f') ||
                     (c >= 'A' && c <= 'F');
        if (!isHex)
            return false;
    }

    return true;
}

// ============================================================================
// Time Conversion Helpers
// ============================================================================

static juce::String timeToIso8601(const juce::Time& time)
{
    return time.toISO8601(true);
}

static juce::Time iso8601ToTime(const juce::String& str)
{
    return juce::Time::fromISO8601(str);
}

// ============================================================================
// VisualPreset JSON Serialization
// ============================================================================

juce::var VisualPreset::toJson() const
{
    // Use DynamicObject::Ptr for exception-safe memory management
    juce::DynamicObject::Ptr obj(new juce::DynamicObject());
    obj->setProperty(JsonKeys::SCHEMA_VERSION, version);
    obj->setProperty(JsonKeys::ID, id);
    obj->setProperty(JsonKeys::NAME, name);
    obj->setProperty(JsonKeys::DESCRIPTION, description);
    obj->setProperty(JsonKeys::CATEGORY, presetCategoryToString(category));
    obj->setProperty(JsonKeys::IS_FAVORITE, isFavorite);
    obj->setProperty(JsonKeys::CREATED_AT, timeToIso8601(createdAt));
    obj->setProperty(JsonKeys::MODIFIED_AT, timeToIso8601(modifiedAt));
    obj->setProperty(JsonKeys::CONFIGURATION, configuration.toJson());
    if (thumbnailData.isNotEmpty())
        obj->setProperty(JsonKeys::THUMBNAIL_DATA, thumbnailData);
    return juce::var(obj.get());
}

VisualPreset VisualPreset::fromJson(const juce::var& json)
{
    VisualPreset preset;
    auto* obj = json.getDynamicObject();
    if (!obj)
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Invalid JSON - not a dynamic object");
        return preset;
    }

    // Schema version validation
    preset.version = static_cast<int>(obj->getProperty(JsonKeys::SCHEMA_VERSION));
    if (preset.version < 0 || preset.version > 100)  // Reasonable upper bound
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Invalid schema version " +
                                 juce::String(preset.version) + ", using 1");
        preset.version = 1;
    }

    // ID validation - required field, must be valid UUID format
    preset.id = obj->getProperty(JsonKeys::ID).toString();
    if (preset.id.isEmpty())
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Empty preset ID, generating new one");
        preset.id = juce::Uuid().toString();
    }
    else if (!isValidUuidFormat(preset.id))
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Invalid UUID format '" + preset.id +
                                 "', generating new one");
        preset.id = juce::Uuid().toString();
    }

    // Name validation - required field, must not be empty
    preset.name = obj->getProperty(JsonKeys::NAME).toString();
    if (preset.name.isEmpty())
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Empty preset name, using 'Untitled'");
        preset.name = "Untitled";
    }
    else if (preset.name.length() > 128)
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Preset name too long, truncating");
        preset.name = preset.name.substring(0, 128);
    }

    // Description validation - optional, validate length
    preset.description = obj->getProperty(JsonKeys::DESCRIPTION).toString();
    if (preset.description.length() > 512)
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Description too long, truncating");
        preset.description = preset.description.substring(0, 512);
    }

    // Category validation
    juce::String categoryStr = obj->getProperty(JsonKeys::CATEGORY).toString();
    preset.category = stringToPresetCategory(categoryStr);
    // Note: stringToPresetCategory returns a default if invalid, so no extra logging needed

    preset.isFavorite = obj->getProperty(JsonKeys::IS_FAVORITE);
    preset.isReadOnly = (preset.category == PresetCategory::System);

    // Date validation - use current time if parsing fails (empty date string results in epoch)
    juce::String createdAtStr = obj->getProperty(JsonKeys::CREATED_AT).toString();
    preset.createdAt = iso8601ToTime(createdAtStr);
    if (preset.createdAt.toMilliseconds() == 0 && createdAtStr.isNotEmpty())
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Failed to parse createdAt date '" +
                                 createdAtStr + "', using current time");
        preset.createdAt = juce::Time::getCurrentTime();
    }
    else if (preset.createdAt.toMilliseconds() == 0)
    {
        preset.createdAt = juce::Time::getCurrentTime();
    }

    juce::String modifiedAtStr = obj->getProperty(JsonKeys::MODIFIED_AT).toString();
    preset.modifiedAt = iso8601ToTime(modifiedAtStr);
    if (preset.modifiedAt.toMilliseconds() == 0 && modifiedAtStr.isNotEmpty())
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Failed to parse modifiedAt date '" +
                                 modifiedAtStr + "', using current time");
        preset.modifiedAt = juce::Time::getCurrentTime();
    }
    else if (preset.modifiedAt.toMilliseconds() == 0)
    {
        preset.modifiedAt = juce::Time::getCurrentTime();
    }

    // Configuration - optional, use defaults if missing or invalid
    auto configVar = obj->getProperty(JsonKeys::CONFIGURATION);
    if (!configVar.isVoid() && configVar.getDynamicObject() != nullptr)
    {
        preset.configuration = VisualConfiguration::fromJson(configVar);
    }
    else
    {
        juce::Logger::writeToLog("VisualPreset::fromJson: Missing or invalid configuration, using defaults");
        preset.configuration = VisualConfiguration::getDefault();
    }
    preset.configuration.presetId = preset.id;

    // Thumbnail data - optional
    if (obj->hasProperty(JsonKeys::THUMBNAIL_DATA))
    {
        preset.thumbnailData = obj->getProperty(JsonKeys::THUMBNAIL_DATA).toString();
        // Validate thumbnail isn't excessively large (base64 encoded, ~4MB limit)
        if (preset.thumbnailData.length() > 5 * 1024 * 1024)
        {
            juce::Logger::writeToLog("VisualPreset::fromJson: Thumbnail data too large, ignoring");
            preset.thumbnailData = {};
        }
    }

    return preset;
}

} // namespace oscil
