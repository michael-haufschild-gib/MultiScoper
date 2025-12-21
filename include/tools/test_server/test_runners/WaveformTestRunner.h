#pragma once
#include "BaseTestRunner.h"

namespace oscil
{

class WaveformTestRunner : public BaseTestRunner
{
public:
    using BaseTestRunner::BaseTestRunner;
    nlohmann::json runTests() override;
};

}
