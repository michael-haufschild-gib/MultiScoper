/*
    Oscil - Shader Tests
*/

#include <gtest/gtest.h>
#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"

namespace oscil
{

class ShaderRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
    }

    void TearDown() override
    {
        shaderRegistry_.reset();
    }

    ShaderRegistry& getShaderRegistry()
    {
        return *shaderRegistry_;
    }

private:
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
};

TEST_F(ShaderRegistryTest, HasBasicShader)
{
    auto& registry = getShaderRegistry();

    // Verify basic shader ID exists
    EXPECT_TRUE(registry.hasShader("basic"));

    // Verify we can get it
    auto* shader = registry.getShader("basic");
    ASSERT_NE(shader, nullptr);

    // Verify properties
    EXPECT_EQ(shader->getId(), "basic");
    EXPECT_FALSE(shader->getDisplayName().isEmpty());
}

TEST_F(ShaderRegistryTest, GetAvailableShaders)
{
    auto& registry = getShaderRegistry();

    // Should have at least the basic shader
    auto available = registry.getAvailableShaders();
    EXPECT_GE(available.size(), 1u);

    // Basic shader should be in the list
    bool foundBasic = false;
    for (const auto& info : available)
    {
        if (info.id == "basic")
        {
            foundBasic = true;
            break;
        }
    }
    EXPECT_TRUE(foundBasic);
}

TEST_F(ShaderRegistryTest, GetNonExistentShader)
{
    auto& registry = getShaderRegistry();

    // Getting a non-existent shader should return nullptr
    EXPECT_FALSE(registry.hasShader("nonexistent_shader_xyz"));
    auto* shader = registry.getShader("nonexistent_shader_xyz");
    EXPECT_EQ(shader, nullptr);
}

} // namespace oscil
