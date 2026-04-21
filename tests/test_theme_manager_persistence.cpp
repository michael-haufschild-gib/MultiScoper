/*
    MultiScoper - Theme Manager Persistence Tests
    Tests for theme import/export, JSON, and ValueTree serialization
*/

#include "ui/theme/ThemeManager.h"

#include <gtest/gtest.h>

using namespace multiscoper;

class ThemeManagerPersistenceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tempDir_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("multiscoper_theme_persist_" + juce::String(juce::Time::currentTimeMillis()));
        tempDir_.createDirectory();
        themeManager_ = std::make_unique<ThemeManager>(tempDir_);
    }

    void TearDown() override
    {
        themeManager_.reset();
        tempDir_.deleteRecursively();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    juce::File tempDir_;
    std::unique_ptr<ThemeManager> themeManager_;
};

// =============================================================================
// XML Import/Export Tests (format is XML despite historical "json" naming)
// =============================================================================

// Test: Import invalid XML
TEST_F(ThemeManagerPersistenceTest, ImportInvalidXml)
{
    bool result = getThemeManager().importTheme("not valid xml");
    EXPECT_FALSE(result);
}

// Test: Import empty string
TEST_F(ThemeManagerPersistenceTest, ImportEmptyString)
{
    bool result = getThemeManager().importTheme("");
    EXPECT_FALSE(result);
}

// Test: Import non-XML content is rejected
TEST_F(ThemeManagerPersistenceTest, ImportNonXmlContent)
{
    bool result = getThemeManager().importTheme("{}");
    EXPECT_FALSE(result);
}

// Test: Import cannot overwrite a protected system theme
TEST_F(ThemeManagerPersistenceTest, ImportSystemThemeNameRejected)
{
    ColorTheme t;
    t.name = "Dark Professional"; // built-in system theme
    t.backgroundPrimary = juce::Colour(0xFF123456);
    auto xml = t.toXmlString();

    EXPECT_FALSE(getThemeManager().importTheme(xml));
}

// Test: Import trims whitespace before checking system theme names
TEST_F(ThemeManagerPersistenceTest, ImportTrimmedSystemThemeNameRejected)
{
    ColorTheme t;
    t.name = "  Dark Professional  "; // whitespace-padded system theme name
    t.backgroundPrimary = juce::Colour(0xFF123456);
    auto xml = t.toXmlString();

    EXPECT_FALSE(getThemeManager().importTheme(xml));
}

// Test: Import rejects unsafe filenames
TEST_F(ThemeManagerPersistenceTest, ImportUnsafeNameRejected)
{
    ColorTheme t;
    t.name = "../../../etc/evil";
    auto xml = t.toXmlString();
    EXPECT_FALSE(getThemeManager().importTheme(xml));

    t.name = "theme:with*bad|chars";
    xml = t.toXmlString();
    EXPECT_FALSE(getThemeManager().importTheme(xml));
}

// Test: Export non-existent theme
TEST_F(ThemeManagerPersistenceTest, ExportNonexistentTheme)
{
    juce::String exported = getThemeManager().exportTheme("NonExistentTheme");
    EXPECT_TRUE(exported.isEmpty());
}

// Test: Export then import roundtrip verifies data survives serialization.
// Import refuses silent overwrite, so the test deletes the original first.
TEST_F(ThemeManagerPersistenceTest, ExportImportRoundtrip)
{
    EXPECT_TRUE(getThemeManager().createTheme("RoundtripTest"));
    juce::String exported = getThemeManager().exportTheme("RoundtripTest");
    EXPECT_FALSE(exported.isEmpty());

    EXPECT_TRUE(getThemeManager().deleteTheme("RoundtripTest"));

    // Import the exported theme — should succeed and theme should exist
    bool imported = getThemeManager().importTheme(exported);
    EXPECT_TRUE(imported);

    // Verify the theme is accessible after import
    juce::String exportedAgain = getThemeManager().exportTheme("RoundtripTest");
    EXPECT_FALSE(exportedAgain.isEmpty());
    EXPECT_TRUE(exportedAgain.contains("RoundtripTest"));

    getThemeManager().deleteTheme("RoundtripTest");
}

// Test: import rejects an XML whose declared name collides with an existing
// custom theme. Callers must delete the existing theme first.
TEST_F(ThemeManagerPersistenceTest, ImportRefusesExistingCustomThemeCollision)
{
    EXPECT_TRUE(getThemeManager().createTheme("CollisionTarget"));
    juce::String exported = getThemeManager().exportTheme("CollisionTarget");
    EXPECT_FALSE(exported.isEmpty());

    // Existing theme with same name must block the import.
    EXPECT_FALSE(getThemeManager().importTheme(exported));

    // Deleting allows the import.
    EXPECT_TRUE(getThemeManager().deleteTheme("CollisionTarget"));
    EXPECT_TRUE(getThemeManager().importTheme(exported));

    getThemeManager().deleteTheme("CollisionTarget");
}

// Test: ColorTheme XML export/import with valid data
TEST_F(ThemeManagerPersistenceTest, ColorThemeFromXmlStringValid)
{
    ColorTheme original;
    original.name = "XmlTest";
    original.backgroundPrimary = juce::Colour(0xFF123456);
    original.textPrimary = juce::Colour(0xFFABCDEF);

    juce::String xmlStr = original.toXmlString();

    ColorTheme restored;
    bool result = restored.fromXmlString(xmlStr);

    EXPECT_TRUE(result);
    EXPECT_EQ(restored.name, "XmlTest");
    EXPECT_EQ(restored.backgroundPrimary.getARGB(), 0xFF123456u);
}

// Test: ColorTheme XML import with invalid data
TEST_F(ThemeManagerPersistenceTest, ColorThemeFromXmlStringInvalid)
{
    ColorTheme theme;
    bool result = theme.fromXmlString("invalid xml data");

    EXPECT_FALSE(result);
}

// =============================================================================
// ValueTree Serialization Tests
// =============================================================================

// Test: fromValueTree with empty tree
TEST_F(ThemeManagerPersistenceTest, ValueTreeEmpty)
{
    ColorTheme theme;
    juce::ValueTree emptyTree;

    theme.fromValueTree(emptyTree);

    // After loading empty tree, theme should retain valid defaults.
    EXPECT_FALSE(theme.backgroundPrimary.isTransparent());
    EXPECT_FALSE(theme.textPrimary.isTransparent());
}

// Test: fromValueTree with wrong type
TEST_F(ThemeManagerPersistenceTest, ValueTreeWrongType)
{
    ColorTheme theme;
    theme.name = "OriginalName";
    juce::ValueTree wrongTree("WrongType");
    wrongTree.setProperty("name", "ShouldNotBeUsed", nullptr);

    theme.fromValueTree(wrongTree);

    // Should not crash, and should not modify the theme (wrong type is rejected)
    EXPECT_EQ(theme.name, "OriginalName");
}

// Test: fromValueTree with missing properties
TEST_F(ThemeManagerPersistenceTest, ValueTreeMissingProperties)
{
    ColorTheme theme;
    // Use correct ValueTree type "Theme" (as used by toValueTree)
    juce::ValueTree partialTree("Theme");
    partialTree.setProperty("name", "PartialTheme", nullptr);
    // Missing all color properties - should use defaults

    theme.fromValueTree(partialTree);

    // Name should be read from the tree
    EXPECT_EQ(theme.name, "PartialTheme");

    // Color properties should have default values (not crash)
    EXPECT_NE(theme.backgroundPrimary.getARGB(), 0u);
}

// Test: crosshairLine survives serialization roundtrip
TEST_F(ThemeManagerPersistenceTest, CrosshairLineRoundtrip)
{
    ColorTheme original;
    original.name = "CrosshairTest";
    original.crosshairLine = juce::Colour(0xAABBCCDD);

    auto valueTree = original.toValueTree();

    ColorTheme restored;
    restored.fromValueTree(valueTree);

    EXPECT_EQ(restored.crosshairLine.getARGB(), 0xAABBCCDDu);
}

// Test: all color fields survive serialization roundtrip
TEST_F(ThemeManagerPersistenceTest, AllColorFieldsRoundtrip)
{
    ColorTheme original;
    original.name = "FullRoundtrip";
    original.backgroundPrimary = juce::Colour(0xFF111111);
    original.backgroundSecondary = juce::Colour(0xFF222222);
    original.backgroundPane = juce::Colour(0xFF333333);
    original.gridMajor = juce::Colour(0xFF444444);
    original.gridMinor = juce::Colour(0xFF555555);
    original.gridZeroLine = juce::Colour(0xFF666666);
    original.crosshairLine = juce::Colour(0xFF777777);
    original.textPrimary = juce::Colour(0xFF888888);
    original.textSecondary = juce::Colour(0xFF999999);
    original.textHighlight = juce::Colour(0xFFAAAAAA);
    original.controlBackground = juce::Colour(0xFFBBBBBB);
    original.controlBorder = juce::Colour(0xFFCCCCCC);
    original.controlHighlight = juce::Colour(0xFFDDDDDD);
    original.controlActive = juce::Colour(0xFFEEEEEE);
    original.statusActive = juce::Colour(0xFF101010);
    original.statusWarning = juce::Colour(0xFF202020);
    original.statusError = juce::Colour(0xFF303030);
    original.btnPrimaryBg = juce::Colour(0xFF404040);
    original.btnSecondaryBg = juce::Colour(0xFF505050);
    original.btnTertiaryBg = juce::Colour(0xFF606060);

    auto valueTree = original.toValueTree();

    ColorTheme restored;
    restored.fromValueTree(valueTree);

    EXPECT_EQ(restored.backgroundPrimary.getARGB(), original.backgroundPrimary.getARGB());
    EXPECT_EQ(restored.backgroundSecondary.getARGB(), original.backgroundSecondary.getARGB());
    EXPECT_EQ(restored.backgroundPane.getARGB(), original.backgroundPane.getARGB());
    EXPECT_EQ(restored.gridMajor.getARGB(), original.gridMajor.getARGB());
    EXPECT_EQ(restored.gridMinor.getARGB(), original.gridMinor.getARGB());
    EXPECT_EQ(restored.gridZeroLine.getARGB(), original.gridZeroLine.getARGB());
    EXPECT_EQ(restored.crosshairLine.getARGB(), original.crosshairLine.getARGB());
    EXPECT_EQ(restored.textPrimary.getARGB(), original.textPrimary.getARGB());
    EXPECT_EQ(restored.textSecondary.getARGB(), original.textSecondary.getARGB());
    EXPECT_EQ(restored.textHighlight.getARGB(), original.textHighlight.getARGB());
    EXPECT_EQ(restored.controlBackground.getARGB(), original.controlBackground.getARGB());
    EXPECT_EQ(restored.controlBorder.getARGB(), original.controlBorder.getARGB());
    EXPECT_EQ(restored.controlHighlight.getARGB(), original.controlHighlight.getARGB());
    EXPECT_EQ(restored.controlActive.getARGB(), original.controlActive.getARGB());
    EXPECT_EQ(restored.statusActive.getARGB(), original.statusActive.getARGB());
    EXPECT_EQ(restored.statusWarning.getARGB(), original.statusWarning.getARGB());
    EXPECT_EQ(restored.statusError.getARGB(), original.statusError.getARGB());
    EXPECT_EQ(restored.btnPrimaryBg.getARGB(), original.btnPrimaryBg.getARGB());
    EXPECT_EQ(restored.btnSecondaryBg.getARGB(), original.btnSecondaryBg.getARGB());
    EXPECT_EQ(restored.btnTertiaryBg.getARGB(), original.btnTertiaryBg.getARGB());
}

// =============================================================================
// Glass Field Serialization Tests
// =============================================================================

// Test: glass fields survive round-trip through ValueTree
TEST_F(ThemeManagerPersistenceTest, AccentAndSurfaceFieldsRoundtrip)
{
    ColorTheme original;
    original.name = "AccentRoundtrip";
    original.accentHue = 270.0f;
    original.accentSaturation = 0.85f;
    original.accentLightness = 0.55f;
    original.borderSubtleAlpha = 0.10f;
    original.borderDefaultAlpha = 0.15f;
    original.borderStrongAlpha = 0.25f;
    original.shadowIntensity = 0.5f;
    original.shadowSpread = 16.0f;

    auto valueTree = original.toValueTree();
    ColorTheme restored;
    restored.fromValueTree(valueTree);

    EXPECT_NEAR(restored.accentHue, 270.0f, 0.01f);
    EXPECT_NEAR(restored.accentSaturation, 0.85f, 0.001f);
    EXPECT_NEAR(restored.accentLightness, 0.55f, 0.001f);
    EXPECT_NEAR(restored.borderSubtleAlpha, 0.10f, 0.001f);
    EXPECT_NEAR(restored.borderDefaultAlpha, 0.15f, 0.001f);
    EXPECT_NEAR(restored.borderStrongAlpha, 0.25f, 0.001f);
    EXPECT_NEAR(restored.shadowIntensity, 0.5f, 0.001f);
    EXPECT_NEAR(restored.shadowSpread, 16.0f, 0.01f);
}

TEST_F(ThemeManagerPersistenceTest, AccentFieldsXmlRoundtrip)
{
    ColorTheme original;
    original.name = "AccentXmlRoundtrip";
    original.accentHue = 40.0f;
    original.accentSaturation = 0.8f;
    original.accentLightness = 0.6f;
    original.shadowIntensity = 0.6f;

    juce::String xml = original.toXmlString();
    ASSERT_FALSE(xml.isEmpty());

    ColorTheme restored;
    EXPECT_TRUE(restored.fromXmlString(xml));

    EXPECT_NEAR(restored.accentHue, 40.0f, 0.01f);
    EXPECT_NEAR(restored.accentSaturation, 0.8f, 0.001f);
    EXPECT_NEAR(restored.accentLightness, 0.6f, 0.001f);
    EXPECT_NEAR(restored.shadowIntensity, 0.6f, 0.001f);
}

// When a ValueTree omits the accent/surface-parameter keys, fromValueTree
// falls back to the struct defaults without crashing.
TEST_F(ThemeManagerPersistenceTest, ThemeWithoutAccentFieldsGetsDefaults)
{
    juce::ValueTree state("Theme");
    state.setProperty("name", "MinimalTheme", nullptr);
    state.setProperty("bgPrimary", static_cast<int>(juce::Colour(0xFF1E1E1E).getARGB()), nullptr);
    state.setProperty("textPrimary", static_cast<int>(juce::Colour(0xFFE0E0E0).getARGB()), nullptr);

    ColorTheme theme;
    theme.fromValueTree(state);

    ColorTheme defaults;
    EXPECT_NEAR(theme.accentHue, defaults.accentHue, 0.01f);
    EXPECT_NEAR(theme.accentSaturation, defaults.accentSaturation, 0.001f);
    EXPECT_NEAR(theme.accentLightness, defaults.accentLightness, 0.001f);
    EXPECT_NEAR(theme.borderSubtleAlpha, defaults.borderSubtleAlpha, 0.001f);
    EXPECT_NEAR(theme.borderDefaultAlpha, defaults.borderDefaultAlpha, 0.001f);
    EXPECT_NEAR(theme.borderStrongAlpha, defaults.borderStrongAlpha, 0.001f);
    EXPECT_NEAR(theme.shadowIntensity, defaults.shadowIntensity, 0.001f);
    EXPECT_NEAR(theme.shadowSpread, defaults.shadowSpread, 0.01f);
}

// Test: button Hover/Active text colors fall back to their matching struct
// default (not the plain text variant) when absent from the ValueTree.
// Regression guard for the previous bug where "btnPriTxtH" defaulted to
// btnPrimaryText, masking hover coloration for legacy theme files.
TEST_F(ThemeManagerPersistenceTest, LegacyThemeWithoutButtonHoverActiveTextKeysKeepsDefaults)
{
    juce::ValueTree state("Theme");
    state.setProperty("name", "LegacyButtonTheme", nullptr);
    // Intentionally write ONLY the plain text keys — omit all hover/active
    // variants so the loader must fall back to struct defaults.
    state.setProperty("btnPriTxt", static_cast<int>(juce::Colour(0xFF010203).getARGB()), nullptr);
    state.setProperty("btnSecTxt", static_cast<int>(juce::Colour(0xFF040506).getARGB()), nullptr);
    state.setProperty("btnTerTxt", static_cast<int>(juce::Colour(0xFF070809).getARGB()), nullptr);

    ColorTheme theme;
    theme.fromValueTree(state);

    ColorTheme defaults;
    EXPECT_EQ(theme.btnPrimaryTextHover.getARGB(), defaults.btnPrimaryTextHover.getARGB());
    EXPECT_EQ(theme.btnPrimaryTextActive.getARGB(), defaults.btnPrimaryTextActive.getARGB());
    EXPECT_EQ(theme.btnSecondaryTextHover.getARGB(), defaults.btnSecondaryTextHover.getARGB());
    EXPECT_EQ(theme.btnSecondaryTextActive.getARGB(), defaults.btnSecondaryTextActive.getARGB());
    EXPECT_EQ(theme.btnTertiaryTextHover.getARGB(), defaults.btnTertiaryTextHover.getARGB());
    EXPECT_EQ(theme.btnTertiaryTextActive.getARGB(), defaults.btnTertiaryTextActive.getARGB());
}

// Test: all four new glass system themes are listed in getAvailableThemes()
TEST_F(ThemeManagerPersistenceTest, GlassSystemThemesAvailable)
{
    auto themes = getThemeManager().getAvailableThemes();

    auto contains = [&](const juce::String& name) {
        for (const auto& t : themes)
            if (t == name)
                return true;
        return false;
    };

    EXPECT_TRUE(contains("Glass Dark Blue")) << "Glass Dark Blue missing from available themes";
    EXPECT_TRUE(contains("Glass Dark Purple")) << "Glass Dark Purple missing from available themes";
    EXPECT_TRUE(contains("Glass Dark Brown")) << "Glass Dark Brown missing from available themes";
    EXPECT_TRUE(contains("Glass Dark Black")) << "Glass Dark Black missing from available themes";
}
