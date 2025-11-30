/*
    Oscil - Environment Map Manager Implementation
*/

#include "rendering/materials/EnvironmentMapManager.h"
#include <cmath>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

EnvironmentConfig EnvironmentConfig::fromPreset(EnvironmentPreset preset)
{
    EnvironmentConfig config;

    switch (preset)
    {
        case EnvironmentPreset::Dark:
            config.skyColorTopR = 0.02f; config.skyColorTopG = 0.02f; config.skyColorTopB = 0.05f;
            config.skyColorBottomR = 0.01f; config.skyColorBottomG = 0.01f; config.skyColorBottomB = 0.02f;
            config.horizonColorR = 0.05f; config.horizonColorG = 0.03f; config.horizonColorB = 0.08f;
            config.accentColorR = 0.2f; config.accentColorG = 0.1f; config.accentColorB = 0.3f;
            config.accentIntensity = 0.2f;
            break;

        case EnvironmentPreset::Neon:
            config.skyColorTopR = 0.05f; config.skyColorTopG = 0.0f; config.skyColorTopB = 0.1f;
            config.skyColorBottomR = 0.0f; config.skyColorBottomG = 0.02f; config.skyColorBottomB = 0.05f;
            config.horizonColorR = 0.8f; config.horizonColorG = 0.0f; config.horizonColorB = 0.5f;
            config.accentColorR = 0.0f; config.accentColorG = 1.0f; config.accentColorB = 1.0f;
            config.accentIntensity = 0.6f;
            config.horizonSharpness = 4.0f;
            break;

        case EnvironmentPreset::Studio:
            config.skyColorTopR = 0.15f; config.skyColorTopG = 0.15f; config.skyColorTopB = 0.17f;
            config.skyColorBottomR = 0.1f; config.skyColorBottomG = 0.1f; config.skyColorBottomB = 0.12f;
            config.horizonColorR = 0.2f; config.horizonColorG = 0.2f; config.horizonColorB = 0.22f;
            config.accentColorR = 1.0f; config.accentColorG = 0.95f; config.accentColorB = 0.9f;
            config.accentIntensity = 0.3f;
            config.horizonSharpness = 1.0f;
            break;

        case EnvironmentPreset::Sunset:
            config.skyColorTopR = 0.1f; config.skyColorTopG = 0.05f; config.skyColorTopB = 0.15f;
            config.skyColorBottomR = 0.02f; config.skyColorBottomG = 0.01f; config.skyColorBottomB = 0.03f;
            config.horizonColorR = 1.0f; config.horizonColorG = 0.4f; config.horizonColorB = 0.1f;
            config.accentColorR = 1.0f; config.accentColorG = 0.6f; config.accentColorB = 0.2f;
            config.accentIntensity = 0.5f;
            config.horizonSharpness = 3.0f;
            break;

        case EnvironmentPreset::CyberSpace:
            config.skyColorTopR = 0.0f; config.skyColorTopG = 0.0f; config.skyColorTopB = 0.0f;
            config.skyColorBottomR = 0.0f; config.skyColorBottomG = 0.02f; config.skyColorBottomB = 0.05f;
            config.horizonColorR = 0.0f; config.horizonColorG = 0.3f; config.horizonColorB = 0.5f;
            config.accentColorR = 0.0f; config.accentColorG = 1.0f; config.accentColorB = 0.5f;
            config.accentIntensity = 0.4f;
            config.horizonSharpness = 8.0f;
            break;

        case EnvironmentPreset::Abstract:
            config.skyColorTopR = 0.2f; config.skyColorTopG = 0.05f; config.skyColorTopB = 0.3f;
            config.skyColorBottomR = 0.05f; config.skyColorBottomG = 0.1f; config.skyColorBottomB = 0.2f;
            config.horizonColorR = 0.1f; config.horizonColorG = 0.3f; config.horizonColorB = 0.4f;
            config.accentColorR = 1.0f; config.accentColorG = 0.2f; config.accentColorB = 0.5f;
            config.accentIntensity = 0.35f;
            config.horizonSharpness = 2.0f;
            break;
    }

    return config;
}

EnvironmentMapManager::EnvironmentMapManager() = default;

EnvironmentMapManager::~EnvironmentMapManager()
{
    // Resources should be released via release() before destruction
}

bool EnvironmentMapManager::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        return true;

    context_ = &context;
    initialized_ = true;

    DBG("EnvironmentMapManager: Initialized");
    return true;
}

void EnvironmentMapManager::release(juce::OpenGLContext& context)
{
    juce::ignoreUnused(context);

    destroyAllMaps();

    if (defaultMap_ != 0)
    {
        glDeleteTextures(1, &defaultMap_);
        defaultMap_ = 0;
    }

    context_ = nullptr;
    initialized_ = false;
}

GLuint EnvironmentMapManager::createProceduralMap(const juce::String& name,
                                                  const EnvironmentConfig& config)
{
    if (!initialized_)
        return 0;

    // Delete existing map with same name
    destroyMap(name);

    // Create cubemap texture
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int res = config.resolution;
    std::vector<float> faceData(static_cast<size_t>(res * res * 3));

    // Generate each cubemap face
    // Faces: +X, -X, +Y, -Y, +Z, -Z
    for (int face = 0; face < 6; ++face)
    {
        generateFaceData(faceData, face, res, config);

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face),
            0,
            GL_RGB16F,
            res, res,
            0,
            GL_RGB,
            GL_FLOAT,
            faceData.data()
        );
    }

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    envMaps_[name] = texture;

    DBG("EnvironmentMapManager: Created map '" << name << "' (" << res << "x" << res << ")");
    return texture;
}

GLuint EnvironmentMapManager::createFromPreset(const juce::String& name,
                                               EnvironmentPreset preset)
{
    return createProceduralMap(name, EnvironmentConfig::fromPreset(preset));
}

GLuint EnvironmentMapManager::getMap(const juce::String& name) const
{
    auto it = envMaps_.find(name);
    return it != envMaps_.end() ? it->second : 0;
}

bool EnvironmentMapManager::hasMap(const juce::String& name) const
{
    return envMaps_.find(name) != envMaps_.end();
}

void EnvironmentMapManager::destroyMap(const juce::String& name)
{
    auto it = envMaps_.find(name);
    if (it != envMaps_.end())
    {
        glDeleteTextures(1, &it->second);
        envMaps_.erase(it);
    }
}

void EnvironmentMapManager::destroyAllMaps()
{
    for (auto& [name, texture] : envMaps_)
    {
        glDeleteTextures(1, &texture);
    }
    envMaps_.clear();
}

GLuint EnvironmentMapManager::getDefaultMap()
{
    if (defaultMap_ != 0)
        return defaultMap_;

    defaultMap_ = createFromPreset("__default__", EnvironmentPreset::Dark);
    return defaultMap_;
}

void EnvironmentMapManager::generateFaceData(std::vector<float>& data,
                                             int face,
                                             int resolution,
                                             const EnvironmentConfig& config)
{
    for (int y = 0; y < resolution; ++y)
    {
        for (int x = 0; x < resolution; ++x)
        {
            // Convert pixel to UV (-1 to 1)
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(resolution) * 2.0f - 1.0f;
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(resolution) * 2.0f - 1.0f;

            // Get direction vector for this pixel
            float dx, dy, dz;
            getCubeDirection(face, u, v, dx, dy, dz);

            // Normalize direction
            float len = std::sqrt(dx*dx + dy*dy + dz*dz);
            dx /= len;
            dy /= len;
            dz /= len;

            // Calculate vertical angle (for sky gradient)
            float vertAngle = std::asin(dy);  // -PI/2 to PI/2
            float vertNorm = (vertAngle / (3.14159265f * 0.5f) + 1.0f) * 0.5f;  // 0 to 1, bottom to top

            // Sky gradient
            float skyT = std::pow(vertNorm, config.skyGradientPower);
            float r = config.skyColorBottomR + (config.skyColorTopR - config.skyColorBottomR) * skyT;
            float g = config.skyColorBottomG + (config.skyColorTopG - config.skyColorBottomG) * skyT;
            float b = config.skyColorBottomB + (config.skyColorTopB - config.skyColorBottomB) * skyT;

            // Horizon glow
            float horizonDist = std::abs(dy);
            float horizonFactor = std::exp(-horizonDist * config.horizonSharpness);
            r = r + (config.horizonColorR - r) * horizonFactor;
            g = g + (config.horizonColorG - g) * horizonFactor;
            b = b + (config.horizonColorB - b) * horizonFactor;

            // Accent light (directional highlight)
            if (config.accentIntensity > 0.0f)
            {
                float accentAngleRad = config.accentAngle * 3.14159265f / 180.0f;
                float accentDirX = 0.0f;
                float accentDirY = std::sin(accentAngleRad);
                float accentDirZ = std::cos(accentAngleRad);

                float accentDot = std::max(0.0f, dx * accentDirX + dy * accentDirY + dz * accentDirZ);
                float accentFactor = std::pow(accentDot, 4.0f) * config.accentIntensity;

                r += config.accentColorR * accentFactor;
                g += config.accentColorG * accentFactor;
                b += config.accentColorB * accentFactor;
            }

            // Store pixel
            size_t idx = static_cast<size_t>((y * resolution + x) * 3);
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
        }
    }
}

void EnvironmentMapManager::getCubeDirection(int face, float u, float v,
                                              float& x, float& y, float& z)
{
    // Face ordering: +X, -X, +Y, -Y, +Z, -Z
    switch (face)
    {
        case 0: // +X
            x = 1.0f; y = -v; z = -u;
            break;
        case 1: // -X
            x = -1.0f; y = -v; z = u;
            break;
        case 2: // +Y
            x = u; y = 1.0f; z = v;
            break;
        case 3: // -Y
            x = u; y = -1.0f; z = -v;
            break;
        case 4: // +Z
            x = u; y = -v; z = 1.0f;
            break;
        case 5: // -Z
            x = -u; y = -v; z = -1.0f;
            break;
        default:
            x = y = z = 0.0f;
            break;
    }
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
