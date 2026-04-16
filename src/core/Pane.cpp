/*
    Oscil - Pane Implementation
    Pane value type: identity, serialization, and basic property accessors.
    PaneLayoutManager (add/remove/move/bounds/listeners) lives in
    PaneLayoutManager.cpp.
*/

#include "core/Pane.h"

#include "core/OscilState.h"

#include <cmath>

namespace oscil
{

Pane::Pane() : id_(PaneId::generate()) {}

Pane::Pane(const juce::ValueTree& state) : id_(PaneId::generate()) { fromValueTree(state); }

juce::ValueTree Pane::toValueTree() const
{
    juce::ValueTree state(PaneIds::Pane);

    state.setProperty(PaneIds::Id, id_.id, nullptr);
    state.setProperty(PaneIds::OrderIndex, orderIndex_, nullptr);
    state.setProperty(PaneIds::Collapsed, collapsed_, nullptr);
    state.setProperty(PaneIds::Name, name_, nullptr);
    state.setProperty(PaneIds::HeightRatio, heightRatio_, nullptr);
    state.setProperty(PaneIds::ColumnIndex, columnIndex_, nullptr);

    return state;
}

void Pane::fromValueTree(const juce::ValueTree& state)
{
    // PaneIds::Pane and StateIds::Pane are both "Pane" (interned by juce::Identifier)
    if (!state.hasType(PaneIds::Pane))
        return;

    id_.id = state.getProperty(PaneIds::Id, id_.id);
    if (!id_.isValid())
        id_.id = state.getProperty(StateIds::Id, id_.id);

    orderIndex_ = state.getProperty(PaneIds::OrderIndex, state.getProperty(StateIds::Order, 0));
    collapsed_ = state.getProperty(PaneIds::Collapsed, state.getProperty(StateIds::Collapsed, false));
    name_ = state.getProperty(PaneIds::Name, state.getProperty(StateIds::Name, ""));
    heightRatio_ = state.getProperty(PaneIds::HeightRatio, DEFAULT_HEIGHT_RATIO);
    columnIndex_ = state.getProperty(PaneIds::ColumnIndex, 0);

    // Validate
    float const safeHeightRatio = std::isnan(heightRatio_) ? DEFAULT_HEIGHT_RATIO : heightRatio_;
    heightRatio_ = juce::jlimit(MIN_HEIGHT_RATIO, MAX_HEIGHT_RATIO, safeHeightRatio);
}

} // namespace oscil
