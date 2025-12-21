#include "OscilTestFixtures.h"
#include "rendering/VisualConfiguration.h"
#include <gtest/gtest.h>

namespace oscil
{

class VisualConfigDeserializationTest : public ::testing::Test
{
};

TEST_F(VisualConfigDeserializationTest, LoadInvalidBlendMode)
{
    juce::ValueTree tree("VisualConfiguration");
    tree.setProperty("compositeBlendMode", 999, nullptr);

    auto config = VisualConfiguration::fromValueTree(tree);

    // Should default to Alpha (0)
    EXPECT_EQ(config.compositeBlendMode, BlendMode::Alpha);
}

}
