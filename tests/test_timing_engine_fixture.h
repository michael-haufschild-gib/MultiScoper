#pragma once

#include "core/dsp/TimingEngine.h"

#include "TestSignals.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>

using namespace multiscoper;
using multiscoper::test::generateRamp;
using multiscoper::test::generateSineWave;
using multiscoper::test::generateStep;

class TimingEngineTest : public ::testing::Test
{
protected:
    TimingEngine engine;

    void SetUp() override
    {
        EngineTimingConfig defaultConfig;
        engine.setConfig(defaultConfig);
    }
};
