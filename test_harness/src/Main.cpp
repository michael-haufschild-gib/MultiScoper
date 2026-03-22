/*
    Oscil Test Harness - Main Entry Point
    Standalone application for E2E testing
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "TestDAW.h"
#include "TestHttpServer.h"

namespace oscil::test
{

/**
 * Main window for the test harness application.
 * Displays status and controls for the test DAW.
 */
class TestHarnessWindow : public juce::DocumentWindow
{
public:
    TestHarnessWindow(TestDAW& daw, TestHttpServer& server)
        : DocumentWindow("Oscil Test Harness",
                         juce::Colours::darkgrey,
                         DocumentWindow::allButtons),
          daw_(daw),
          server_(server)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new StatusComponent(daw, server), true);
        setResizable(true, true);
        centreWithSize(520, 380);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    /**
     * Status display component showing DAW and server state with interactive controls.
     */
    class StatusComponent : public juce::Component,
                            private juce::Timer,
                            private juce::Slider::Listener,
                            private juce::Button::Listener
    {
    public:
        StatusComponent(TestDAW& daw, TestHttpServer& server)
            : daw_(daw), server_(server)
        {
            // Play/Stop button
            playStopButton_.setButtonText("Stop");
            playStopButton_.addListener(this);
            addAndMakeVisible(playStopButton_);

            // BPM slider
            bpmSlider_.setRange(20.0, 300.0, 1.0);
            bpmSlider_.setValue(daw_.getTransport().getBpm(), juce::dontSendNotification);
            bpmSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
            bpmSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
            bpmSlider_.addListener(this);
            addAndMakeVisible(bpmSlider_);

            // BPM label
            bpmLabel_.setText("BPM:", juce::dontSendNotification);
            bpmLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
            addAndMakeVisible(bpmLabel_);

            // Waveform selector and editor button for each track
            for (int i = 0; i < 3; ++i)
            {
                auto& combo = waveformCombos_[i];
                combo.addItem("Sine", 1);
                combo.addItem("Square", 2);
                combo.addItem("Saw", 3);
                combo.addItem("Triangle", 4);
                combo.addItem("Noise", 5);
                combo.addItem("Silence", 6);
                combo.setSelectedId(1, juce::dontSendNotification);
                combo.onChange = [this, i]() { onWaveformChanged(i); };
                addAndMakeVisible(combo);

                auto& label = trackLabels_[i];
                label.setText("Track " + juce::String(i + 1) + ":", juce::dontSendNotification);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                addAndMakeVisible(label);

                // Editor open/close button for each track
                auto& editorBtn = editorButtons_[i];
                editorBtn.setButtonText("Open Editor");
                editorBtn.addListener(this);
                addAndMakeVisible(editorBtn);
            }

            setSize(520, 380);
            startTimerHz(4); // Update 4 times per second
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(20);
            const int rowHeight = 28;
            const int spacing = 6;

            // Skip title area
            bounds.removeFromTop(50);

            // Server status row (painted, no component)
            bounds.removeFromTop(rowHeight);

            // Transport controls row
            auto transportRow = bounds.removeFromTop(rowHeight + spacing);
            playStopButton_.setBounds(transportRow.removeFromLeft(100));
            transportRow.removeFromLeft(20);
            bpmLabel_.setBounds(transportRow.removeFromLeft(40));
            bpmSlider_.setBounds(transportRow.reduced(0, 2));

            bounds.removeFromTop(spacing);

            // Position row (painted)
            bounds.removeFromTop(rowHeight);

            // Tracks section
            bounds.removeFromTop(rowHeight); // "Tracks:" label

            for (int i = 0; i < 3; ++i)
            {
                auto trackRow = bounds.removeFromTop(rowHeight);
                trackLabels_[i].setBounds(trackRow.removeFromLeft(70));
                waveformCombos_[i].setBounds(trackRow.removeFromLeft(100));
                trackRow.removeFromLeft(10); // spacing
                // Frequency info is painted, skip space for it
                trackRow.removeFromLeft(100);
                trackRow.removeFromLeft(10); // spacing
                editorButtons_[i].setBounds(trackRow.removeFromLeft(90));
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::darkgrey);

            g.setColour(juce::Colours::white);
            const int lineHeight = 28;
            int y = 20;

            // Title
            g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
            g.drawText("Oscil Test Harness", 20, y, getWidth() - 40, lineHeight, juce::Justification::centred);
            y += lineHeight + 22;

            g.setFont(juce::FontOptions(14.0f));

            // Server status
            auto serverStatus = server_.isRunning()
                ? juce::String("Running on port ") + juce::String(server_.getPort())
                : juce::String("Stopped");
            g.setColour(server_.isRunning() ? juce::Colours::lightgreen : juce::Colours::red);
            g.drawText("HTTP Server: " + serverStatus, 20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight;

            // Transport controls are components, skip their area
            y += lineHeight + 6;

            // Position
            g.setColour(juce::Colours::white);
            auto& transport = daw_.getTransport();
            g.drawText(juce::String("Position: ") + juce::String(transport.getPositionBeats(), 2) + " beats  |  " +
                       juce::String(transport.getPositionSeconds(), 2) + " sec",
                       20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight + 6;

            y = paintTrackInfo(g, y, lineHeight);

            // API endpoint hint
            g.setColour(juce::Colours::grey);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText("API: http://localhost:" + juce::String(server_.getPort()) + "/health",
                       20, getHeight() - 30, getWidth() - 40, 20, juce::Justification::centred);
        }

        // Paints track header and per-track frequency info. Returns updated y position.
        int paintTrackInfo(juce::Graphics& g, int y, int lineHeight)
        {
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
            g.drawText("Tracks:", 20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight;

            g.setFont(juce::FontOptions(12.0f));
            for (int i = 0; i < 3; ++i)
            {
                auto* track = daw_.getTrack(i);
                if (track != nullptr)
                {
                    auto& gen = track->getAudioGenerator();
                    juce::String freqInfo = juce::String(gen.getFrequency(), 1) + " Hz, " +
                                            juce::String(gen.getAmplitude() * 100.0f, 0) + "%";
                    g.drawText(freqInfo, 210, y, 150, lineHeight, juce::Justification::left);
                }
                y += lineHeight;
            }
            return y;
        }

        void timerCallback() override
        {
            // Update button text based on transport state
            bool isPlaying = daw_.getTransport().isPlaying();
            if (isPlaying != wasPlaying_)
            {
                playStopButton_.setButtonText(isPlaying ? "Stop" : "Play");
                wasPlaying_ = isPlaying;
            }

            // Sync BPM slider if changed externally (via API)
            double currentBpm = daw_.getTransport().getBpm();
            if (std::abs(currentBpm - bpmSlider_.getValue()) > 0.5)
            {
                bpmSlider_.setValue(currentBpm, juce::dontSendNotification);
            }

            // Sync waveform combos and editor buttons
            for (int i = 0; i < 3; ++i)
            {
                auto* track = daw_.getTrack(i);
                if (track != nullptr)
                {
                    int waveformId = static_cast<int>(track->getAudioGenerator().getWaveform()) + 1;
                    if (waveformCombos_[i].getSelectedId() != waveformId)
                    {
                        waveformCombos_[i].setSelectedId(waveformId, juce::dontSendNotification);
                    }

                    // Sync editor button text
                    bool editorVisible = track->isEditorVisible();
                    juce::String expectedText = editorVisible ? "Close Editor" : "Open Editor";
                    if (editorButtons_[i].getButtonText() != expectedText)
                    {
                        editorButtons_[i].setButtonText(expectedText);
                    }
                }
            }

            repaint();
        }

        void buttonClicked(juce::Button* button) override
        {
            if (button == &playStopButton_)
            {
                if (daw_.isRunning())
                    daw_.stop();
                else
                    daw_.start();
                return;
            }

            // Check if it's an editor button
            for (int i = 0; i < 3; ++i)
            {
                if (button == &editorButtons_[i])
                {
                    auto* track = daw_.getTrack(i);
                    if (track != nullptr)
                    {
                        if (track->isEditorVisible())
                            track->hideEditor();
                        else
                            track->showEditor();
                    }
                    return;
                }
            }
        }

        void sliderValueChanged(juce::Slider* slider) override
        {
            if (slider == &bpmSlider_)
            {
                daw_.getTransport().setBpm(slider->getValue());
            }
        }

    private:
        void onWaveformChanged(int trackIndex)
        {
            auto* track = daw_.getTrack(trackIndex);
            if (track != nullptr)
            {
                int selectedId = waveformCombos_[trackIndex].getSelectedId();
                Waveform wf = static_cast<Waveform>(selectedId - 1);
                track->getAudioGenerator().setWaveform(wf);
            }
        }

        TestDAW& daw_;
        TestHttpServer& server_;

        juce::TextButton playStopButton_;
        juce::Slider bpmSlider_;
        juce::Label bpmLabel_;
        juce::ComboBox waveformCombos_[3];
        juce::Label trackLabels_[3];
        juce::TextButton editorButtons_[3];

        bool wasPlaying_ = true;
    };

    TestDAW& daw_;
    TestHttpServer& server_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestHarnessWindow)
};

/**
 * Main application class for the test harness.
 */
class TestHarnessApplication : public juce::JUCEApplication
{
public:
    TestHarnessApplication() = default;

    const juce::String getApplicationName() override { return "Oscil Test Harness"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);

        // Parse command line for port
        int port = TestHttpServer::DEFAULT_PORT;
        auto args = juce::JUCEApplication::getCommandLineParameterArray();
        for (int i = 0; i < args.size() - 1; ++i)
        {
            if (args[i] == "--port" || args[i] == "-p")
            {
                port = args[i + 1].getIntValue();
                if (port <= 0 || port > 65535)
                    port = TestHttpServer::DEFAULT_PORT;
            }
        }

        // Initialize the test DAW
        daw_ = std::make_unique<TestDAW>();
        daw_->initialize();
        daw_->start();  // Start audio processing immediately so waveforms render

        // Create and start HTTP server
        server_ = std::make_unique<TestHttpServer>(*daw_);
        if (!server_->start(port))
        {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Server Error",
                "Failed to start HTTP server on port " + juce::String(port));
        }

        // Create the main window
        mainWindow_ = std::make_unique<TestHarnessWindow>(*daw_, *server_);

        std::cout << "Oscil Test Harness started" << std::endl;
        std::cout << "HTTP API available at: http://localhost:" << port << std::endl;
        std::cout << "Health check: http://localhost:" << port << "/health" << std::endl;
    }

    void shutdown() override
    {
        std::cout << "Shutting down Oscil Test Harness..." << std::endl;

        mainWindow_.reset();

        if (server_)
            server_->stop();
        server_.reset();

        if (daw_)
        {
            daw_->stop();
            daw_->hideAllEditors();
        }
        daw_.reset();

        std::cout << "Shutdown complete" << std::endl;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

private:
    std::unique_ptr<TestDAW> daw_;
    std::unique_ptr<TestHttpServer> server_;
    std::unique_ptr<TestHarnessWindow> mainWindow_;
};

} // namespace oscil::test

// Application entry point
START_JUCE_APPLICATION(oscil::test::TestHarnessApplication)
