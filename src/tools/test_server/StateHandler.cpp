/*
    MultiScoper - State Handler Implementation
*/

#include "tools/test_server/StateHandler.h"

#include "core/InstanceRegistry.h"
#include "core/MultiScoperState.h"
#include "core/Pane.h"

#include "plugin/PluginEditor.h"
#include "plugin/PluginFactory.h"
#include "plugin/PluginProcessor.h"

namespace multiscoper
{

void StateHandler::handleHealth(const httplib::Request& /*req*/, httplib::Response& res)
{
    nlohmann::json response;
    response["status"] = "ok";
    response["port"] = port_;
    sendJson(res, response);
}

void StateHandler::handleStateReset(const httplib::Request& /*req*/, httplib::Response& res)
{
    auto result = runOnMessageThread([this]() -> nlohmann::json {
        nlohmann::json response;

        // Reset MultiScoperState (oscillators, panes, layout)
        auto& state = editor_.getProcessor().getState();

        // Remove all oscillators
        auto oscillators = state.getOscillators();
        for (const auto& osc : oscillators)
        {
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

    sendJson(res, result);
}

void StateHandler::clearTestSources()
{
    auto& registry = PluginFactory::getInstance().getInstanceRegistry();
    for (const auto& pair : testSourceBuffers_)
    {
        SourceId id;
        id.id = juce::String(pair.first);
        registry.unregisterInstance(id);
    }
    testSourceBuffers_.clear();
}

} // namespace multiscoper
