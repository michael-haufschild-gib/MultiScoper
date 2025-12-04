/*
    Oscil - Lighting Configuration
    Common lighting settings for 3D shaders.
*/

#pragma once

namespace oscil
{

/**
 * Lighting configuration for 3D shaders.
 */
struct LightingConfig
{
    // Directional light
    float lightDirX = 0.5f;
    float lightDirY = 1.0f;
    float lightDirZ = 0.3f;

    // Colors
    float ambientR = 0.1f, ambientG = 0.1f, ambientB = 0.15f;
    float diffuseR = 1.0f, diffuseG = 1.0f, diffuseB = 1.0f;
    float specularR = 1.0f, specularG = 1.0f, specularB = 1.0f;

    float specularPower = 32.0f;
    float specularIntensity = 0.5f;
};

} // namespace oscil
