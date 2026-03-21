/*
    Oscil - Pane Body Tests
*/

#include <gtest/gtest.h>

#include "OscilTestFixtures.h"
#include "ui/layout/pane/PaneBody.h"

using namespace oscil;
using namespace oscil::test;

class PaneBodyTest : public OscilPluginTestFixture
{
protected:
    void SetUp() override
    {
        OscilPluginTestFixture::SetUp();

        paneBody_ = std::make_unique<PaneBody>(*processor, getRegistry(), getThemeManager(), getShaderRegistry());
        paneBody_->setBounds(0, 0, 640, 320);
        paneBody_->resized();
    }

    void TearDown() override
    {
        paneBody_.reset();
        OscilPluginTestFixture::TearDown();
    }

    std::unique_ptr<PaneBody> paneBody_;
};

TEST_F(PaneBodyTest, WaveformLayerPassesMouseHitTestingToPaneBody)
{
    Oscillator oscillator;
    paneBody_->addOscillator(oscillator);
    paneBody_->resized();

    auto* waveformStack = paneBody_->getWaveformStack();
    ASSERT_NE(waveformStack, nullptr);
    ASSERT_NE(waveformStack->getWaveformAt(0), nullptr);

    bool interceptsThis = true;
    bool interceptsChildren = true;
    waveformStack->getInterceptsMouseClicks(interceptsThis, interceptsChildren);

    EXPECT_FALSE(interceptsThis);
    EXPECT_FALSE(interceptsChildren);

    auto* hitComponent = waveformStack->getComponentAt(juce::Point<int>{ 20, 20 });
    EXPECT_EQ(hitComponent, nullptr);
}

TEST_F(PaneBodyTest, NoSourceOscillatorDoesNotAttachCaptureBuffer)
{
    Oscillator oscillator;
    oscillator.setSourceId(SourceId::noSource());

    paneBody_->addOscillator(oscillator);
    paneBody_->resized();

    auto* waveformStack = paneBody_->getWaveformStack();
    ASSERT_NE(waveformStack, nullptr);

    auto* waveform = waveformStack->getWaveformAt(0);
    ASSERT_NE(waveform, nullptr);
    EXPECT_EQ(waveform->getCaptureBuffer(), nullptr);
}
