/*
    Oscil - Shader Registry Tests
    Verifies shader registration, lookup, factory creation, and invariants
    that all built-in shaders must satisfy.
*/

#include "rendering/ShaderRegistry.h"
#include "rendering/WaveformShader.h"

#include <gtest/gtest.h>
#include <set>

namespace oscil
{

class ShaderRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override { registry_ = std::make_unique<ShaderRegistry>(); }

    ShaderRegistry& registry() { return *registry_; }

private:
    std::unique_ptr<ShaderRegistry> registry_;
};

// Bug caught: registerBuiltInShaders() silently fails, leaving registry empty.
// User sees a blank shader dropdown and waveforms render with no shader.
TEST_F(ShaderRegistryTest, RegistryContainsAtLeastOneShader)
{
    auto available = registry().getAvailableShaders();
    EXPECT_GE(available.size(), 1u) << "Registry must register at least the 'basic' shader";
}

// Bug caught: default shader ID refers to a shader that wasn't registered,
// causing nullptr dereference when rendering the first waveform.
TEST_F(ShaderRegistryTest, DefaultShaderIdReferencesRegisteredShader)
{
    juce::String defaultId = registry().getDefaultShaderId();
    EXPECT_FALSE(defaultId.isEmpty()) << "Default shader ID must be non-empty";
    EXPECT_TRUE(registry().hasShader(defaultId))
        << "Default shader ID '" << defaultId.toStdString() << "' does not exist in registry";

    auto* shader = registry().getShader(defaultId);
    ASSERT_NE(shader, nullptr);
    EXPECT_EQ(shader->getId(), defaultId);
}

// Bug caught: shader registered with empty or duplicate ID, causing lookup
// failures or silent overwrites.
TEST_F(ShaderRegistryTest, AllRegisteredShadersHaveUniqueNonEmptyIds)
{
    auto available = registry().getAvailableShaders();
    std::set<std::string> seen;

    for (const auto& info : available)
    {
        EXPECT_FALSE(info.id.isEmpty()) << "Shader has empty ID";
        EXPECT_FALSE(info.displayName.isEmpty()) << "Shader '" << info.id.toStdString() << "' has empty display name";

        auto [_, inserted] = seen.insert(info.id.toStdString());
        EXPECT_TRUE(inserted) << "Duplicate shader ID: '" << info.id.toStdString() << "'";
    }
}

// Bug caught: getShader() returns pointer to prototype, but prototype has
// different ID or properties than what was registered (e.g., moved-from state).
TEST_F(ShaderRegistryTest, GetShaderReturnsConsistentPrototype)
{
    auto available = registry().getAvailableShaders();
    for (const auto& info : available)
    {
        auto* shader = registry().getShader(info.id);
        ASSERT_NE(shader, nullptr) << "getShader returned nullptr for registered ID '" << info.id.toStdString() << "'";
        EXPECT_EQ(shader->getId(), info.id) << "Prototype ID mismatch for '" << info.id.toStdString() << "'";
        EXPECT_EQ(shader->getDisplayName(), info.displayName)
            << "Prototype displayName mismatch for '" << info.id.toStdString() << "'";
    }
}

// Bug caught: createShader() factory returns nullptr or a shader with a
// different ID than requested, causing waveform to render with wrong shader.
TEST_F(ShaderRegistryTest, CreateShaderReturnsNewInstanceForEveryRegisteredShader)
{
    auto available = registry().getAvailableShaders();
    for (const auto& info : available)
    {
        auto instance = registry().createShader(info.id);
        ASSERT_NE(instance, nullptr) << "createShader returned nullptr for '" << info.id.toStdString() << "'";
        EXPECT_EQ(instance->getId(), info.id) << "Created shader has wrong ID for '" << info.id.toStdString() << "'";
    }
}

// Bug caught: createShader() returns the prototype itself instead of a new
// instance, causing shared mutable state between waveforms.
TEST_F(ShaderRegistryTest, CreateShaderReturnsDistinctInstances)
{
    juce::String defaultId = registry().getDefaultShaderId();
    auto instance1 = registry().createShader(defaultId);
    auto instance2 = registry().createShader(defaultId);

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);
    EXPECT_NE(instance1.get(), instance2.get()) << "createShader must return distinct instances, not the same pointer";

    // Neither should be the prototype
    auto* prototype = registry().getShader(defaultId);
    EXPECT_NE(instance1.get(), prototype);
    EXPECT_NE(instance2.get(), prototype);
}

// Bug caught: querying a nonexistent shader ID causes crash instead of
// returning nullptr/empty.
TEST_F(ShaderRegistryTest, NonExistentShaderReturnsNullSafely)
{
    EXPECT_FALSE(registry().hasShader("nonexistent_shader_xyz"));
    EXPECT_EQ(registry().getShader("nonexistent_shader_xyz"), nullptr);
    EXPECT_EQ(registry().createShader("nonexistent_shader_xyz"), nullptr);
}

// Bug caught: empty string passed as shader ID causes undefined behavior
// (e.g., empty key in unordered_map).
TEST_F(ShaderRegistryTest, EmptyStringShaderIdHandledGracefully)
{
    EXPECT_FALSE(registry().hasShader(""));
    EXPECT_EQ(registry().getShader(""), nullptr);
    EXPECT_EQ(registry().createShader(""), nullptr);
}

// Bug caught: ShaderInfo description is empty, causing blank tooltips in UI.
TEST_F(ShaderRegistryTest, AllShadersHaveNonEmptyDescriptions)
{
    auto available = registry().getAvailableShaders();
    for (const auto& info : available)
    {
        EXPECT_FALSE(info.description.isEmpty()) << "Shader '" << info.id.toStdString() << "' has empty description";
    }
}

} // namespace oscil
