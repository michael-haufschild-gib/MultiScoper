/*
    Oscil - Service Context
    Aggregates core services for dependency injection
*/

#pragma once

namespace oscil
{

class IInstanceRegistry;
class IThemeService;

struct ServiceContext
{
    IInstanceRegistry& instanceRegistry;
    IThemeService& themeService;
};

} // namespace oscil
