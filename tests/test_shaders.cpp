/*
    Oscil - Shader Tests
*/

#include <gtest/gtest.h>
#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"

namespace oscil
{

TEST(ShaderRegistryTest, HasHoloMatrixShader)
{
    auto& registry = ShaderRegistry::getInstance();
    
    // Verify ID exists
    EXPECT_TRUE(registry.hasShader("holo_matrix"));
    
    // Verify we can get it
    auto* shader = registry.getShader("holo_matrix");
    ASSERT_NE(shader, nullptr);
    
    // Verify properties
    EXPECT_EQ(shader->getId(), "holo_matrix");
    EXPECT_EQ(shader->getDisplayName(), "Holo Matrix");
    EXPECT_FALSE(shader->getDescription().isEmpty());
    
    // Verify list contains it
    auto available = registry.getAvailableShaders();
    bool found = false;
    for (const auto& info : available)
    {
        if (info.id == "holo_matrix")
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

} // namespace oscil
