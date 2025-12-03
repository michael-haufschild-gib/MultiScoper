#include <gtest/gtest.h>
#include "ui/panels/OscillatorPresenter.h"
#include "core/Oscillator.h"

using namespace oscil;

class OscillatorPresenterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Oscillator osc;
        osc.setName("Test Osc");
        osc.setVisible(true);
        osc.setProcessingMode(ProcessingMode::FullStereo);
        presenter = std::make_unique<OscillatorPresenter>(osc);
    }

    std::unique_ptr<OscillatorPresenter> presenter;
};

TEST_F(OscillatorPresenterTest, Initialization)
{
    EXPECT_EQ(presenter->getName(), "Test Osc");
    EXPECT_TRUE(presenter->isVisible());
    EXPECT_EQ(presenter->getProcessingMode(), ProcessingMode::FullStereo);
}

TEST_F(OscillatorPresenterTest, SetNameUpdatesOscillatorAndNotifies)
{
    bool notified = false;
    presenter->setOnOscillatorChanged([&](const Oscillator& osc) {
        notified = true;
        EXPECT_EQ(osc.getName(), "New Name");
    });

    presenter->setName("New Name");
    EXPECT_TRUE(notified);
    EXPECT_EQ(presenter->getName(), "New Name");
}

TEST_F(OscillatorPresenterTest, SetVisibleUpdatesOscillatorAndNotifies)
{
    bool visibilityToggled = false;
    bool changed = false;

    presenter->setOnVisibilityToggled([&](const OscillatorId&, bool visible) {
        visibilityToggled = true;
        EXPECT_FALSE(visible);
    });

    presenter->setOnOscillatorChanged([&](const Oscillator& osc) {
        changed = true;
        EXPECT_FALSE(osc.isVisible());
    });

    presenter->setVisible(false);

    EXPECT_TRUE(visibilityToggled);
    EXPECT_TRUE(changed);
    EXPECT_FALSE(presenter->isVisible());
}

TEST_F(OscillatorPresenterTest, ToggleVisibility)
{
    bool visibilityToggled = false;
    presenter->setOnVisibilityToggled([&](const OscillatorId&, bool visible) {
        visibilityToggled = true;
        EXPECT_FALSE(visible);
    });

    presenter->toggleVisibility(); // Was true, now false
    EXPECT_TRUE(visibilityToggled);
    EXPECT_FALSE(presenter->isVisible());
}

TEST_F(OscillatorPresenterTest, RequestDeleteNotifies)
{
    bool deleteRequested = false;
    presenter->setOnDeleteRequested([&](const OscillatorId&) {
        deleteRequested = true;
    });

    presenter->requestDelete();
    EXPECT_TRUE(deleteRequested);
}

TEST_F(OscillatorPresenterTest, SetProcessingModeUpdates)
{
    bool notified = false;
    presenter->setOnOscillatorChanged([&](const Oscillator& osc) {
        notified = true;
        EXPECT_EQ(osc.getProcessingMode(), ProcessingMode::Mono);
    });

    presenter->setProcessingMode(ProcessingMode::Mono);
    EXPECT_TRUE(notified);
    EXPECT_EQ(presenter->getProcessingMode(), ProcessingMode::Mono);
}
