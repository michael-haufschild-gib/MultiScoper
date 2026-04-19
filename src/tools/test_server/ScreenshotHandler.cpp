/*
    MultiScoper - Screenshot Handler Implementation
*/

#include "tools/test_server/ScreenshotHandler.h"

#include "plugin/PluginEditor.h"

#include <juce_graphics/juce_graphics.h>

namespace multiscoper
{

void ScreenshotHandler::handleTakeScreenshot(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
        std::string const requestedFilename = body.value("filename", "test_screenshot.png");

        // Sanitize filename to prevent path traversal (strip directory components)
        juce::String const safeFilename = juce::File::createLegalFileName(juce::String(requestedFilename)).trim();
        if (safeFilename.isEmpty())
            throw std::runtime_error("Invalid filename");

        auto result = runOnMessageThread([this, safeFilename]() -> nlohmann::json {
            nlohmann::json response;

            // Create a snapshot of the editor
            auto bounds = editor_.getLocalBounds();
            juce::Image const image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
            juce::Graphics g(image);
            editor_.paintEntireComponent(g, true);

            // Save to file
            juce::File const screenshotsDir = juce::File::getCurrentWorkingDirectory().getChildFile("screenshots");
            juce::File const outputFile(screenshotsDir.getChildFile(safeFilename));
            outputFile.getParentDirectory().createDirectory();

            juce::FileOutputStream stream(outputFile);
            if (stream.openedOk())
            {
                juce::PNGImageFormat pngFormat;
                if (pngFormat.writeImageToStream(image, stream))
                {
                    response["status"] = "ok";
                    response["path"] = outputFile.getFullPathName().toStdString();
                }
                else
                {
                    response["error"] = "Failed to encode PNG image";
                }
            }
            else
            {
                response["error"] = "Failed to open file for writing";
            }

            return response;
        });

        sendJson(res, result);
    }
    catch (const std::exception& e)
    {
        sendJson(res, jsonError(e.what()), 400);
    }
}

} // namespace multiscoper
