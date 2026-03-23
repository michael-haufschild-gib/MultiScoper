/*
    Oscil Test Harness - HTTP Server: Screenshot & Verification Handlers
*/

#include "TestHttpServer.h"

namespace oscil::test
{

void TestHttpServer::setupVerificationRoutes()
{
    server_->Post("/screenshot",
                  [this](const httplib::Request& req, httplib::Response& res) { handleScreenshot(req, res); });
    server_->Post("/screenshot/compare",
                  [this](const httplib::Request& req, httplib::Response& res) { handleScreenshotCompare(req, res); });
    server_->Post("/baseline/save",
                  [this](const httplib::Request& req, httplib::Response& res) { handleBaselineSave(req, res); });
    server_->Post("/verify/waveform",
                  [this](const httplib::Request& req, httplib::Response& res) { handleVerifyWaveform(req, res); });
    server_->Post("/verify/color",
                  [this](const httplib::Request& req, httplib::Response& res) { handleVerifyColor(req, res); });
    server_->Post("/verify/bounds",
                  [this](const httplib::Request& req, httplib::Response& res) { handleVerifyBounds(req, res); });
    server_->Post("/verify/visible",
                  [this](const httplib::Request& req, httplib::Response& res) { handleVerifyVisible(req, res); });
    server_->Get("/analyze/waveform",
                 [this](const httplib::Request& req, httplib::Response& res) { handleAnalyzeWaveform(req, res); });
}

void TestHttpServer::handleScreenshot(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string path = body.value("path", "/tmp/screenshot.png");
        std::string element = body.value("element", "window");

        juce::File outputFile(path);
        bool success = false;

        if (element == "window")
        {
            if (auto* editor = daw_.getPrimaryEditor())
                success = screenshot_.captureWindow(editor, outputFile);
        }
        else
        {
            success = screenshot_.captureElement(element, outputFile);
        }

        if (success)
        {
            json data;
            data["path"] = outputFile.getFullPathName().toStdString();
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Failed to capture screenshot").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleScreenshotCompare(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string baselinePath = body.value("baseline", "");
        int tolerance = body.value("tolerance", 5);

        juce::File baselineFile(baselinePath);
        auto result = screenshot_.compareElementToBaseline(elementId, baselineFile, tolerance);

        json data;
        data["match"] = result.match;
        data["similarity"] = result.similarity;
        data["differentPixels"] = result.differentPixels;
        data["totalPixels"] = result.totalPixels;

        if (!result.diffBounds.isEmpty())
        {
            data["diffBounds"] = {{"x", result.diffBounds.getX()},
                                  {"y", result.diffBounds.getY()},
                                  {"width", result.diffBounds.getWidth()},
                                  {"height", result.diffBounds.getHeight()}};
        }

        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleBaselineSave(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string dirPath = body.value("directory", "/tmp/baselines");
        std::string name = body.value("name", "");

        juce::File baselineDir(dirPath);
        bool success = screenshot_.saveBaseline(elementId, baselineDir, name);

        if (success)
        {
            json data;
            data["elementId"] = elementId;
            data["directory"] = dirPath;
            res.set_content(successResponse(data).dump(), "application/json");
        }
        else
        {
            res.set_content(errorResponse("Failed to save baseline").dump(), "application/json");
        }
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyWaveform(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        float minAmplitude = body.value("minAmplitude", 0.05f);

        json data;
        data["pass"] = screenshot_.verifyWaveformRendered(elementId, minAmplitude);
        data["elementId"] = elementId;
        data["minAmplitude"] = minAmplitude;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyColor(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        std::string colorHex = body.value("color", "#000000");
        int tolerance = body.value("tolerance", 10);
        std::string mode = body.value("mode", "background");
        float minCoverage = body.value("minCoverage", 0.01f);

        juce::Colour expectedColor = juce::Colour::fromString(colorHex);
        bool pass = false;

        if (mode == "background")
            pass = screenshot_.verifyBackgroundColor(elementId, expectedColor, tolerance);
        else if (mode == "contains")
            pass = screenshot_.verifyContainsColor(elementId, expectedColor, minCoverage, tolerance);

        json data;
        data["pass"] = pass;
        data["elementId"] = elementId;
        data["expectedColor"] = colorHex;
        data["mode"] = mode;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyBounds(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");
        int expectedWidth = body.value("width", 0);
        int expectedHeight = body.value("height", 0);
        int tolerance = body.value("tolerance", 5);

        auto bounds = screenshot_.getElementBounds(elementId);
        bool pass = screenshot_.verifyElementBounds(elementId, expectedWidth, expectedHeight, tolerance);

        json data;
        data["pass"] = pass;
        data["elementId"] = elementId;
        data["actualBounds"] = {
            {"x", bounds.getX()}, {"y", bounds.getY()}, {"width", bounds.getWidth()}, {"height", bounds.getHeight()}};
        data["expectedWidth"] = expectedWidth;
        data["expectedHeight"] = expectedHeight;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleVerifyVisible(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        std::string elementId = body.value("elementId", "");

        json data;
        data["pass"] = screenshot_.verifyElementVisible(elementId);
        data["elementId"] = elementId;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleAnalyzeWaveform(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        std::string elementId = req.get_param_value("elementId");
        std::string bgColorHex = req.get_param_value("backgroundColor");

        juce::Colour backgroundColor = bgColorHex.empty() ? juce::Colours::black : juce::Colour::fromString(bgColorHex);

        auto image = screenshot_.getElementImage(elementId);
        if (image.isNull())
        {
            res.set_content(errorResponse("Element not found or no image").dump(), "application/json");
            return;
        }

        auto analysis = screenshot_.analyzeWaveform(image, backgroundColor);

        json data;
        data["detected"] = analysis.detected;
        data["amplitude"] = analysis.amplitude;
        data["activity"] = analysis.activity;
        data["zeroCrossings"] = analysis.zeroCrossings;
        data["dominantColor"] = analysis.dominantColor.toString().toStdString();
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

} // namespace oscil::test
