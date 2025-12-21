
#include <gtest/gtest.h>
#include "ui/components/OscilSlider.h"
#include "ui/theme/ThemeManager.h"

using namespace oscil;

class OscilSliderBugTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        themeManager_ = std::make_unique<ThemeManager>();
    }

    ThemeManager& getThemeManager() { return *themeManager_; }

private:
    std::unique_ptr<ThemeManager> themeManager_;
};

TEST_F(OscilSliderBugTest, StepLogicExceedsMax)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 10.0);
    slider.setStep(4.0);

    // Set value to Max.
    // Logic: 
    // 1. Clamp 10 -> 10.
    // 2. steps = round((10 - 0) / 4) = round(2.5) = 3.
    // 3. value = 0 + 3 * 4 = 12.
    // 4. 12 > 10!
    slider.setValue(10.0, false);

    EXPECT_LE(slider.getValue(), 10.0);
}

TEST_F(OscilSliderBugTest, StepLogicExceedsMax2)
{
    OscilSlider slider(getThemeManager());
    slider.setRange(0.0, 10.0);
    slider.setStep(6.0);

    slider.setValue(10.0, false);
    
    // steps = round(10/6) = round(1.66) = 2.
    // value = 12.
    EXPECT_LE(slider.getValue(), 10.0);
}
