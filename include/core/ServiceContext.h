/*
    MultiScoper - Service Context
    Aggregates core services for dependency injection
*/

#pragma once

namespace multiscoper
{

class IInstanceRegistry;
class IThemeService;
class ShaderRegistry;
class PresetManager;

struct ServiceContext
{
    IInstanceRegistry& instanceRegistry;
    IThemeService& themeService;
    ShaderRegistry& shaderRegistry;
    PresetManager& presetManager;
};

} // namespace multiscoper
