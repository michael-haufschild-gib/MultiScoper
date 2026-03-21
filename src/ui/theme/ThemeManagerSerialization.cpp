/*
    Oscil - Theme Manager Serialization
    ColorTheme ValueTree/JSON persistence and ThemeManager file I/O
*/

#include "ui/theme/ThemeManager.h"

namespace oscil
{

// === ColorTheme Serialization ===

juce::ValueTree ColorTheme::toValueTree() const
{
    juce::ValueTree state("Theme");

    state.setProperty("name", name, nullptr);
    state.setProperty("isSystem", isSystemTheme, nullptr);
    state.setProperty("bgPrimary", static_cast<int>(backgroundPrimary.getARGB()), nullptr);
    state.setProperty("bgSecondary", static_cast<int>(backgroundSecondary.getARGB()), nullptr);
    state.setProperty("bgPane", static_cast<int>(backgroundPane.getARGB()), nullptr);
    state.setProperty("gridMajor", static_cast<int>(gridMajor.getARGB()), nullptr);
    state.setProperty("gridMinor", static_cast<int>(gridMinor.getARGB()), nullptr);
    state.setProperty("gridZero", static_cast<int>(gridZeroLine.getARGB()), nullptr);
    state.setProperty("textPrimary", static_cast<int>(textPrimary.getARGB()), nullptr);
    state.setProperty("textSecondary", static_cast<int>(textSecondary.getARGB()), nullptr);
    state.setProperty("textHighlight", static_cast<int>(textHighlight.getARGB()), nullptr);
    state.setProperty("ctrlBg", static_cast<int>(controlBackground.getARGB()), nullptr);
    state.setProperty("ctrlBorder", static_cast<int>(controlBorder.getARGB()), nullptr);
    state.setProperty("ctrlHighlight", static_cast<int>(controlHighlight.getARGB()), nullptr);
    state.setProperty("ctrlActive", static_cast<int>(controlActive.getARGB()), nullptr);
    state.setProperty("statusActive", static_cast<int>(statusActive.getARGB()), nullptr);
    state.setProperty("statusWarning", static_cast<int>(statusWarning.getARGB()), nullptr);
    state.setProperty("statusError", static_cast<int>(statusError.getARGB()), nullptr);

    // Button Primary
    state.setProperty("btnPriBg", static_cast<int>(btnPrimaryBg.getARGB()), nullptr);
    state.setProperty("btnPriBgH", static_cast<int>(btnPrimaryBgHover.getARGB()), nullptr);
    state.setProperty("btnPriBgA", static_cast<int>(btnPrimaryBgActive.getARGB()), nullptr);
    state.setProperty("btnPriBgD", static_cast<int>(btnPrimaryBgDisabled.getARGB()), nullptr);
    state.setProperty("btnPriTxt", static_cast<int>(btnPrimaryText.getARGB()), nullptr);
    state.setProperty("btnPriTxtD", static_cast<int>(btnPrimaryTextDisabled.getARGB()), nullptr);

    // Button Secondary
    state.setProperty("btnSecBg", static_cast<int>(btnSecondaryBg.getARGB()), nullptr);
    state.setProperty("btnSecBgH", static_cast<int>(btnSecondaryBgHover.getARGB()), nullptr);
    state.setProperty("btnSecBgA", static_cast<int>(btnSecondaryBgActive.getARGB()), nullptr);
    state.setProperty("btnSecBgD", static_cast<int>(btnSecondaryBgDisabled.getARGB()), nullptr);
    state.setProperty("btnSecTxt", static_cast<int>(btnSecondaryText.getARGB()), nullptr);
    state.setProperty("btnSecTxtD", static_cast<int>(btnSecondaryTextDisabled.getARGB()), nullptr);

    // Button Tertiary
    state.setProperty("btnTerBg", static_cast<int>(btnTertiaryBg.getARGB()), nullptr);
    state.setProperty("btnTerBgH", static_cast<int>(btnTertiaryBgHover.getARGB()), nullptr);
    state.setProperty("btnTerBgA", static_cast<int>(btnTertiaryBgActive.getARGB()), nullptr);
    state.setProperty("btnTerBgD", static_cast<int>(btnTertiaryBgDisabled.getARGB()), nullptr);
    state.setProperty("btnTerTxt", static_cast<int>(btnTertiaryText.getARGB()), nullptr);
    state.setProperty("btnTerTxtD", static_cast<int>(btnTertiaryTextDisabled.getARGB()), nullptr);

    // Serialize waveform colors
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

    // Button Colors
    btnPrimaryBg = getColour("btnPriBg", btnPrimaryBg);
    btnPrimaryBgHover = getColour("btnPriBgH", btnPrimaryBgHover);
    btnPrimaryBgActive = getColour("btnPriBgA", btnPrimaryBgActive);
    btnPrimaryBgDisabled = getColour("btnPriBgD", btnPrimaryBgDisabled);
    btnPrimaryText = getColour("btnPriTxt", btnPrimaryText);
    btnPrimaryTextDisabled = getColour("btnPriTxtD", btnPrimaryTextDisabled);

    btnPrimaryTextHover = btnPrimaryText;
    btnPrimaryTextActive = btnPrimaryText;

    btnSecondaryBg = getColour("btnSecBg", btnSecondaryBg);
    btnSecondaryBgHover = getColour("btnSecBgH", btnSecondaryBgHover);
    btnSecondaryBgActive = getColour("btnSecBgA", btnSecondaryBgActive);
    btnSecondaryBgDisabled = getColour("btnSecBgD", btnSecondaryBgDisabled);
    btnSecondaryText = getColour("btnSecTxt", btnSecondaryText);
    btnSecondaryTextDisabled = getColour("btnSecTxtD", btnSecondaryTextDisabled);
    btnSecondaryTextHover = btnSecondaryText;
    btnSecondaryTextActive = btnSecondaryText;

    btnTertiaryBg = getColour("btnTerBg", btnTertiaryBg);
    btnTertiaryBgHover = getColour("btnTerBgH", btnTertiaryBgHover);
    btnTertiaryBgActive = getColour("btnTerBgA", btnTertiaryBgActive);
    btnTertiaryBgDisabled = getColour("btnTerBgD", btnTertiaryBgDisabled);
    btnTertiaryText = getColour("btnTerTxt", btnTertiaryText);
    btnTertiaryTextDisabled = getColour("btnTerTxtD", btnTertiaryTextDisabled);
    btnTertiaryTextHover = btnTertiaryText;
    btnTertiaryTextActive = btnTertiaryText;

    // Parse waveform colors
    juce::String colorStr = state.getProperty("waveformColors", "");
    if (colorStr.isNotEmpty())
    {
        waveformColors.clear();
        juce::StringArray colors;
        colors.addTokens(colorStr, ",", "");
        for (const auto& c : colors)
        {
            waveformColors.push_back(juce::Colour(static_cast<juce::uint32>(c.getHexValue32())));
        }
    }
}

juce::String ColorTheme::toJson() const
{
    auto state = toValueTree();
    if (auto xml = state.createXml())
    {
        return xml->toString();
    }
    return {};
}

bool ColorTheme::fromJson(const juce::String& json)
{
    if (auto xml = juce::XmlDocument::parse(json))
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
                if (theme.name.isNotEmpty() && !theme.isSystemTheme)
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

void ThemeManager::deleteThemeFile(const juce::String& themeName)
{
    auto themesDir = getThemesDirectory();
    auto file = themesDir.getChildFile(themeName + ".xml");
    if (file.exists())
    {
        file.deleteFile();
    }
}

} // namespace oscil
