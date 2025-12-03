/*
    Oscil - State Handler Implementation
*/

#include "tools/test_server/StateHandler.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "core/OscilState.h"
#include "ui/layout/Pane.h"
#include "core/InstanceRegistry.h"

namespace oscil
{

void StateHandler::handleHealth(const httplib::Request& /*req*/, httplib::Response& res)
{
    nlohmann::json response;
    response["status"] = "ok";
    response["port"] = port_;
    res.set_content(response.dump(), "application/json");
}

void StateHandler::handleStateReset(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;

        // Reset OscilState (oscillators, panes, layout)
        auto& state = editor_.getProcessor().getState();

        // Remove all oscillators
        auto oscillators = state.getOscillators();
        for (const auto& osc : oscillators) {
            state.removeOscillator(osc.getId());
        }

        // Reset layout
        auto& layoutManager = state.getLayoutManager();
        layoutManager.clear(); // Remove all panes

        // Create default pane
        Pane defaultPane;
        defaultPane.setName("Pane 1");
        defaultPane.setOrderIndex(0);
        layoutManager.addPane(defaultPane);

        state.setColumnLayout(ColumnLayout::Single);

        // Clear test sources
        clearTestSources();

        // Trigger UI refresh
        editor_.refreshPanels();
        editor_.resized();

        response["status"] = "ok";
        return response;
    });

    res.set_content(result.dump(), "application/json");
}

void StateHandler::clearTestSources()
{
    auto& registry = InstanceRegistry::getInstance();
    for (const auto& pair : testSourceBuffers_)
    {
        SourceId id;
        id.id = juce::String(pair.first);
        registry.unregisterInstance(id);
    }
    testSourceBuffers_.clear();
}

} // namespace oscil
