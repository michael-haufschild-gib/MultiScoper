/*
    Oscil - Source ID Tests
    Tests for SourceId and InstanceId generation, validation, equality, hashing
*/

#include "core/Source.h"

#include "helpers/Fixtures.h"

#include <gtest/gtest.h>

using namespace oscil;
using namespace oscil::test;

// === SourceId Tests ===

TEST(SourceIdTest, SourceIdGeneration)
{
    auto id1 = SourceId::generate();
    auto id2 = SourceId::generate();

    EXPECT_TRUE(id1.isValid());
    EXPECT_TRUE(id2.isValid());
    EXPECT_NE(id1, id2);
}

TEST(SourceIdTest, SourceIdInvalid)
{
    auto invalid = SourceId::invalid();
    EXPECT_FALSE(invalid.isValid());
    EXPECT_TRUE(invalid.id.isEmpty());
}

TEST(SourceIdTest, SourceIdNoSource)
{
    auto noSource = SourceId::noSource();
    EXPECT_FALSE(noSource.isValid());
    EXPECT_TRUE(noSource.isNoSource());
    EXPECT_EQ(noSource.id, "NO_SOURCE");
}

TEST(SourceIdTest, SourceIdEquality)
{
    SourceId id1{"test-id-123"};
    SourceId id2{"test-id-123"};
    SourceId id3{"test-id-456"};

    EXPECT_EQ(id1, id2);
    EXPECT_NE(id1, id3);
}

TEST(SourceIdTest, SourceIdLessThanOperator)
{
    SourceId a{"aaa"};
    SourceId b{"bbb"};
    SourceId c{"aaa"};

    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(a < c); // Equal
    EXPECT_FALSE(c < a); // Equal
}

TEST(SourceIdTest, SourceIdHashDifferent)
{
    SourceIdHash hasher;

    SourceId id1{"test-1"};
    SourceId id2{"test-2"};

    // Different IDs should have different hashes (with high probability)
    EXPECT_NE(hasher(id1), hasher(id2));
}

TEST(SourceIdTest, SourceIdHashSame)
{
    SourceIdHash hasher;

    SourceId id1{"test-id"};
    SourceId id2{"test-id"};

    // Same IDs should have same hash
    EXPECT_EQ(hasher(id1), hasher(id2));
}

// === InstanceId Tests ===

TEST(InstanceIdTest, InstanceIdGeneration)
{
    auto id1 = InstanceId::generate();
    auto id2 = InstanceId::generate();

    EXPECT_TRUE(id1.isValid());
    EXPECT_TRUE(id2.isValid());
    EXPECT_NE(id1, id2);
}

TEST(InstanceIdTest, InstanceIdInvalid)
{
    auto invalid = InstanceId::invalid();
    EXPECT_FALSE(invalid.isValid());
}

TEST(InstanceIdTest, InstanceIdHashDifferent)
{
    InstanceIdHash hasher;

    InstanceId id1{"instance-1"};
    InstanceId id2{"instance-2"};

    EXPECT_NE(hasher(id1), hasher(id2));
}

TEST(InstanceIdTest, InstanceIdHashSame)
{
    InstanceIdHash hasher;

    InstanceId id1{"instance-id"};
    InstanceId id2{"instance-id"};

    EXPECT_EQ(hasher(id1), hasher(id2));
}
