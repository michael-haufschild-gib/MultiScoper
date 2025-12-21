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
    auto obj = new juce::DynamicObject();
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
    return juce::var(obj);
}

VisualPreset VisualPreset::fromJson(const juce::var& json)
{
    VisualPreset preset;
    if (auto* obj = json.getDynamicObject())
    {
        preset.version = static_cast<int>(obj->getProperty(JsonKeys::SCHEMA_VERSION));
        preset.id = obj->getProperty(JsonKeys::ID).toString();
        preset.name = obj->getProperty(JsonKeys::NAME).toString();
        preset.description = obj->getProperty(JsonKeys::DESCRIPTION).toString();
        preset.category = stringToPresetCategory(obj->getProperty(JsonKeys::CATEGORY).toString());
        preset.isFavorite = obj->getProperty(JsonKeys::IS_FAVORITE);
        preset.isReadOnly = (preset.category == PresetCategory::System);
        preset.createdAt = iso8601ToTime(obj->getProperty(JsonKeys::CREATED_AT).toString());
        preset.modifiedAt = iso8601ToTime(obj->getProperty(JsonKeys::MODIFIED_AT).toString());
        preset.configuration = VisualConfiguration::fromJson(obj->getProperty(JsonKeys::CONFIGURATION));
        preset.configuration.presetId = preset.id;
        if (obj->hasProperty(JsonKeys::THUMBNAIL_DATA))
            preset.thumbnailData = obj->getProperty(JsonKeys::THUMBNAIL_DATA).toString();
    }
    return preset;
}

} // namespace oscil
