/*
    Oscil - Particle Serialization Implementation
*/

#include "rendering/serialization/ParticleSerialization.h"

namespace oscil
{

namespace
{
    static const juce::Identifier PARTICLES_TYPE("Particles");

    juce::String particleEmissionModeToStringLocal(ParticleEmissionMode mode)
    {
        switch (mode)
        {
            case ParticleEmissionMode::AlongWaveform:   return "along_waveform";
            case ParticleEmissionMode::AtPeaks:         return "at_peaks";
            case ParticleEmissionMode::AtZeroCrossings: return "at_zero_crossings";
            case ParticleEmissionMode::Continuous:      return "continuous";
            case ParticleEmissionMode::Burst:           return "burst";
            default:                                    return "along_waveform";
        }
    }

    ParticleEmissionMode stringToParticleEmissionModeLocal(const juce::String& str)
    {
        if (str == "at_peaks")           return ParticleEmissionMode::AtPeaks;
        if (str == "at_zero_crossings")  return ParticleEmissionMode::AtZeroCrossings;
        if (str == "continuous")         return ParticleEmissionMode::Continuous;
        if (str == "burst")              return ParticleEmissionMode::Burst;
        return ParticleEmissionMode::AlongWaveform;
    }

    juce::String particleBlendModeToStringLocal(ParticleBlendMode mode)
    {
        switch (mode)
        {
            case ParticleBlendMode::Additive: return "additive";
            case ParticleBlendMode::Alpha:    return "alpha";
            case ParticleBlendMode::Multiply: return "multiply";
            case ParticleBlendMode::Screen:   return "screen";
            default:                          return "additive";
        }
    }

    ParticleBlendMode stringToParticleBlendModeLocal(const juce::String& str)
    {
        if (str == "alpha")    return ParticleBlendMode::Alpha;
        if (str == "multiply") return ParticleBlendMode::Multiply;
        if (str == "screen")   return ParticleBlendMode::Screen;
        return ParticleBlendMode::Additive;
    }

    juce::String colorToHexLocal(juce::Colour colour)
    {
        return juce::String::toHexString(static_cast<int>(colour.getARGB()));
    }

    juce::Colour hexToColorLocal(const juce::String& hex)
    {
        return juce::Colour(static_cast<juce::uint32>(hex.getHexValue64()));
    }
}

juce::ValueTree ParticleSerialization::toValueTree(const ParticleSettings& particles)
{
    juce::ValueTree particlesTree(PARTICLES_TYPE);
    particlesTree.setProperty("enabled", particles.enabled, nullptr);
    particlesTree.setProperty("emissionMode", static_cast<int>(particles.emissionMode), nullptr);
    particlesTree.setProperty("emissionRate", particles.emissionRate, nullptr);
    particlesTree.setProperty("particleLife", particles.particleLife, nullptr);
    particlesTree.setProperty("particleSize", particles.particleSize, nullptr);
    particlesTree.setProperty("particleColor", static_cast<juce::int64>(particles.particleColor.getARGB()), nullptr);
    particlesTree.setProperty("blendMode", static_cast<int>(particles.blendMode), nullptr);
    particlesTree.setProperty("gravity", particles.gravity, nullptr);
    particlesTree.setProperty("drag", particles.drag, nullptr);
    particlesTree.setProperty("randomness", particles.randomness, nullptr);
    particlesTree.setProperty("velocityScale", particles.velocityScale, nullptr);
    particlesTree.setProperty("audioReactive", particles.audioReactive, nullptr);
    particlesTree.setProperty("audioEmissionBoost", particles.audioEmissionBoost, nullptr);
    particlesTree.setProperty("textureId", particles.textureId, nullptr);
    particlesTree.setProperty("textureRows", particles.textureRows, nullptr);
    particlesTree.setProperty("textureCols", particles.textureCols, nullptr);
    particlesTree.setProperty("softParticles", particles.softParticles, nullptr);
    particlesTree.setProperty("softDepthSensitivity", particles.softDepthSensitivity, nullptr);
    particlesTree.setProperty("useTurbulence", particles.useTurbulence, nullptr);
    particlesTree.setProperty("turbulenceStrength", particles.turbulenceStrength, nullptr);
    particlesTree.setProperty("turbulenceScale", particles.turbulenceScale, nullptr);
    particlesTree.setProperty("turbulenceSpeed", particles.turbulenceSpeed, nullptr);
    return particlesTree;
}

ParticleSettings ParticleSerialization::fromValueTree(const juce::ValueTree& tree)
{
    ParticleSettings particles;
    // If tree is root, find child. If tree is PARTICLES_TYPE, use it directly.
    juce::ValueTree particlesTree = tree.hasType(PARTICLES_TYPE) ? tree : tree.getChildWithName(PARTICLES_TYPE);
    
    if (particlesTree.isValid())
    {
        particles.enabled = particlesTree.getProperty("enabled", false);
        particles.emissionMode = static_cast<ParticleEmissionMode>(
            static_cast<int>(particlesTree.getProperty("emissionMode", 0)));
        particles.emissionRate = particlesTree.getProperty("emissionRate", 100.0f);
        particles.particleLife = particlesTree.getProperty("particleLife", 2.0f);
        particles.particleSize = particlesTree.getProperty("particleSize", 4.0f);
        particles.particleColor = juce::Colour(static_cast<juce::uint32>(
            static_cast<juce::int64>(particlesTree.getProperty("particleColor", static_cast<juce::int64>(0xFFFFAA00)))));
        particles.blendMode = static_cast<ParticleBlendMode>(
            static_cast<int>(particlesTree.getProperty("blendMode", 0)));
        particles.gravity = particlesTree.getProperty("gravity", 0.0f);
        particles.drag = particlesTree.getProperty("drag", 0.1f);
        particles.randomness = particlesTree.getProperty("randomness", 0.5f);
        particles.velocityScale = particlesTree.getProperty("velocityScale", 1.0f);
        particles.audioReactive = particlesTree.getProperty("audioReactive", true);
        particles.audioEmissionBoost = particlesTree.getProperty("audioEmissionBoost", 2.0f);
        particles.textureId = particlesTree.getProperty("textureId", "").toString();
        particles.textureRows = particlesTree.getProperty("textureRows", 1);
        particles.textureCols = particlesTree.getProperty("textureCols", 1);
        particles.softParticles = particlesTree.getProperty("softParticles", false);
        particles.softDepthSensitivity = particlesTree.getProperty("softDepthSensitivity", 1.0f);
        particles.useTurbulence = particlesTree.getProperty("useTurbulence", false);
        particles.turbulenceStrength = particlesTree.getProperty("turbulenceStrength", 0.0f);
        particles.turbulenceScale = particlesTree.getProperty("turbulenceScale", 0.5f);
        particles.turbulenceSpeed = particlesTree.getProperty("turbulenceSpeed", 0.5f);
    }
    return particles;
}

juce::var ParticleSerialization::toJson(const ParticleSettings& particles)
{
    auto particlesObj = new juce::DynamicObject();
    particlesObj->setProperty("enabled", particles.enabled);
    particlesObj->setProperty("emission_mode", particleEmissionModeToStringLocal(particles.emissionMode));
    particlesObj->setProperty("emission_rate", particles.emissionRate);
    particlesObj->setProperty("particle_life", particles.particleLife);
    particlesObj->setProperty("particle_size", particles.particleSize);
    particlesObj->setProperty("particle_color", colorToHexLocal(particles.particleColor));
    particlesObj->setProperty("blend_mode", particleBlendModeToStringLocal(particles.blendMode));
    particlesObj->setProperty("gravity", particles.gravity);
    particlesObj->setProperty("drag", particles.drag);
    particlesObj->setProperty("randomness", particles.randomness);
    particlesObj->setProperty("velocity_scale", particles.velocityScale);
    particlesObj->setProperty("audio_reactive", particles.audioReactive);
    particlesObj->setProperty("audio_emission_boost", particles.audioEmissionBoost);
    particlesObj->setProperty("texture_id", particles.textureId);
    particlesObj->setProperty("texture_rows", particles.textureRows);
    particlesObj->setProperty("texture_cols", particles.textureCols);
    particlesObj->setProperty("soft_particles", particles.softParticles);
    particlesObj->setProperty("soft_depth_sensitivity", particles.softDepthSensitivity);
    particlesObj->setProperty("use_turbulence", particles.useTurbulence);
    particlesObj->setProperty("turbulence_strength", particles.turbulenceStrength);
    particlesObj->setProperty("turbulence_scale", particles.turbulenceScale);
    particlesObj->setProperty("turbulence_speed", particles.turbulenceSpeed);
    return juce::var(particlesObj);
}

ParticleSettings ParticleSerialization::fromJson(const juce::var& json)
{
    ParticleSettings particles;
    // Check if json is the particles object itself or root object containing particles
    const juce::DynamicObject* particlesObj = nullptr;
    
    if (auto* rootObj = json.getDynamicObject())
    {
        if (rootObj->hasProperty("emission_mode")) {
            particlesObj = rootObj;
        } else if (auto* prop = rootObj->getProperty("particles").getDynamicObject()) {
            particlesObj = prop;
        }
    }

    if (particlesObj)
    {
        particles.enabled = particlesObj->getProperty("enabled");
        particles.emissionMode = stringToParticleEmissionModeLocal(particlesObj->getProperty("emission_mode").toString());
        particles.emissionRate = static_cast<float>(static_cast<double>(particlesObj->getProperty("emission_rate")));
        particles.particleLife = static_cast<float>(static_cast<double>(particlesObj->getProperty("particle_life")));
        particles.particleSize = static_cast<float>(static_cast<double>(particlesObj->getProperty("particle_size")));
        particles.particleColor = hexToColorLocal(particlesObj->getProperty("particle_color").toString());
        particles.blendMode = stringToParticleBlendModeLocal(particlesObj->getProperty("blend_mode").toString());
        particles.gravity = static_cast<float>(static_cast<double>(particlesObj->getProperty("gravity")));
        particles.drag = static_cast<float>(static_cast<double>(particlesObj->getProperty("drag")));
        particles.randomness = static_cast<float>(static_cast<double>(particlesObj->getProperty("randomness")));
        particles.velocityScale = static_cast<float>(static_cast<double>(particlesObj->getProperty("velocity_scale")));
        particles.audioReactive = particlesObj->getProperty("audio_reactive");
        particles.audioEmissionBoost = static_cast<float>(static_cast<double>(particlesObj->getProperty("audio_emission_boost")));
        particles.textureId = particlesObj->getProperty("texture_id").toString();
        particles.textureRows = static_cast<int>(particlesObj->getProperty("texture_rows"));
        particles.textureCols = static_cast<int>(particlesObj->getProperty("texture_cols"));
        particles.softParticles = particlesObj->getProperty("soft_particles");
        particles.softDepthSensitivity = static_cast<float>(static_cast<double>(particlesObj->getProperty("soft_depth_sensitivity")));
        particles.useTurbulence = particlesObj->getProperty("use_turbulence");
        particles.turbulenceStrength = static_cast<float>(static_cast<double>(particlesObj->getProperty("turbulence_strength")));
        particles.turbulenceScale = static_cast<float>(static_cast<double>(particlesObj->getProperty("turbulence_scale")));
        particles.turbulenceSpeed = static_cast<float>(static_cast<double>(particlesObj->getProperty("turbulence_speed")));
    }
    return particles;
}

} // namespace oscil
