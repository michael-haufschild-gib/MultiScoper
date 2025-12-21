/*
    Oscil - 3D Settings Serialization Implementation
*/

#include "rendering/serialization/Settings3DSerialization.h"

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
        config.cameraDistance = settings3DTree.getProperty("cameraDistance", 5.0f);
        config.cameraAngleX = settings3DTree.getProperty("cameraAngleX", 15.0f);
        config.cameraAngleY = settings3DTree.getProperty("cameraAngleY", 0.0f);
        config.autoRotate = settings3DTree.getProperty("autoRotate", false);
        config.rotateSpeed = settings3DTree.getProperty("rotateSpeed", 10.0f);
        config.meshResolutionX = settings3DTree.getProperty("meshResolutionX", 128);
        config.meshResolutionZ = settings3DTree.getProperty("meshResolutionZ", 32);
        config.meshScale = settings3DTree.getProperty("meshScale", 1.0f);
    }
    return config;
}

juce::var Settings3DSerialization::toJson(const Settings3D& settings3D)
{
    auto settings3DObj = new juce::DynamicObject();
    settings3DObj->setProperty("enabled", settings3D.enabled);
    settings3DObj->setProperty("camera_distance", settings3D.cameraDistance);
    settings3DObj->setProperty("camera_angle_x", settings3D.cameraAngleX);
    settings3DObj->setProperty("camera_angle_y", settings3D.cameraAngleY);
    settings3DObj->setProperty("auto_rotate", settings3D.autoRotate);
    settings3DObj->setProperty("rotate_speed", settings3D.rotateSpeed);
    settings3DObj->setProperty("mesh_resolution_x", settings3D.meshResolutionX);
    settings3DObj->setProperty("mesh_resolution_z", settings3D.meshResolutionZ);
    settings3DObj->setProperty("mesh_scale", settings3D.meshScale);
    return juce::var(settings3DObj);
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
        config.cameraDistance = static_cast<float>(static_cast<double>(obj->getProperty("camera_distance")));
        config.cameraAngleX = static_cast<float>(static_cast<double>(obj->getProperty("camera_angle_x")));
        config.cameraAngleY = static_cast<float>(static_cast<double>(obj->getProperty("camera_angle_y")));
        config.autoRotate = obj->getProperty("auto_rotate");
        config.rotateSpeed = static_cast<float>(static_cast<double>(obj->getProperty("rotate_speed")));
        config.meshResolutionX = static_cast<int>(obj->getProperty("mesh_resolution_x"));
        config.meshResolutionZ = static_cast<int>(obj->getProperty("mesh_resolution_z"));
        config.meshScale = static_cast<float>(static_cast<double>(obj->getProperty("mesh_scale")));
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
        config.lightDirX = lightingTree.getProperty("lightDirX", 0.5f);
        config.lightDirY = lightingTree.getProperty("lightDirY", 1.0f);
        config.lightDirZ = lightingTree.getProperty("lightDirZ", 0.3f);
        config.ambientR = lightingTree.getProperty("ambientR", 0.1f);
        config.ambientG = lightingTree.getProperty("ambientG", 0.1f);
        config.ambientB = lightingTree.getProperty("ambientB", 0.15f);
        config.diffuseR = lightingTree.getProperty("diffuseR", 1.0f);
        config.diffuseG = lightingTree.getProperty("diffuseG", 1.0f);
        config.diffuseB = lightingTree.getProperty("diffuseB", 1.0f);
        config.specularR = lightingTree.getProperty("specularR", 1.0f);
        config.specularG = lightingTree.getProperty("specularG", 1.0f);
        config.specularB = lightingTree.getProperty("specularB", 1.0f);
        config.specularPower = lightingTree.getProperty("specularPower", 32.0f);
        config.specularIntensity = lightingTree.getProperty("specularIntensity", 0.5f);
    }
    return config;
}

juce::var Settings3DSerialization::toJson(const LightingConfig& lighting)
{
    auto lightingObj = new juce::DynamicObject();
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
    return juce::var(lightingObj);
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
        config.lightDirX = static_cast<float>(static_cast<double>(obj->getProperty("light_dir_x")));
        config.lightDirY = static_cast<float>(static_cast<double>(obj->getProperty("light_dir_y")));
        config.lightDirZ = static_cast<float>(static_cast<double>(obj->getProperty("light_dir_z")));
        config.ambientR = static_cast<float>(static_cast<double>(obj->getProperty("ambient_r")));
        config.ambientG = static_cast<float>(static_cast<double>(obj->getProperty("ambient_g")));
        config.ambientB = static_cast<float>(static_cast<double>(obj->getProperty("ambient_b")));
        config.diffuseR = static_cast<float>(static_cast<double>(obj->getProperty("diffuse_r")));
        config.diffuseG = static_cast<float>(static_cast<double>(obj->getProperty("diffuse_g")));
        config.diffuseB = static_cast<float>(static_cast<double>(obj->getProperty("diffuse_b")));
        config.specularR = static_cast<float>(static_cast<double>(obj->getProperty("specular_r")));
        config.specularG = static_cast<float>(static_cast<double>(obj->getProperty("specular_g")));
        config.specularB = static_cast<float>(static_cast<double>(obj->getProperty("specular_b")));
        config.specularPower = static_cast<float>(static_cast<double>(obj->getProperty("specular_power")));
        config.specularIntensity = static_cast<float>(static_cast<double>(obj->getProperty("specular_intensity")));
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
        config.reflectivity = materialTree.getProperty("reflectivity", 0.5f);
        config.refractiveIndex = materialTree.getProperty("refractiveIndex", 1.5f);
        config.fresnelPower = materialTree.getProperty("fresnelPower", 2.0f);
        config.tintColor = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(materialTree.getProperty("tintColor", static_cast<juce::int64>(0xFFFFFFFF)))));
        config.roughness = materialTree.getProperty("roughness", 0.1f);
        config.useEnvironmentMap = materialTree.getProperty("useEnvironmentMap", true);
        config.environmentMapId = materialTree.getProperty("environmentMapId", "default_studio").toString();
    }
    return config;
}

juce::var Settings3DSerialization::toJson(const MaterialSettings& material)
{
    auto materialObj = new juce::DynamicObject();
    materialObj->setProperty("enabled", material.enabled);
    materialObj->setProperty("reflectivity", material.reflectivity);
    materialObj->setProperty("refractive_index", material.refractiveIndex);
    materialObj->setProperty("fresnel_power", material.fresnelPower);
    materialObj->setProperty("tint_color", colorToHexLocal(material.tintColor));
    materialObj->setProperty("roughness", material.roughness);
    materialObj->setProperty("use_environment_map", material.useEnvironmentMap);
    materialObj->setProperty("environment_map_id", material.environmentMapId);
    return juce::var(materialObj);
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
        config.reflectivity = static_cast<float>(static_cast<double>(obj->getProperty("reflectivity")));
        config.refractiveIndex = static_cast<float>(static_cast<double>(obj->getProperty("refractive_index")));
        config.fresnelPower = static_cast<float>(static_cast<double>(obj->getProperty("fresnel_power")));
        config.tintColor = hexToColorLocal(obj->getProperty("tint_color").toString());
        config.roughness = static_cast<float>(static_cast<double>(obj->getProperty("roughness")));
        config.useEnvironmentMap = obj->getProperty("use_environment_map");
        config.environmentMapId = obj->getProperty("environment_map_id").toString();
    }
    return config;
}

} // namespace oscil
