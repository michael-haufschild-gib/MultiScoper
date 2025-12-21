#pragma once
#include "BaseTestRunner.h"

namespace oscil
{

class RenderingTestRunner : public BaseTestRunner
{
public:
    using BaseTestRunner::BaseTestRunner;
    nlohmann::json runTests() override;
};

}
