#pragma once

#include "core/dsp/TimingEngine.h"

#include "TestSignals.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>

using namespace oscil;
using oscil::test::generateRamp;
using oscil::test::generateSineWave;
using oscil::test::generateStep;

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
