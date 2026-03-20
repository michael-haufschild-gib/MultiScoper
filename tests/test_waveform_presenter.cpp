#include <gtest/gtest.h>
#include "ui/panels/WaveformPresenter.h"
#include "core/SharedCaptureBuffer.h"
#include <vector>

using namespace oscil;

// Mock SharedCaptureBuffer to control input data
class MockCaptureBuffer : public SharedCaptureBuffer {
public:
    std::vector<float> leftData;
    std::vector<float> rightData;
    mutable int lastRequestedSamplesLeft = 0;
    mutable int lastRequestedSamplesRight = 0;
    CaptureFrameMetadata metadata;

    MockCaptureBuffer() : SharedCaptureBuffer(4096) {} // dummy size

    using SharedCaptureBuffer::read;

    int read(float* dest, int numSamples, int channel) const override {
        if (channel == 0)
            lastRequestedSamplesLeft = numSamples;
        else if (channel == 1)
            lastRequestedSamplesRight = numSamples;

        const auto& source = (channel == 0) ? leftData : rightData;
        int count = std::min(numSamples, static_cast<int>(source.size()));
        for (int i = 0; i < count; ++i) {
            dest[i] = source[i];
        }
        // Fill remainder with silence
        for (int i = count; i < numSamples; ++i) {
            dest[i] = 0.0f;
        }
        return count;
    }

    CaptureFrameMetadata getLatestMetadata() const override {
        return metadata;
    }
};

class WaveformPresenterTest : public ::testing::Test {
protected:
    WaveformPresenter presenter;
    std::shared_ptr<MockCaptureBuffer> buffer;

    void SetUp() override {
        buffer = std::make_shared<MockCaptureBuffer>();
        // Fill with some dummy data
        buffer->leftData.assign(2048, 0.5f);
        buffer->rightData.assign(2048, -0.5f);
        buffer->metadata.timestamp = 0;
        buffer->metadata.numSamples = 512;
        
        presenter.setCaptureBuffer(buffer);
        presenter.setDisplaySamples(1024);
        presenter.setDisplayWidth(100);
    }
};

TEST_F(WaveformPresenterTest, ProcessReadsBuffer) {
    presenter.process();
    
    EXPECT_TRUE(presenter.hasData());
    EXPECT_FALSE(presenter.getDisplayBuffer1().empty());
    
    // Check that data matches input (decimated/processed)
    // Input is constant 0.5, so output should be 0.5
    // Note: SignalProcessor might process in place
    // Decimator might average or pick max/min
    EXPECT_NEAR(presenter.getDisplayBuffer1()[0], 0.5f, 0.01f);
}

TEST_F(WaveformPresenterTest, ProcessingModeMono) {
    presenter.setProcessingMode(ProcessingMode::Mono);
    presenter.process();
    
    EXPECT_FALSE(presenter.isStereo());
    // Mono mix: (L+R)/2 = (0.5 + -0.5)/2 = 0
    EXPECT_NEAR(presenter.getDisplayBuffer1()[0], 0.0f, 0.01f);
}

TEST_F(WaveformPresenterTest, AutoScaleLogic) {
    presenter.setAutoScale(true);
    
    // Input peak is 0.5 (constant 0.5 signal)
    // Target scale should be ~ 0.9 / 0.5 = 1.8
    
    // Process multiple times to allow smoothing to settle
    for(int i=0; i<200; ++i) {
        presenter.process();
    }
    
    float scale = presenter.getEffectiveScale();
    EXPECT_GT(scale, 1.0f);
    EXPECT_NEAR(scale, 1.8f, 0.1f) << "Scale: " << scale << ", Peak: " << presenter.getPeakLevel();
}

TEST_F(WaveformPresenterTest, ManualScaleDoesNotChange) {
    presenter.setAutoScale(false);
    presenter.process();
    
    EXPECT_EQ(presenter.getEffectiveScale(), 1.0f);
}

TEST_F(WaveformPresenterTest, GainProcessing) {
    // +6dB = 2.0x
    presenter.setGainDb(6.0f);
    presenter.setAutoScale(false);
    
    presenter.process();
    
    // Input 0.5 * 2.0 = 1.0
    EXPECT_NEAR(presenter.getDisplayBuffer1()[0], 1.0f, 0.01f);
}

TEST_F(WaveformPresenterTest, RestartRequestWaitsUntilTriggerTimestampIsReached) {
    buffer->metadata.timestamp = 9000;
    buffer->metadata.numSamples = 128;

    presenter.requestRestartAtTimestamp(10000);
    presenter.process();

    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 0);
    EXPECT_EQ(buffer->lastRequestedSamplesRight, 0);
    EXPECT_FALSE(presenter.hasData());
}

TEST_F(WaveformPresenterTest, RestartRequestClampsReadWindowUntilDisplayWindowIsFilled) {
    presenter.requestRestartAtTimestamp(10000);

    buffer->metadata.timestamp = 10000;
    buffer->metadata.numSamples = 128;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 128);
    EXPECT_EQ(buffer->lastRequestedSamplesRight, 128);

    buffer->metadata.timestamp = 10128;
    buffer->metadata.numSamples = 256;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 384);

    buffer->metadata.timestamp = 15000;
    buffer->metadata.numSamples = 1024;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 1024);

    // Restart budget should be cleared once full window is available.
    buffer->metadata.timestamp = 17000;
    buffer->metadata.numSamples = 64;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 1024);
}

TEST_F(WaveformPresenterTest, RestartRequestNewerTriggerOverridesPendingWindow) {
    presenter.requestRestartAtTimestamp(10000);

    buffer->metadata.timestamp = 10000;
    buffer->metadata.numSamples = 128;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 128);

    // Newer trigger arrives before the first restart window fills.
    presenter.requestRestartAtTimestamp(10200);

    // Until we reach the new trigger timestamp, presenter should wait.
    buffer->metadata.timestamp = 10100;
    buffer->metadata.numSamples = 64;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 128);
    EXPECT_FALSE(presenter.hasData());

    // Once we pass the newer trigger point, the window is recalculated from it.
    buffer->metadata.timestamp = 10200;
    buffer->metadata.numSamples = 96;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 96);
}

TEST_F(WaveformPresenterTest, RestartBurstEventuallyReturnsToFullWindowWhenTriggersStop) {
    presenter.requestRestartAtTimestamp(10000);

    // Simulate rapid trigger bursts by repeatedly replacing restart timestamp
    // with a newer boundary while timeline advances.
    buffer->metadata.timestamp = 10020;
    buffer->metadata.numSamples = 80;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 100);

    presenter.requestRestartAtTimestamp(10100);
    buffer->metadata.timestamp = 10110;
    buffer->metadata.numSamples = 90;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 100);

    presenter.requestRestartAtTimestamp(10200);
    buffer->metadata.timestamp = 10230;
    buffer->metadata.numSamples = 70;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 100);

    // After bursts stop and enough samples accrue, presenter must converge
    // back to full display window reads.
    buffer->metadata.timestamp = 11250;
    buffer->metadata.numSamples = 1024;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 1024);

    buffer->metadata.timestamp = 13000;
    buffer->metadata.numSamples = 64;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 1024);
}

TEST_F(WaveformPresenterTest, RestartRequestWithZeroTimestampAnchorsToCurrentFrameEnd) {
    buffer->metadata.timestamp = 5000;
    buffer->metadata.numSamples = 128;

    presenter.requestRestartAtTimestamp(0);
    presenter.process();

    // First process anchors to current frame end and waits for post-anchor data.
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 0);
    EXPECT_FALSE(presenter.hasData());

    buffer->metadata.timestamp = 5128;
    buffer->metadata.numSamples = 64;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 64);
}

TEST_F(WaveformPresenterTest, RestartRequestWithNegativeTimestampAnchorsToCurrentFrameEnd) {
    buffer->metadata.timestamp = 7000;
    buffer->metadata.numSamples = 64;

    presenter.requestRestartAtTimestamp(-2);
    presenter.process();

    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 0);
    EXPECT_FALSE(presenter.hasData());

    buffer->metadata.timestamp = 7064;
    buffer->metadata.numSamples = 96;
    presenter.process();
    EXPECT_EQ(buffer->lastRequestedSamplesLeft, 96);
}

TEST_F(WaveformPresenterTest, RestartRequestKeepsFixedDisplayWindowWithTrailingSilence) {
    presenter.setDisplaySamples(8);
    presenter.setDisplayWidth(2); // targetSamples = 4
    presenter.setAutoScale(false);

    buffer->leftData = { 1.0f, 0.0f };
    buffer->rightData = { 1.0f, 0.0f };
    buffer->metadata.timestamp = 1000;
    buffer->metadata.numSamples = 2;

    presenter.requestRestartAtTimestamp(1000);
    presenter.process();

    const auto& display = presenter.getDisplayBuffer1();
    ASSERT_EQ(display.size(), 4u);
    EXPECT_NEAR(display[0], 1.0f, 0.01f);
    EXPECT_NEAR(display[1], 0.0f, 0.01f);
    EXPECT_NEAR(display[2], 0.0f, 0.01f);
    EXPECT_NEAR(display[3], 0.0f, 0.01f);
}

TEST_F(WaveformPresenterTest, NoSamplesReadClearsPreviouslyRenderedData) {
    presenter.process();
    ASSERT_TRUE(presenter.hasData());

    buffer->leftData.clear();
    buffer->rightData.clear();
    presenter.process();

    EXPECT_FALSE(presenter.hasData());
    EXPECT_FLOAT_EQ(presenter.getPeakLevel(), 0.0f);
    EXPECT_FLOAT_EQ(presenter.getRMSLevel(), 0.0f);
}
