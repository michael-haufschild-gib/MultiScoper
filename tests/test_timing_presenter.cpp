#include <gtest/gtest.h>
#include "ui/presenters/TimingPresenter.h"
#include "dsp/TimingConfig.h"

using namespace oscil;

class TimingPresenterTest : public ::testing::Test
{
protected:
    TimingPresenter presenter;
};

TEST_F(TimingPresenterTest, DefaultInitialization)
{
    EXPECT_EQ(presenter.getTimingMode(), TimingMode::TIME);
    EXPECT_FLOAT_EQ(presenter.getTimeIntervalMs(), 500.0f);
    EXPECT_FALSE(presenter.isHostSyncEnabled());
    EXPECT_EQ(presenter.getWaveformMode(), WaveformMode::FreeRunning);
}

TEST_F(TimingPresenterTest, SetTimingModeUpdatesStateAndNotifies)
{
    bool notified = false;
    bool stateChanged = false;

    presenter.setOnTimingModeChanged([&](TimingMode mode) {
        notified = true;
        EXPECT_EQ(mode, TimingMode::MELODIC);
    });
    presenter.setOnStateChanged([&]() { stateChanged = true; });

    presenter.setTimingMode(TimingMode::MELODIC);

    EXPECT_TRUE(notified);
    EXPECT_TRUE(stateChanged);
    EXPECT_TRUE(presenter.isMelodicMode());
    EXPECT_FALSE(presenter.isTimeMode());
}

TEST_F(TimingPresenterTest, SetTimeIntervalUpdatesStateAndNotifies)
{
    bool notified = false;
    presenter.setOnTimeIntervalChanged([&](float ms) {
        notified = true;
        EXPECT_FLOAT_EQ(ms, 1000.0f);
    });

    presenter.setTimeIntervalMs(1000.0f);

    EXPECT_TRUE(notified);
    EXPECT_FLOAT_EQ(presenter.getTimeIntervalMs(), 1000.0f);
}

TEST_F(TimingPresenterTest, HostSyncToggleUpdatesVisibilityHelpers)
{
    presenter.setTimingMode(TimingMode::MELODIC);
    presenter.setHostSyncEnabled(false);

    EXPECT_TRUE(presenter.shouldShowBPMField());
    EXPECT_FALSE(presenter.shouldShowBPMValue());

    presenter.setHostSyncEnabled(true);

    EXPECT_FALSE(presenter.shouldShowBPMField());
    EXPECT_TRUE(presenter.shouldShowBPMValue());
}

TEST_F(TimingPresenterTest, SyncBadgeVisibility)
{
    presenter.setTimingMode(TimingMode::MELODIC);
    presenter.setHostSyncEnabled(true);
    presenter.setSyncStatus(false);

    EXPECT_FALSE(presenter.shouldShowSyncedBadge());

    presenter.setSyncStatus(true);
    EXPECT_TRUE(presenter.shouldShowSyncedBadge());
    
    presenter.setTimingMode(TimingMode::TIME);
    EXPECT_FALSE(presenter.shouldShowSyncedBadge());
}
