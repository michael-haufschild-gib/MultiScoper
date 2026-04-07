/*
    Oscil - Theme Manager Serialization
    ColorTheme ValueTree/JSON persistence and ThemeManager file I/O
*/

#include "ui/theme/ThemeManager.h"

namespace oscil
{

// === ColorTheme Serialization ===

namespace
{

void saveColour(juce::ValueTree& state, const char* key, juce::Colour c)
{
    state.setProperty(key, static_cast<int>(c.getARGB()), nullptr);
}

void saveButtonColors(juce::ValueTree& state, const ColorTheme& t)
{
    saveColour(state, "btnPriBg", t.btnPrimaryBg);
    saveColour(state, "btnPriBgH", t.btnPrimaryBgHover);
    saveColour(state, "btnPriBgA", t.btnPrimaryBgActive);
    saveColour(state, "btnPriBgD", t.btnPrimaryBgDisabled);
    saveColour(state, "btnPriTxt", t.btnPrimaryText);
    saveColour(state, "btnPriTxtH", t.btnPrimaryTextHover);
    saveColour(state, "btnPriTxtA", t.btnPrimaryTextActive);
    saveColour(state, "btnPriTxtD", t.btnPrimaryTextDisabled);

    saveColour(state, "btnSecBg", t.btnSecondaryBg);
    saveColour(state, "btnSecBgH", t.btnSecondaryBgHover);
    saveColour(state, "btnSecBgA", t.btnSecondaryBgActive);
    saveColour(state, "btnSecBgD", t.btnSecondaryBgDisabled);
    saveColour(state, "btnSecTxt", t.btnSecondaryText);
    saveColour(state, "btnSecTxtH", t.btnSecondaryTextHover);
    saveColour(state, "btnSecTxtA", t.btnSecondaryTextActive);
    saveColour(state, "btnSecTxtD", t.btnSecondaryTextDisabled);

    saveColour(state, "btnTerBg", t.btnTertiaryBg);
    saveColour(state, "btnTerBgH", t.btnTertiaryBgHover);
    saveColour(state, "btnTerBgA", t.btnTertiaryBgActive);
    saveColour(state, "btnTerBgD", t.btnTertiaryBgDisabled);
    saveColour(state, "btnTerTxt", t.btnTertiaryText);
    saveColour(state, "btnTerTxtH", t.btnTertiaryTextHover);
    saveColour(state, "btnTerTxtA", t.btnTertiaryTextActive);
    saveColour(state, "btnTerTxtD", t.btnTertiaryTextDisabled);
}

void saveGlassParams(juce::ValueTree& state, const ColorTheme& t)
{
    auto set = [&](const char* key, float val) { state.setProperty(key, static_cast<double>(val), nullptr); };
    set("accentHue", t.accentHue);
    set("accentSaturation", t.accentSaturation);
    set("accentLightness", t.accentLightness);
    set("glassAlpha", t.glassAlpha);
    set("panelAlpha", t.panelAlpha);
    set("blurRadius", t.blurRadius);
    set("borderSubtleAlpha", t.borderSubtleAlpha);
    set("borderDefaultAlpha", t.borderDefaultAlpha);
    set("borderStrongAlpha", t.borderStrongAlpha);
    set("shadowIntensity", t.shadowIntensity);
    set("shadowSpread", t.shadowSpread);
    set("lightEdgeAlpha", t.lightEdgeAlpha);
    set("accentGlowRadius", t.accentGlowRadius);
    set("accentGlowAlpha", t.accentGlowAlpha);
}

using ColourReader = std::function<juce::Colour(const char*, juce::Colour)>;

void loadButtonColors(ColorTheme& t, const ColourReader& getColour)
{
    t.btnPrimaryBg = getColour("btnPriBg", t.btnPrimaryBg);
    t.btnPrimaryBgHover = getColour("btnPriBgH", t.btnPrimaryBgHover);
    t.btnPrimaryBgActive = getColour("btnPriBgA", t.btnPrimaryBgActive);
    t.btnPrimaryBgDisabled = getColour("btnPriBgD", t.btnPrimaryBgDisabled);
    t.btnPrimaryText = getColour("btnPriTxt", t.btnPrimaryText);
    t.btnPrimaryTextHover = getColour("btnPriTxtH", t.btnPrimaryText);
    t.btnPrimaryTextActive = getColour("btnPriTxtA", t.btnPrimaryText);
    t.btnPrimaryTextDisabled = getColour("btnPriTxtD", t.btnPrimaryTextDisabled);

    t.btnSecondaryBg = getColour("btnSecBg", t.btnSecondaryBg);
    t.btnSecondaryBgHover = getColour("btnSecBgH", t.btnSecondaryBgHover);
    t.btnSecondaryBgActive = getColour("btnSecBgA", t.btnSecondaryBgActive);
    t.btnSecondaryBgDisabled = getColour("btnSecBgD", t.btnSecondaryBgDisabled);
    t.btnSecondaryText = getColour("btnSecTxt", t.btnSecondaryText);
    t.btnSecondaryTextHover = getColour("btnSecTxtH", t.btnSecondaryText);
    t.btnSecondaryTextActive = getColour("btnSecTxtA", t.btnSecondaryText);
    t.btnSecondaryTextDisabled = getColour("btnSecTxtD", t.btnSecondaryTextDisabled);

    t.btnTertiaryBg = getColour("btnTerBg", t.btnTertiaryBg);
    t.btnTertiaryBgHover = getColour("btnTerBgH", t.btnTertiaryBgHover);
    t.btnTertiaryBgActive = getColour("btnTerBgA", t.btnTertiaryBgActive);
    t.btnTertiaryBgDisabled = getColour("btnTerBgD", t.btnTertiaryBgDisabled);
    t.btnTertiaryText = getColour("btnTerTxt", t.btnTertiaryText);
    t.btnTertiaryTextHover = getColour("btnTerTxtH", t.btnTertiaryText);
    t.btnTertiaryTextActive = getColour("btnTerTxtA", t.btnTertiaryText);
    t.btnTertiaryTextDisabled = getColour("btnTerTxtD", t.btnTertiaryTextDisabled);
}

void loadWaveformColors(ColorTheme& t, const juce::ValueTree& state)
{
    juce::String const colorStr = state.getProperty("waveformColors", "");
    if (colorStr.isNotEmpty())
    {
        t.waveformColors.clear();
        juce::StringArray colors;
        colors.addTokens(colorStr, ",", "");
        for (const auto& c : colors)
            t.waveformColors.emplace_back(static_cast<juce::uint32>(c.getHexValue32()));
    }
}

} // namespace

juce::ValueTree ColorTheme::toValueTree() const
{
    juce::ValueTree state("Theme");

    state.setProperty("name", name, nullptr);
    state.setProperty("isSystem", isSystemTheme, nullptr);
    saveColour(state, "bgPrimary", backgroundPrimary);
    saveColour(state, "bgSecondary", backgroundSecondary);
    saveColour(state, "bgPane", backgroundPane);
    saveColour(state, "gridMajor", gridMajor);
    saveColour(state, "gridMinor", gridMinor);
    saveColour(state, "gridZero", gridZeroLine);
    saveColour(state, "crosshairLine", crosshairLine);
    saveColour(state, "textPrimary", textPrimary);
    saveColour(state, "textSecondary", textSecondary);
    saveColour(state, "textHighlight", textHighlight);
    saveColour(state, "ctrlBg", controlBackground);
    saveColour(state, "ctrlBorder", controlBorder);
    saveColour(state, "ctrlHighlight", controlHighlight);
    saveColour(state, "ctrlActive", controlActive);
    saveColour(state, "statusActive", statusActive);
    saveColour(state, "statusWarning", statusWarning);
    saveColour(state, "statusError", statusError);
    saveButtonColors(state, *this);
    saveGlassParams(state, *this);

    juce::String colorStr;
    for (const auto& color : waveformColors)
    {
        if (colorStr.isNotEmpty())
            colorStr += ",";
        colorStr += juce::String::toHexString(static_cast<int>(color.getARGB()));
    }
    state.setProperty("waveformColors", colorStr, nullptr);

    return state;
}

void ColorTheme::fromValueTree(const juce::ValueTree& state)
{
    if (!state.hasType("Theme"))
        return;

    name = state.getProperty("name", "Custom");
    isSystemTheme = state.getProperty("isSystem", false);

    auto getColour = [&state](const char* prop, juce::Colour defaultVal) {
        return juce::Colour(static_cast<juce::uint32>(
            static_cast<int>(state.getProperty(prop, static_cast<int>(defaultVal.getARGB())))));
    };

    backgroundPrimary = getColour("bgPrimary", backgroundPrimary);
    backgroundSecondary = getColour("bgSecondary", backgroundSecondary);
    backgroundPane = getColour("bgPane", backgroundPane);
    gridMajor = getColour("gridMajor", gridMajor);
    gridMinor = getColour("gridMinor", gridMinor);
    gridZeroLine = getColour("gridZero", gridZeroLine);
    crosshairLine = getColour("crosshairLine", crosshairLine);
    textPrimary = getColour("textPrimary", textPrimary);
    textSecondary = getColour("textSecondary", textSecondary);
    textHighlight = getColour("textHighlight", textHighlight);
    controlBackground = getColour("ctrlBg", controlBackground);
    controlBorder = getColour("ctrlBorder", controlBorder);
    controlHighlight = getColour("ctrlHighlight", controlHighlight);
    controlActive = getColour("ctrlActive", controlActive);
    statusActive = getColour("statusActive", statusActive);
    statusWarning = getColour("statusWarning", statusWarning);
    statusError = getColour("statusError", statusError);

    loadButtonColors(*this, getColour);

    // Glass / Accent system — use struct defaults when fields are missing (backward compat)
    const ColorTheme defaults;
    auto getFloat = [&state](const char* prop, float defaultVal) {
        return static_cast<float>(static_cast<double>(state.getProperty(prop, static_cast<double>(defaultVal))));
    };
    accentHue = getFloat("accentHue", defaults.accentHue);
    accentSaturation = getFloat("accentSaturation", defaults.accentSaturation);
    accentLightness = getFloat("accentLightness", defaults.accentLightness);
    glassAlpha = getFloat("glassAlpha", defaults.glassAlpha);
    panelAlpha = getFloat("panelAlpha", defaults.panelAlpha);
    blurRadius = getFloat("blurRadius", defaults.blurRadius);
    borderSubtleAlpha = getFloat("borderSubtleAlpha", defaults.borderSubtleAlpha);
    borderDefaultAlpha = getFloat("borderDefaultAlpha", defaults.borderDefaultAlpha);
    borderStrongAlpha = getFloat("borderStrongAlpha", defaults.borderStrongAlpha);
    shadowIntensity = getFloat("shadowIntensity", defaults.shadowIntensity);
    shadowSpread = getFloat("shadowSpread", defaults.shadowSpread);
    lightEdgeAlpha = getFloat("lightEdgeAlpha", defaults.lightEdgeAlpha);
    accentGlowRadius = getFloat("accentGlowRadius", defaults.accentGlowRadius);
    accentGlowAlpha = getFloat("accentGlowAlpha", defaults.accentGlowAlpha);

    loadWaveformColors(*this, state);
}

juce::String ColorTheme::toXmlString() const
{
    auto state = toValueTree();
    if (auto xml = state.createXml())
    {
        return xml->toString();
    }
    return {};
}

bool ColorTheme::fromXmlString(const juce::String& xmlString)
{
    if (auto xml = juce::XmlDocument::parse(xmlString))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid())
        {
            fromValueTree(state);
            return true;
        }
    }
    return false;
}

// === ThemeManager File I/O ===

juce::File ThemeManager::getThemesDirectory() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appDataDir.getChildFile("Oscil").getChildFile("themes");
}

void ThemeManager::loadThemes()
{
    auto themesDir = getThemesDirectory();
    if (!themesDir.exists())
        return;

    for (const auto& file : themesDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
    {
        if (auto xml = juce::XmlDocument::parse(file))
        {
            auto loadedTree = juce::ValueTree::fromXml(*xml);
            if (loadedTree.isValid())
            {
                ColorTheme theme;
                theme.fromValueTree(loadedTree);
                if (isValidThemeName(theme.name) && !theme.isSystemTheme)
                {
                    themes_[theme.name] = theme;
                }
            }
        }
    }
}

void ThemeManager::saveThemes()
{
    for (const auto& [name, theme] : themes_)
    {
        if (!theme.isSystemTheme)
        {
            pendingSaves_.insert(name);
        }
    }
    startTimer(500);
}

void ThemeManager::saveTheme(const juce::String& themeName)
{
    auto it = themes_.find(themeName);
    if (it == themes_.end() || it->second.isSystemTheme)
        return;

    pendingSaves_.insert(themeName);
    startTimer(500);
}

void ThemeManager::deleteThemeFile(const juce::String& themeName) const
{
    auto themesDir = getThemesDirectory();
    auto file = themesDir.getChildFile(themeName + ".xml");
    if (file.exists())
    {
        file.deleteFile();
    }
}

} // namespace oscil
