#include <gtest/gtest.h>
#include "ui/presenters/WaveformPresenter.h"
#include "core/SharedCaptureBuffer.h"
#include <vector>

using namespace oscil;

// Mock SharedCaptureBuffer to control input data
class MockCaptureBuffer : public SharedCaptureBuffer {
public:
    std::vector<float> leftData;
    std::vector<float> rightData;

    MockCaptureBuffer() : SharedCaptureBuffer(4096) {} // dummy size

    using SharedCaptureBuffer::read;

    int read(float* dest, int numSamples, int channel) const override {
        const auto& source = (channel == 0) ? leftData : rightData;
        int count = std::min(numSamples, static_cast<int>(source.size()));
        for (int i = 0; i < count; ++i) {
            dest[i] = source[i];
        }
        // Fill remainder with silence
        for (int i = count; i < numSamples; ++i) {
            dest[i] = 0.0f;
        }
        return numSamples;
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