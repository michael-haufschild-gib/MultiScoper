/*
    Oscil - 3D Settings Serialization Implementation
*/

#include "rendering/serialization/Settings3DSerialization.h"
#include <cmath>

namespace oscil
{

namespace
{
    static const juce::Identifier SETTINGS3D_TYPE("Settings3D");
    static const juce::Identifier LIGHTING_TYPE("Lighting");
    static const juce::Identifier MATERIAL_TYPE("Material");

    juce::String colorToHexLocal(juce::Colour colour)
    {
        return juce::String::toHexString(static_cast<int>(colour.getARGB()));
    }

    juce::Colour hexToColorLocal(const juce::String& hex)
    {
        return juce::Colour(static_cast<juce::uint32>(hex.getHexValue64()));
    }

    /**
     * Clamp and validate a float value, logging if out of range
     */
    float clampAndValidate3D(float value, float minVal, float maxVal, float defaultVal, const char* fieldName)
    {
        if (std::isnan(value) || std::isinf(value))
        {
            juce::Logger::writeToLog(juce::String("Settings3D: Invalid ") + fieldName +
                                     " (nan/inf), using default " + juce::String(defaultVal));
            return defaultVal;
        }
        if (value < minVal || value > maxVal)
        {
            juce::Logger::writeToLog(juce::String("Settings3D: ") + fieldName + " " +
                                     juce::String(value) + " out of range [" +
                                     juce::String(minVal) + "," + juce::String(maxVal) + "], clamping");
            return std::clamp(value, minVal, maxVal);
        }
        return value;
    }

    /**
     * Clamp and validate an int value, logging if out of range
     */
    int clampAndValidate3DInt(int value, int minVal, int maxVal, int defaultVal, const char* fieldName)
    {
        if (value < minVal || value > maxVal)
        {
            juce::Logger::writeToLog(juce::String("Settings3D: ") + fieldName + " " +
                                     juce::String(value) + " out of range [" +
                                     juce::String(minVal) + "," + juce::String(maxVal) + "], clamping");
            return std::clamp(value, minVal, maxVal);
        }
        return value;
    }
}

juce::ValueTree Settings3DSerialization::toValueTree(const Settings3D& settings3D)
{
    juce::ValueTree settings3DTree(SETTINGS3D_TYPE);
    settings3DTree.setProperty("enabled", settings3D.enabled, nullptr);
    settings3DTree.setProperty("cameraDistance", settings3D.cameraDistance, nullptr);
    settings3DTree.setProperty("cameraAngleX", settings3D.cameraAngleX, nullptr);
    settings3DTree.setProperty("cameraAngleY", settings3D.cameraAngleY, nullptr);
    settings3DTree.setProperty("autoRotate", settings3D.autoRotate, nullptr);
    settings3DTree.setProperty("rotateSpeed", settings3D.rotateSpeed, nullptr);
    settings3DTree.setProperty("meshResolutionX", settings3D.meshResolutionX, nullptr);
    settings3DTree.setProperty("meshResolutionZ", settings3D.meshResolutionZ, nullptr);
    settings3DTree.setProperty("meshScale", settings3D.meshScale, nullptr);
    return settings3DTree;
}

Settings3D Settings3DSerialization::fromValueTree(const juce::ValueTree& tree)
{
    Settings3D config;
    juce::ValueTree settings3DTree = tree.hasType(SETTINGS3D_TYPE) ? tree : tree.getChildWithName(SETTINGS3D_TYPE);

    if (settings3DTree.isValid())
    {
        config.enabled = settings3DTree.getProperty("enabled", false);
        config.cameraDistance = clampAndValidate3D(
            settings3DTree.getProperty("cameraDistance", 5.0f), 0.1f, 100.0f, 5.0f, "cameraDistance");
        config.cameraAngleX = clampAndValidate3D(
            settings3DTree.getProperty("cameraAngleX", 15.0f), -180.0f, 180.0f, 15.0f, "cameraAngleX");
        config.cameraAngleY = clampAndValidate3D(
            settings3DTree.getProperty("cameraAngleY", 0.0f), -180.0f, 180.0f, 0.0f, "cameraAngleY");
        config.autoRotate = settings3DTree.getProperty("autoRotate", false);
        config.rotateSpeed = clampAndValidate3D(
            settings3DTree.getProperty("rotateSpeed", 10.0f), 0.0f, 100.0f, 10.0f, "rotateSpeed");
        config.meshResolutionX = clampAndValidate3DInt(
            settings3DTree.getProperty("meshResolutionX", 128), 4, 1024, 128, "meshResolutionX");
        config.meshResolutionZ = clampAndValidate3DInt(
            settings3DTree.getProperty("meshResolutionZ", 32), 4, 256, 32, "meshResolutionZ");
        config.meshScale = clampAndValidate3D(
            settings3DTree.getProperty("meshScale", 1.0f), 0.01f, 10.0f, 1.0f, "meshScale");
    }
    return config;
}

juce::var Settings3DSerialization::toJson(const Settings3D& settings3D)
{
    // Use DynamicObject::Ptr for exception-safe memory management
    juce::DynamicObject::Ptr settings3DObj(new juce::DynamicObject());
    settings3DObj->setProperty("enabled", settings3D.enabled);
    settings3DObj->setProperty("camera_distance", settings3D.cameraDistance);
    settings3DObj->setProperty("camera_angle_x", settings3D.cameraAngleX);
    settings3DObj->setProperty("camera_angle_y", settings3D.cameraAngleY);
    settings3DObj->setProperty("auto_rotate", settings3D.autoRotate);
    settings3DObj->setProperty("rotate_speed", settings3D.rotateSpeed);
    settings3DObj->setProperty("mesh_resolution_x", settings3D.meshResolutionX);
    settings3DObj->setProperty("mesh_resolution_z", settings3D.meshResolutionZ);
    settings3DObj->setProperty("mesh_scale", settings3D.meshScale);
    return juce::var(settings3DObj.get());
}

Settings3D Settings3DSerialization::fromJson(const juce::var& json)
{
    Settings3D config;
    const juce::DynamicObject* obj = nullptr;
    if (auto* root = json.getDynamicObject())
    {
        if (root->hasProperty("camera_distance")) obj = root;
        else if (auto* prop = root->getProperty("settings_3d").getDynamicObject()) obj = prop;
    }

    if (obj)
    {
        config.enabled = obj->getProperty("enabled");
        config.cameraDistance = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("camera_distance"))),
            0.1f, 100.0f, 5.0f, "cameraDistance");
        config.cameraAngleX = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("camera_angle_x"))),
            -180.0f, 180.0f, 15.0f, "cameraAngleX");
        config.cameraAngleY = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("camera_angle_y"))),
            -180.0f, 180.0f, 0.0f, "cameraAngleY");
        config.autoRotate = obj->getProperty("auto_rotate");
        config.rotateSpeed = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("rotate_speed"))),
            0.0f, 100.0f, 10.0f, "rotateSpeed");
        config.meshResolutionX = clampAndValidate3DInt(
            static_cast<int>(obj->getProperty("mesh_resolution_x")), 4, 1024, 128, "meshResolutionX");
        config.meshResolutionZ = clampAndValidate3DInt(
            static_cast<int>(obj->getProperty("mesh_resolution_z")), 4, 256, 32, "meshResolutionZ");
        config.meshScale = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("mesh_scale"))),
            0.01f, 10.0f, 1.0f, "meshScale");
    }
    return config;
}

juce::ValueTree Settings3DSerialization::toValueTree(const LightingConfig& lighting)
{
    juce::ValueTree lightingTree(LIGHTING_TYPE);
    lightingTree.setProperty("lightDirX", lighting.lightDirX, nullptr);
    lightingTree.setProperty("lightDirY", lighting.lightDirY, nullptr);
    lightingTree.setProperty("lightDirZ", lighting.lightDirZ, nullptr);
    lightingTree.setProperty("ambientR", lighting.ambientR, nullptr);
    lightingTree.setProperty("ambientG", lighting.ambientG, nullptr);
    lightingTree.setProperty("ambientB", lighting.ambientB, nullptr);
    lightingTree.setProperty("diffuseR", lighting.diffuseR, nullptr);
    lightingTree.setProperty("diffuseG", lighting.diffuseG, nullptr);
    lightingTree.setProperty("diffuseB", lighting.diffuseB, nullptr);
    lightingTree.setProperty("specularR", lighting.specularR, nullptr);
    lightingTree.setProperty("specularG", lighting.specularG, nullptr);
    lightingTree.setProperty("specularB", lighting.specularB, nullptr);
    lightingTree.setProperty("specularPower", lighting.specularPower, nullptr);
    lightingTree.setProperty("specularIntensity", lighting.specularIntensity, nullptr);
    return lightingTree;
}

LightingConfig Settings3DSerialization::lightingFromValueTree(const juce::ValueTree& tree)
{
    LightingConfig config;
    juce::ValueTree lightingTree = tree.hasType(LIGHTING_TYPE) ? tree : tree.getChildWithName(LIGHTING_TYPE);

    if (lightingTree.isValid())
    {
        // Light direction components can be any value (normalized vector)
        config.lightDirX = clampAndValidate3D(
            lightingTree.getProperty("lightDirX", 0.5f), -10.0f, 10.0f, 0.5f, "lightDirX");
        config.lightDirY = clampAndValidate3D(
            lightingTree.getProperty("lightDirY", 1.0f), -10.0f, 10.0f, 1.0f, "lightDirY");
        config.lightDirZ = clampAndValidate3D(
            lightingTree.getProperty("lightDirZ", 0.3f), -10.0f, 10.0f, 0.3f, "lightDirZ");
        // Color components should be 0.0-1.0
        config.ambientR = clampAndValidate3D(
            lightingTree.getProperty("ambientR", 0.1f), 0.0f, 1.0f, 0.1f, "ambientR");
        config.ambientG = clampAndValidate3D(
            lightingTree.getProperty("ambientG", 0.1f), 0.0f, 1.0f, 0.1f, "ambientG");
        config.ambientB = clampAndValidate3D(
            lightingTree.getProperty("ambientB", 0.15f), 0.0f, 1.0f, 0.15f, "ambientB");
        config.diffuseR = clampAndValidate3D(
            lightingTree.getProperty("diffuseR", 1.0f), 0.0f, 1.0f, 1.0f, "diffuseR");
        config.diffuseG = clampAndValidate3D(
            lightingTree.getProperty("diffuseG", 1.0f), 0.0f, 1.0f, 1.0f, "diffuseG");
        config.diffuseB = clampAndValidate3D(
            lightingTree.getProperty("diffuseB", 1.0f), 0.0f, 1.0f, 1.0f, "diffuseB");
        config.specularR = clampAndValidate3D(
            lightingTree.getProperty("specularR", 1.0f), 0.0f, 1.0f, 1.0f, "specularR");
        config.specularG = clampAndValidate3D(
            lightingTree.getProperty("specularG", 1.0f), 0.0f, 1.0f, 1.0f, "specularG");
        config.specularB = clampAndValidate3D(
            lightingTree.getProperty("specularB", 1.0f), 0.0f, 1.0f, 1.0f, "specularB");
        config.specularPower = clampAndValidate3D(
            lightingTree.getProperty("specularPower", 32.0f), 1.0f, 256.0f, 32.0f, "specularPower");
        config.specularIntensity = clampAndValidate3D(
            lightingTree.getProperty("specularIntensity", 0.5f), 0.0f, 2.0f, 0.5f, "specularIntensity");
    }
    return config;
}

juce::var Settings3DSerialization::toJson(const LightingConfig& lighting)
{
    // Use DynamicObject::Ptr for exception-safe memory management
    juce::DynamicObject::Ptr lightingObj(new juce::DynamicObject());
    lightingObj->setProperty("light_dir_x", lighting.lightDirX);
    lightingObj->setProperty("light_dir_y", lighting.lightDirY);
    lightingObj->setProperty("light_dir_z", lighting.lightDirZ);
    lightingObj->setProperty("ambient_r", lighting.ambientR);
    lightingObj->setProperty("ambient_g", lighting.ambientG);
    lightingObj->setProperty("ambient_b", lighting.ambientB);
    lightingObj->setProperty("diffuse_r", lighting.diffuseR);
    lightingObj->setProperty("diffuse_g", lighting.diffuseG);
    lightingObj->setProperty("diffuse_b", lighting.diffuseB);
    lightingObj->setProperty("specular_r", lighting.specularR);
    lightingObj->setProperty("specular_g", lighting.specularG);
    lightingObj->setProperty("specular_b", lighting.specularB);
    lightingObj->setProperty("specular_power", lighting.specularPower);
    lightingObj->setProperty("specular_intensity", lighting.specularIntensity);
    return juce::var(lightingObj.get());
}

LightingConfig Settings3DSerialization::lightingFromJson(const juce::var& json)
{
    LightingConfig config;
    const juce::DynamicObject* obj = nullptr;
    if (auto* root = json.getDynamicObject())
    {
        if (root->hasProperty("light_dir_x")) obj = root;
        else if (auto* prop = root->getProperty("lighting").getDynamicObject()) obj = prop;
    }

    if (obj)
    {
        config.lightDirX = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("light_dir_x"))),
            -10.0f, 10.0f, 0.5f, "lightDirX");
        config.lightDirY = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("light_dir_y"))),
            -10.0f, 10.0f, 1.0f, "lightDirY");
        config.lightDirZ = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("light_dir_z"))),
            -10.0f, 10.0f, 0.3f, "lightDirZ");
        config.ambientR = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("ambient_r"))),
            0.0f, 1.0f, 0.1f, "ambientR");
        config.ambientG = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("ambient_g"))),
            0.0f, 1.0f, 0.1f, "ambientG");
        config.ambientB = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("ambient_b"))),
            0.0f, 1.0f, 0.15f, "ambientB");
        config.diffuseR = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("diffuse_r"))),
            0.0f, 1.0f, 1.0f, "diffuseR");
        config.diffuseG = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("diffuse_g"))),
            0.0f, 1.0f, 1.0f, "diffuseG");
        config.diffuseB = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("diffuse_b"))),
            0.0f, 1.0f, 1.0f, "diffuseB");
        config.specularR = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("specular_r"))),
            0.0f, 1.0f, 1.0f, "specularR");
        config.specularG = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("specular_g"))),
            0.0f, 1.0f, 1.0f, "specularG");
        config.specularB = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("specular_b"))),
            0.0f, 1.0f, 1.0f, "specularB");
        config.specularPower = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("specular_power"))),
            1.0f, 256.0f, 32.0f, "specularPower");
        config.specularIntensity = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("specular_intensity"))),
            0.0f, 2.0f, 0.5f, "specularIntensity");
    }
    return config;
}

juce::ValueTree Settings3DSerialization::toValueTree(const MaterialSettings& material)
{
    juce::ValueTree materialTree(MATERIAL_TYPE);
    materialTree.setProperty("enabled", material.enabled, nullptr);
    materialTree.setProperty("reflectivity", material.reflectivity, nullptr);
    materialTree.setProperty("refractiveIndex", material.refractiveIndex, nullptr);
    materialTree.setProperty("fresnelPower", material.fresnelPower, nullptr);
    materialTree.setProperty("tintColor", static_cast<juce::int64>(material.tintColor.getARGB()), nullptr);
    materialTree.setProperty("roughness", material.roughness, nullptr);
    materialTree.setProperty("useEnvironmentMap", material.useEnvironmentMap, nullptr);
    materialTree.setProperty("environmentMapId", material.environmentMapId, nullptr);
    return materialTree;
}

MaterialSettings Settings3DSerialization::materialFromValueTree(const juce::ValueTree& tree)
{
    MaterialSettings config;
    juce::ValueTree materialTree = tree.hasType(MATERIAL_TYPE) ? tree : tree.getChildWithName(MATERIAL_TYPE);

    if (materialTree.isValid())
    {
        config.enabled = materialTree.getProperty("enabled", false);
        config.reflectivity = clampAndValidate3D(
            materialTree.getProperty("reflectivity", 0.5f), 0.0f, 1.0f, 0.5f, "reflectivity");
        config.refractiveIndex = clampAndValidate3D(
            materialTree.getProperty("refractiveIndex", 1.5f), 1.0f, 3.0f, 1.5f, "refractiveIndex");
        config.fresnelPower = clampAndValidate3D(
            materialTree.getProperty("fresnelPower", 2.0f), 0.1f, 10.0f, 2.0f, "fresnelPower");
        config.tintColor = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(materialTree.getProperty("tintColor", static_cast<juce::int64>(0xFFFFFFFF)))));
        config.roughness = clampAndValidate3D(
            materialTree.getProperty("roughness", 0.1f), 0.0f, 1.0f, 0.1f, "roughness");
        config.useEnvironmentMap = materialTree.getProperty("useEnvironmentMap", true);
        config.environmentMapId = materialTree.getProperty("environmentMapId", "default_studio").toString();
        // Validate environment map ID isn't excessively long
        if (config.environmentMapId.length() > 256)
        {
            juce::Logger::writeToLog("Settings3D: environmentMapId too long, truncating");
            config.environmentMapId = config.environmentMapId.substring(0, 256);
        }
    }
    return config;
}

juce::var Settings3DSerialization::toJson(const MaterialSettings& material)
{
    // Use DynamicObject::Ptr for exception-safe memory management
    juce::DynamicObject::Ptr materialObj(new juce::DynamicObject());
    materialObj->setProperty("enabled", material.enabled);
    materialObj->setProperty("reflectivity", material.reflectivity);
    materialObj->setProperty("refractive_index", material.refractiveIndex);
    materialObj->setProperty("fresnel_power", material.fresnelPower);
    materialObj->setProperty("tint_color", colorToHexLocal(material.tintColor));
    materialObj->setProperty("roughness", material.roughness);
    materialObj->setProperty("use_environment_map", material.useEnvironmentMap);
    materialObj->setProperty("environment_map_id", material.environmentMapId);
    return juce::var(materialObj.get());
}

MaterialSettings Settings3DSerialization::materialFromJson(const juce::var& json)
{
    MaterialSettings config;
    const juce::DynamicObject* obj = nullptr;
    if (auto* root = json.getDynamicObject())
    {
        if (root->hasProperty("reflectivity")) obj = root;
        else if (auto* prop = root->getProperty("material").getDynamicObject()) obj = prop;
    }

    if (obj)
    {
        config.enabled = obj->getProperty("enabled");
        config.reflectivity = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("reflectivity"))),
            0.0f, 1.0f, 0.5f, "reflectivity");
        config.refractiveIndex = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("refractive_index"))),
            1.0f, 3.0f, 1.5f, "refractiveIndex");
        config.fresnelPower = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("fresnel_power"))),
            0.1f, 10.0f, 2.0f, "fresnelPower");
        config.tintColor = hexToColorLocal(obj->getProperty("tint_color").toString());
        config.roughness = clampAndValidate3D(
            static_cast<float>(static_cast<double>(obj->getProperty("roughness"))),
            0.0f, 1.0f, 0.1f, "roughness");
        config.useEnvironmentMap = obj->getProperty("use_environment_map");
        config.environmentMapId = obj->getProperty("environment_map_id").toString();
        // Validate environment map ID isn't excessively long
        if (config.environmentMapId.length() > 256)
        {
            juce::Logger::writeToLog("Settings3D: environmentMapId too long, truncating");
            config.environmentMapId = config.environmentMapId.substring(0, 256);
        }
    }
    return config;
}

} // namespace oscil
