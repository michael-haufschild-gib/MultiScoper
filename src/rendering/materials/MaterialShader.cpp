/*
    Oscil - Material Shader Base Class Implementation
*/

#include "rendering/materials/MaterialShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

void MaterialShader::setMaterialUniforms(juce::OpenGLExtensionFunctions& ext,
                                         GLint baseColorLoc,
                                         GLint metallicLoc,
                                         GLint roughnessLoc,
                                         GLint reflectivityLoc,
                                         GLint iorLoc,
                                         GLint emissiveLoc,
                                         GLint fresnelPowerLoc,
                                         const MaterialProperties& material)
{
    if (baseColorLoc >= 0)
    {
        ext.glUniform4f(baseColorLoc,
                        material.baseColorR,
                        material.baseColorG,
                        material.baseColorB,
                        material.baseColorA);
    }

    if (metallicLoc >= 0)
        ext.glUniform1f(metallicLoc, material.metallic);

    if (roughnessLoc >= 0)
        ext.glUniform1f(roughnessLoc, material.roughness);

    if (reflectivityLoc >= 0)
        ext.glUniform1f(reflectivityLoc, material.reflectivity);

    if (iorLoc >= 0)
        ext.glUniform1f(iorLoc, material.ior);

    if (emissiveLoc >= 0)
    {
        ext.glUniform4f(emissiveLoc,
                        material.emissiveR,
                        material.emissiveG,
                        material.emissiveB,
                        material.emissiveIntensity);
    }

    if (fresnelPowerLoc >= 0)
        ext.glUniform1f(fresnelPowerLoc, material.fresnelPower);
}

void MaterialShader::bindEnvironmentMap(juce::OpenGLExtensionFunctions& ext,
                                        int textureUnit,
                                        GLint uniformLoc) const
{
    if (envMapTexture_ == 0)
        return;

    ext.glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
    glBindTexture(GL_TEXTURE_CUBE_MAP, envMapTexture_);

    if (uniformLoc >= 0)
        ext.glUniform1i(uniformLoc, textureUnit);
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
