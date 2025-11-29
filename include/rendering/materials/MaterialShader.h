/*
    Oscil - Material Shader Base Class
    Abstract base for physically-based material shaders
*/

#pragma once

#include "rendering/shaders3d/WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

/**
 * Material properties for PBR-like rendering.
 */
struct MaterialProperties
{
    // Base color
    float baseColorR = 1.0f;
    float baseColorG = 1.0f;
    float baseColorB = 1.0f;
    float baseColorA = 1.0f;

    // Physical properties
    float metallic = 0.0f;      // 0 = dielectric, 1 = metallic
    float roughness = 0.5f;     // 0 = smooth/mirror, 1 = rough/diffuse
    float reflectivity = 0.5f;  // Fresnel reflectivity

    // Refraction (for glass-like materials)
    float ior = 1.5f;           // Index of refraction
    float refractionStrength = 0.0f;

    // Emission
    float emissiveR = 0.0f;
    float emissiveG = 0.0f;
    float emissiveB = 0.0f;
    float emissiveIntensity = 0.0f;

    // Environment map influence
    float envMapStrength = 1.0f;
    float envMapBlur = 0.0f;  // Mip level for blurred reflections

    // Additional effects
    float fresnelPower = 5.0f;
    float normalStrength = 1.0f;
};

/**
 * Abstract base class for material-based shaders.
 * Extends WaveformShader3D with PBR material support.
 */
class MaterialShader : public WaveformShader3D
{
public:
    ~MaterialShader() override = default;

    /**
     * Set material properties.
     */
    virtual void setMaterial(const MaterialProperties& material) { material_ = material; }

    /**
     * Get current material properties.
     */
    [[nodiscard]] const MaterialProperties& getMaterial() const { return material_; }

    /**
     * Set environment map texture.
     * @param textureId OpenGL cubemap texture ID
     */
    virtual void setEnvironmentMap(GLuint textureId) { envMapTexture_ = textureId; }

    /**
     * Check if environment map is set.
     */
    [[nodiscard]] bool hasEnvironmentMap() const { return envMapTexture_ != 0; }

protected:
    /**
     * Helper to set common material uniforms.
     */
    static void setMaterialUniforms(juce::OpenGLExtensionFunctions& ext,
                                    GLint baseColorLoc,
                                    GLint metallicLoc,
                                    GLint roughnessLoc,
                                    GLint reflectivityLoc,
                                    GLint iorLoc,
                                    GLint emissiveLoc,
                                    GLint fresnelPowerLoc,
                                    const MaterialProperties& material);

    /**
     * Bind environment map to texture unit.
     * @param ext OpenGL extensions
     * @param textureUnit Texture unit to bind to (e.g., 1)
     * @param uniformLoc Sampler uniform location
     */
    void bindEnvironmentMap(juce::OpenGLExtensionFunctions& ext,
                            int textureUnit,
                            GLint uniformLoc) const;

    MaterialProperties material_;
    GLuint envMapTexture_ = 0;
};

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
