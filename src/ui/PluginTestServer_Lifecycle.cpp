/*
    Oscil - Plugin Test Server Lifecycle Implementation
*/

#include "ui/PluginTestServer.h"
#include "ui/PluginEditor.h"
#include "core/PluginProcessor.h"
#include "core/OscilState.h"
#include "core/InstanceRegistry.h"
#include "core/Pane.h"

namespace oscil
{

void PluginTestServer::clearTestSources()
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

void PluginTestServer::handleStateReset(const httplib::Request& /*req*/, httplib::Response& res)
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

} // namespace oscil
