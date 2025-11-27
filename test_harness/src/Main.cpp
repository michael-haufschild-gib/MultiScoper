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
        centreWithSize(400, 300);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    /**
     * Status display component showing DAW and server state.
     */
    class StatusComponent : public juce::Component, private juce::Timer
    {
    public:
        StatusComponent(TestDAW& daw, TestHttpServer& server)
            : daw_(daw), server_(server)
        {
            setSize(400, 300);
            startTimerHz(4); // Update 4 times per second
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::darkgrey);

            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(16.0f));

            int y = 20;
            const int lineHeight = 24;

            // Title
            g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
            g.drawText("Oscil Test Harness", 20, y, getWidth() - 40, lineHeight, juce::Justification::centred);
            y += lineHeight * 2;

            g.setFont(juce::FontOptions(14.0f));

            // Server status
            auto serverStatus = server_.isRunning()
                ? juce::String("Running on port ") + juce::String(server_.getPort())
                : juce::String("Stopped");
            g.setColour(server_.isRunning() ? juce::Colours::lightgreen : juce::Colours::red);
            g.drawText("HTTP Server: " + serverStatus, 20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight;

            // Transport status
            g.setColour(juce::Colours::white);
            auto& transport = daw_.getTransport();
            auto transportStatus = transport.isPlaying() ? "Playing" : "Stopped";
            g.drawText(juce::String("Transport: ") + transportStatus, 20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight;

            // BPM
            g.drawText(juce::String("BPM: ") + juce::String(transport.getBpm(), 1), 20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight;

            // Position
            g.drawText(juce::String("Position: ") + juce::String(transport.getPositionBeats(), 2) + " beats",
                       20, y, getWidth() - 40, lineHeight, juce::Justification::left);
            y += lineHeight * 2;

            // Track info
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
                    juce::String waveformName;
                    switch (gen.getWaveform())
                    {
                        case Waveform::Sine: waveformName = "Sine"; break;
                        case Waveform::Square: waveformName = "Square"; break;
                        case Waveform::Saw: waveformName = "Saw"; break;
                        case Waveform::Triangle: waveformName = "Triangle"; break;
                        case Waveform::Noise: waveformName = "Noise"; break;
                        case Waveform::Silence: waveformName = "Silence"; break;
                    }

                    juce::String trackInfo = juce::String("Track ") + juce::String(i + 1) + ": " +
                                              waveformName + " @ " +
                                              juce::String(gen.getFrequency(), 1) + " Hz, " +
                                              juce::String(gen.getAmplitude() * 100.0f, 0) + "%";
                    g.drawText(trackInfo, 30, y, getWidth() - 50, lineHeight, juce::Justification::left);
                    y += lineHeight;
                }
            }

            // API endpoint hint
            y = getHeight() - 40;
            g.setColour(juce::Colours::grey);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText("API: http://localhost:" + juce::String(server_.getPort()) + "/health",
                       20, y, getWidth() - 40, lineHeight, juce::Justification::centred);
        }

        void timerCallback() override
        {
            repaint();
        }

    private:
        TestDAW& daw_;
        TestHttpServer& server_;
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
