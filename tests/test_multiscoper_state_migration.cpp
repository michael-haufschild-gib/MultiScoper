/*
    MultiScoper - MultiScoperState Fixture-Driven Migration Tests
    Loads XML fixtures from tests/fixtures/ and verifies post-load behavior.
*/

#include "core/MultiScoperState.h"

#include <juce_core/juce_core.h>

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace
{

/// Locate the fixture file by walking up from the current path.
/// Test binaries run from build/dev/ so fixtures/ is not relative to cwd.
std::filesystem::path findFixture(const std::string& name)
{
    namespace fs = std::filesystem;
    // Candidate roots: cwd, parents up to 6 levels, plus a compile-time path.
    fs::path start = fs::current_path();
    for (int i = 0; i < 8; ++i)
    {
        const fs::path candidate = start / "tests" / "fixtures" / name;
        if (fs::exists(candidate))
            return candidate;
        if (!start.has_parent_path() || start == start.parent_path())
            break;
        start = start.parent_path();
    }
    return {};
}

juce::String loadFixtureText(const std::string& name)
{
    const auto path = findFixture(name);
    if (path.empty())
        return {};
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return juce::String(buffer.str());
}

} // namespace

class MultiScoperStateMigrationFixturesTest : public ::testing::Test
{
protected:
    multiscoper::MultiScoperState state;
};

TEST_F(MultiScoperStateMigrationFixturesTest, V0FixtureLoadsAndIsStampedCurrent)
{
    const auto xml = loadFixtureText("multiscoper_state_v0.xml");
    ASSERT_FALSE(xml.isEmpty()) << "v0 fixture not found on disk";

    EXPECT_TRUE(state.fromXmlString(xml));
    EXPECT_EQ(state.getSchemaVersion(), multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state.getThemeName(), "Pre-Version Theme");
    // Default-fill: schema upgrade must not drop payload fields that did exist.
    EXPECT_EQ(static_cast<int>(state.getColumnLayout()), 2);
}

TEST_F(MultiScoperStateMigrationFixturesTest, V1FixtureLoadsAndIsStampedCurrent)
{
    const auto xml = loadFixtureText("multiscoper_state_v1.xml");
    ASSERT_FALSE(xml.isEmpty()) << "v1 fixture not found on disk";

    EXPECT_TRUE(state.fromXmlString(xml));
    EXPECT_EQ(state.getSchemaVersion(), multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(state.getThemeName(), "V1 Theme");
    EXPECT_EQ(static_cast<int>(state.getColumnLayout()), 3);
    EXPECT_TRUE(state.isShowGridEnabled());
}

TEST_F(MultiScoperStateMigrationFixturesTest, FutureFixtureIsRejectedAndPreservesState)
{
    const auto xml = loadFixtureText("multiscoper_state_v_future.xml");
    ASSERT_FALSE(xml.isEmpty()) << "v_future fixture not found on disk";

    const auto defaultThemeBefore = state.getThemeName();
    EXPECT_FALSE(state.fromXmlString(xml));
    // State must be unchanged: future theme must NOT have been applied.
    EXPECT_EQ(state.getThemeName(), defaultThemeBefore);
    EXPECT_NE(state.getThemeName(), "Future Theme");
    // Version on the live state remains at current, not at 99.
    EXPECT_EQ(state.getSchemaVersion(), multiscoper::MultiScoperState::CURRENT_SCHEMA_VERSION);
}
