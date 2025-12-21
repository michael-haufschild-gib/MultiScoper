#pragma once
#include "BaseTestRunner.h"

namespace oscil
{

class SettingsTestRunner : public BaseTestRunner
{
public:
    using BaseTestRunner::BaseTestRunner;
    nlohmann::json runTests() override;
};

}
