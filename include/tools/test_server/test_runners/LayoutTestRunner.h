#pragma once
#include "BaseTestRunner.h"

namespace oscil
{

class LayoutTestRunner : public BaseTestRunner
{
public:
    using BaseTestRunner::BaseTestRunner;
    nlohmann::json runTests() override;
    nlohmann::json runDragDropTests();
};

}
