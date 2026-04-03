/*
    Oscil - Screenshot Handler Implementation
*/

#include "tools/test_server/ScreenshotHandler.h"

#include "plugin/PluginEditor.h"

#include <juce_graphics/juce_graphics.h>

namespace oscil
{

void ScreenshotHandler::handleTakeScreenshot(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string const filename = body.value("filename", "test_screenshot.png");

        auto result = runOnMessageThread([this, filename]() -> nlohmann::json {
            nlohmann::json response;

            // Create a snapshot of the editor
            auto bounds = editor_.getLocalBounds();
            juce::Image const image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
            juce::Graphics g(image);
            editor_.paintEntireComponent(g, true);

            // Save to file
            juce::File const outputFile(
                juce::File::getCurrentWorkingDirectory().getChildFile("screenshots").getChildFile(filename));
            outputFile.getParentDirectory().createDirectory();

            juce::FileOutputStream stream(outputFile);
            if (stream.openedOk())
            {
                juce::PNGImageFormat pngFormat;
                pngFormat.writeImageToStream(image, stream);
                response["status"] = "ok";
                response["path"] = outputFile.getFullPathName().toStdString();
            }
            else
            {
                response["error"] = "Failed to open file for writing";
            }

            return response;
        });

        res.set_content(result.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["error"] = e.what();
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

} // namespace oscil
