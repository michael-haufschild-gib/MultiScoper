/*
    Oscil - Theme Manager Implementation
*/

#include "ui/theme/ThemeManager.h"

namespace oscil
{

// ... (ColorTheme implementation skipped for brevity, it is unchanged) ...
// Wait, I must provide the FULL content for write_file or use replace.
// Since I have the full content in history, I will reconstruct it.
// Actually, ColorTheme methods are at the top. I should keep them.

void ColorTheme::initializeDefaultWaveformColors()
{
    static const std::vector<juce::Colour> defaultColors = {
        juce::Colour(0xFF00FF00),  // Green
        juce::Colour(0xFF00FFFF),  // Cyan
        juce::Colour(0xFFFF00FF),  // Magenta
        juce::Colour(0xFFFFFF00),  // Yellow
        juce::Colour(0xFFFF8000),  // Orange
        juce::Colour(0xFF8080FF),  // Light blue
        juce::Colour(0xFFFF8080),  // Light red
        juce::Colour(0xFF80FF80),  // Light green
        juce::Colour(0xFFFFFFFF),  // White
        juce::Colour(0xFF00FF80),  // Spring green
        juce::Colour(0xFF8000FF),  // Purple
        juce::Colour(0xFFFF0080),  // Pink
        juce::Colour(0xFF80FFFF),  // Light cyan
        juce::Colour(0xFFFFFF80),  // Light yellow
        juce::Colour(0xFFFF80FF),  // Light magenta
        juce::Colour(0xFF0080FF),  // Sky blue
    };

    waveformColors = defaultColors;

    // Generate additional colors to reach 64
    while (waveformColors.size() < 64)
    {
        float hue = static_cast<float>(waveformColors.size()) / 64.0f;
        waveformColors.push_back(juce::Colour::fromHSV(hue, 0.8f, 1.0f, 1.0f));
    }
}

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

// ThemeManager implementation

ThemeManager::ThemeManager()
{
    initializeSystemThemes();
    loadThemes();

    // Set default theme
    if (themes_.find("Dark Professional") != themes_.end())
    {
        currentTheme_ = themes_["Dark Professional"];
    }
}

ThemeManager::~ThemeManager()
{
    // Synchronously flush any pending saves before destruction
    stopTimer();
    
    // Process pending saves synchronously
    if (!pendingSaves_.empty())
    {
        auto themesDir = getThemesDirectory();
        themesDir.createDirectory();

        for (const auto& name : pendingSaves_)
        {
            auto it = themes_.find(name);
            if (it != themes_.end() && !it->second.isSystemTheme)
            {
                auto file = themesDir.getChildFile(name + ".xml");
                if (auto xml = it->second.toValueTree().createXml())
                {
                    xml->writeTo(file);
                }
            }
        }
    }
}

void ThemeManager::timerCallback()
{
    stopTimer();
    flushPendingSaves();
}

void ThemeManager::flushPendingSaves()
{
    if (pendingSaves_.empty())
        return;

    // Prepare data for background thread
    std::vector<std::pair<juce::String, juce::String>> filesToWrite;
    auto themesDir = getThemesDirectory();

    // Capture current state on main thread
    for (const auto& name : pendingSaves_)
    {
        auto it = themes_.find(name);
        if (it != themes_.end() && !it->second.isSystemTheme)
        {
            if (auto xml = it->second.toValueTree().createXml())
            {
                filesToWrite.push_back({
                    themesDir.getChildFile(name + ".xml").getFullPathName(),
                    xml->toString()
                });
            }
        }
    }
    
    pendingSaves_.clear();

    if (filesToWrite.empty())
        return;

    // Launch background thread for I/O
    juce::Thread::launch([filesToWrite]() {
        for (const auto& [path, content] : filesToWrite)
        {
            juce::File file(path);
            file.getParentDirectory().createDirectory();
            file.replaceWithText(content);
        }
    });
}

void ThemeManager::initializeSystemThemes()
{
    themes_["Dark Professional"] = SystemThemes::createDarkProfessional();
    themes_["Classic Green"] = SystemThemes::createClassicGreen();
    themes_["Classic Amber"] = SystemThemes::createClassicAmber();
    themes_["High Contrast"] = SystemThemes::createHighContrast();
    themes_["Light Mode"] = SystemThemes::createLightMode();
}

bool ThemeManager::setCurrentTheme(const juce::String& themeName)
{
    auto it = themes_.find(themeName);
    if (it == themes_.end())
        return false;

    currentTheme_ = it->second;
    notifyListeners();
    return true;
}

std::vector<juce::String> ThemeManager::getAvailableThemes() const
{
    std::vector<juce::String> result;
    for (const auto& [name, theme] : themes_)
    {
        result.push_back(name);
    }
    std::sort(result.begin(), result.end());
    return result;
}

const ColorTheme* ThemeManager::getTheme(const juce::String& themeName) const
{
    auto it = themes_.find(themeName);
    return it != themes_.end() ? &it->second : nullptr;
}

bool ThemeManager::createTheme(const juce::String& name, const juce::String& sourceTheme)
{
    if (themes_.find(name) != themes_.end())
        return false;

    ColorTheme newTheme;
    if (sourceTheme.isNotEmpty())
    {
        auto it = themes_.find(sourceTheme);
        if (it != themes_.end())
            newTheme = it->second;
    }

    newTheme.name = name;
    newTheme.isSystemTheme = false;
    themes_[name] = newTheme;

    // Save immediately (queued)
    saveTheme(name);
    return true;
}

bool ThemeManager::updateTheme(const juce::String& name, const ColorTheme& theme)
{
    auto it = themes_.find(name);
    if (it == themes_.end() || it->second.isSystemTheme)
        return false;

    it->second = theme;
    it->second.name = name;
    it->second.isSystemTheme = false;

    // Save immediately (queued)
    saveTheme(name);

    if (currentTheme_.name == name)
    {
        currentTheme_ = it->second;
        notifyListeners();
    }

    return true;
}

bool ThemeManager::deleteTheme(const juce::String& name)
{
    auto it = themes_.find(name);
    if (it == themes_.end() || it->second.isSystemTheme)
        return false;

    if (currentTheme_.name == name)
    {
        for (const auto& [themeName, theme] : themes_)
        {
            if (theme.isSystemTheme)
            {
                currentTheme_ = theme;
                notifyListeners();
                break;
            }
        }
    }

    themes_.erase(it);
    
    // Remove from pending saves if present
    pendingSaves_.erase(name);

    // Delete theme file from disk
    deleteThemeFile(name);
    return true;
}

bool ThemeManager::cloneTheme(const juce::String& sourceName, const juce::String& newName)
{
    auto it = themes_.find(sourceName);
    if (it == themes_.end())
        return false;

    if (themes_.find(newName) != themes_.end())
        return false;

    ColorTheme clone = it->second;
    clone.name = newName;
    clone.isSystemTheme = false;
    themes_[newName] = clone;

    saveTheme(newName);
    return true;
}

bool ThemeManager::isSystemTheme(const juce::String& name) const
{
    auto it = themes_.find(name);
    return it != themes_.end() && it->second.isSystemTheme;
}

bool ThemeManager::importTheme(const juce::String& json)
{
    ColorTheme theme;
    if (!theme.fromJson(json))
        return false;

    if (theme.name.isEmpty())
        return false;

    theme.isSystemTheme = false;
    themes_[theme.name] = theme;

    saveTheme(theme.name);
    return true;
}

juce::String ThemeManager::exportTheme(const juce::String& name) const
{
    auto it = themes_.find(name);
    if (it == themes_.end())
        return {};

    return it->second.toJson();
}

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
    // Queue all custom themes for saving
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

    // Queue for async save with debounce
    pendingSaves_.insert(themeName);
    startTimer(500); // 500ms debounce
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

void ThemeManager::addListener(ThemeManagerListener* listener)
{
    listeners_.add(listener);
}

void ThemeManager::removeListener(ThemeManagerListener* listener)
{
    listeners_.remove(listener);
}

void ThemeManager::notifyListeners()
{
    // Ensure theme change notifications happen on the message thread
    // since listeners will typically call repaint() which requires message thread
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        listeners_.call([this](ThemeManagerListener& listener) {
            listener.themeChanged(currentTheme_);
        });
    }
    else
    {
        // Capture theme by value to avoid race conditions
        // Use WeakReference to prevent use-after-free if ThemeManager is destroyed
        // before the async callback runs
        auto themeCopy = currentTheme_;
        juce::WeakReference<ThemeManager> weakThis(this);
        juce::MessageManager::callAsync([weakThis, themeCopy]() {
            if (auto* self = weakThis.get())
            {
                self->listeners_.call([&themeCopy](ThemeManagerListener& listener) {
                    listener.themeChanged(themeCopy);
                });
            }
        });
    }
}

// System theme definitions (unchanged)
namespace SystemThemes
{
    // ... (same as original)
    ColorTheme createDarkProfessional()
    {
        ColorTheme theme;
        theme.name = "Dark Professional";
        theme.isSystemTheme = true;
        // ... (omitted values for brevity, assumed to be in original file or included via copy-paste if I was replacing full file. 
        // Wait, I must replace the FULL file. I will put the values back.)
        
        theme.backgroundPrimary = juce::Colour(0xFF1E1E1E);
        theme.backgroundSecondary = juce::Colour(0xFF2D2D2D);
        theme.backgroundPane = juce::Colour(0xFF252525);
        theme.gridMajor = juce::Colour(0xFF3A3A3A);
        theme.gridMinor = juce::Colour(0xFF2A2A2A);
        theme.gridZeroLine = juce::Colour(0xFF4A4A4A);
        theme.textPrimary = juce::Colour(0xFFE0E0E0);
        theme.textSecondary = juce::Colour(0xFFA0A0A0);
        theme.textHighlight = juce::Colour(0xFFFFFFFF);
        theme.controlBackground = juce::Colour(0xFF353535);
        theme.controlBorder = juce::Colour(0xFF454545);
        theme.controlHighlight = juce::Colour(0xFF505050);
        theme.controlActive = juce::Colour(0xFF007ACC);
        theme.statusActive = juce::Colour(0xFF00CC00);
        theme.statusWarning = juce::Colour(0xFFCCAA00);
        theme.statusError = juce::Colour(0xFFCC0000);
        
        theme.btnPrimaryBg = juce::Colour(0xFF007ACC);
        theme.btnPrimaryBgHover = juce::Colour(0xFF008AD9);
        theme.btnPrimaryBgActive = juce::Colour(0xFF0062A3);
        theme.btnPrimaryBgDisabled = juce::Colour(0xFF353535);
        theme.btnPrimaryText = juce::Colour(0xFFFFFFFF);
        theme.btnPrimaryTextDisabled = juce::Colour(0xFFA0A0A0);

        theme.btnSecondaryBg = juce::Colour(0xFF3A3A3A);
        theme.btnSecondaryBgHover = juce::Colour(0xFF454545);
        theme.btnSecondaryBgActive = juce::Colour(0xFF303030);
        theme.btnSecondaryBgDisabled = juce::Colour(0xFF252525);
        theme.btnSecondaryText = juce::Colour(0xFFE0E0E0);
        theme.btnSecondaryTextDisabled = juce::Colour(0xFF606060);

        theme.btnTertiaryBg = juce::Colours::transparentBlack;
        theme.btnTertiaryBgHover = juce::Colour(0x1AFFFFFF);
        theme.btnTertiaryBgActive = juce::Colour(0x33FFFFFF);
        theme.btnTertiaryBgDisabled = juce::Colours::transparentBlack;
        theme.btnTertiaryText = juce::Colour(0xFFE0E0E0);
        theme.btnTertiaryTextDisabled = juce::Colour(0xFF606060);
        return theme;
    }

    ColorTheme createClassicGreen()
    {
        ColorTheme theme;
        theme.name = "Classic Green";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFF0A0A0A);
        theme.backgroundSecondary = juce::Colour(0xFF151515);
        theme.backgroundPane = juce::Colour(0xFF0D0D0D);
        theme.gridMajor = juce::Colour(0xFF1A3A1A);
        theme.gridMinor = juce::Colour(0xFF0D1F0D);
        theme.gridZeroLine = juce::Colour(0xFF2A4A2A);
        theme.textPrimary = juce::Colour(0xFF00FF00);
        theme.textSecondary = juce::Colour(0xFF00AA00);
        theme.textHighlight = juce::Colour(0xFF00FF00);
        theme.controlBackground = juce::Colour(0xFF1A1A1A);
        theme.controlBorder = juce::Colour(0xFF00AA00);
        theme.controlHighlight = juce::Colour(0xFF003300);
        theme.controlActive = juce::Colour(0xFF00FF00);
        
        theme.waveformColors.clear();
        for (int i = 0; i < 64; ++i) {
            float brightness = 0.5f + 0.5f * (static_cast<float>(i) / 64.0f);
            theme.waveformColors.push_back(juce::Colour::fromHSV(0.33f, 1.0f, brightness, 1.0f));
        }
        return theme;
    }

    ColorTheme createClassicAmber()
    {
        ColorTheme theme;
        theme.name = "Classic Amber";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFF0A0A00);
        theme.backgroundSecondary = juce::Colour(0xFF151508);
        theme.backgroundPane = juce::Colour(0xFF0D0D05);
        theme.gridMajor = juce::Colour(0xFF3A3A1A);
        theme.gridMinor = juce::Colour(0xFF1F1F0D);
        theme.gridZeroLine = juce::Colour(0xFF4A4A2A);
        theme.textPrimary = juce::Colour(0xFFFFAA00);
        theme.textSecondary = juce::Colour(0xFFAA7700);
        theme.textHighlight = juce::Colour(0xFFFFCC00);
        theme.controlBackground = juce::Colour(0xFF1A1A0A);
        theme.controlBorder = juce::Colour(0xFFAA7700);
        theme.controlHighlight = juce::Colour(0xFF332200);
        theme.controlActive = juce::Colour(0xFFFFAA00);
        
        theme.waveformColors.clear();
        for (int i = 0; i < 64; ++i) {
            float hue = 0.08f + 0.04f * std::sin(static_cast<float>(i) * 0.2f);
            theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 1.0f, 1.0f, 1.0f));
        }
        return theme;
    }

    ColorTheme createHighContrast()
    {
        ColorTheme theme;
        theme.name = "High Contrast";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFF000000);
        theme.backgroundSecondary = juce::Colour(0xFF000000);
        theme.backgroundPane = juce::Colour(0xFF000000);
        theme.gridMajor = juce::Colour(0xFF404040);
        theme.gridMinor = juce::Colour(0xFF202020);
        theme.gridZeroLine = juce::Colour(0xFF606060);
        theme.textPrimary = juce::Colour(0xFFFFFFFF);
        theme.textSecondary = juce::Colour(0xFFCCCCCC);
        theme.textHighlight = juce::Colour(0xFFFFFF00);
        theme.controlBackground = juce::Colour(0xFF000000);
        theme.controlBorder = juce::Colour(0xFFFFFFFF);
        theme.controlHighlight = juce::Colour(0xFF404040);
        theme.controlActive = juce::Colour(0xFFFFFF00);
        return theme;
    }

    ColorTheme createLightMode()
    {
        ColorTheme theme;
        theme.name = "Light Mode";
        theme.isSystemTheme = true;
        theme.backgroundPrimary = juce::Colour(0xFFF5F5F5);
        theme.backgroundSecondary = juce::Colour(0xFFE8E8E8);
        theme.backgroundPane = juce::Colour(0xFFFFFFFF);
        theme.gridMajor = juce::Colour(0xFFCCCCCC);
        theme.gridMinor = juce::Colour(0xFFE0E0E0);
        theme.gridZeroLine = juce::Colour(0xFFAAAAAA);
        theme.textPrimary = juce::Colour(0xFF202020);
        theme.textSecondary = juce::Colour(0xFF606060);
        theme.textHighlight = juce::Colour(0xFF000000);
        theme.controlBackground = juce::Colour(0xFFFFFFFF);
        theme.controlBorder = juce::Colour(0xFFCCCCCC);
        theme.controlHighlight = juce::Colour(0xFFE0E0E0);
        theme.controlActive = juce::Colour(0xFF0066CC);
        
        theme.waveformColors.clear();
        theme.waveformColors = {
            juce::Colour(0xFF007700), juce::Colour(0xFF007777), juce::Colour(0xFF770077), juce::Colour(0xFF777700),
            juce::Colour(0xFFCC5500), juce::Colour(0xFF0000CC), juce::Colour(0xFFCC0000), juce::Colour(0xFF00CC00),
        };
        while (theme.waveformColors.size() < 64) {
            float hue = static_cast<float>(theme.waveformColors.size()) / 64.0f;
            theme.waveformColors.push_back(juce::Colour::fromHSV(hue, 0.9f, 0.6f, 1.0f));
        }
        return theme;
    }
}

} // namespace oscil