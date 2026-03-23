/*
    Oscil - Pane Implementation
    PRD aligned with column distribution and drag-drop support
*/

#include "core/Pane.h"
#include "core/OscilState.h"
#include <algorithm>
#include <cmath>

namespace oscil
{

Pane::Pane()
    : id_(PaneId::generate())
{
}

Pane::Pane(const juce::ValueTree& state)
    : id_(PaneId::generate())
{
    fromValueTree(state);
}

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

    orderIndex_ = state.getProperty(PaneIds::OrderIndex,
                  state.getProperty(StateIds::Order, 0));
    collapsed_ = state.getProperty(PaneIds::Collapsed,
                 state.getProperty(StateIds::Collapsed, false));
    name_ = state.getProperty(PaneIds::Name,
            state.getProperty(StateIds::Name, ""));
    heightRatio_ = state.getProperty(PaneIds::HeightRatio, DEFAULT_HEIGHT_RATIO);
    columnIndex_ = state.getProperty(PaneIds::ColumnIndex, 0);

    // Validate
    float safeHeightRatio = std::isnan(heightRatio_) ? DEFAULT_HEIGHT_RATIO : heightRatio_;
    heightRatio_ = juce::jlimit(MIN_HEIGHT_RATIO, MAX_HEIGHT_RATIO, safeHeightRatio);
}

// PaneLayoutManager implementation

void PaneLayoutManager::setColumnLayout(ColumnLayout layout)
{
    if (columnLayout_ == layout)
        return;  // No change, preserve existing pane arrangement

    columnLayout_ = layout;
    redistributePanes();
    notifyColumnLayoutChanged();
}

void PaneLayoutManager::addPane(const Pane& pane)
{
    // Determine target column BEFORE adding the pane
    // This prevents the newly added pane (with default columnIndex=0)
    // from being counted in getPaneCountInColumn
    int minPanes = INT_MAX;
    int targetColumn = 0;
    int numColumns = getColumnCount();

    for (int c = 0; c < numColumns; ++c)
    {
        int count = getPaneCountInColumn(c);
        if (count < minPanes)
        {
            minPanes = count;
            targetColumn = c;
        }
    }

    // Now add the pane with correct column assignment
    panes_.push_back(pane);
    panes_.back().setColumnIndex(targetColumn);
    panes_.back().setOrderIndex(static_cast<int>(panes_.size()) - 1);

    notifyPaneAdded(pane.getId());
    sortPanesByOrder();
}

void PaneLayoutManager::removePane(const PaneId& paneId)
{
    auto it = std::find_if(panes_.begin(), panes_.end(),
        [&paneId](const Pane& p) { return p.getId() == paneId; });

    if (it != panes_.end())
    {
        panes_.erase(it);
        notifyPaneRemoved(paneId);

        // Re-index remaining panes
        for (size_t i = 0; i < panes_.size(); ++i)
        {
            panes_[i].setOrderIndex(static_cast<int>(i));
        }
    }
}

std::vector<Pane*> PaneLayoutManager::getPanesInColumn(int column)
{
    std::vector<Pane*> result;
    for (auto& pane : panes_)
    {
        if (pane.getColumnIndex() == column)
        {
            result.push_back(&pane);
        }
    }
    // Sort by order index within column
    std::sort(result.begin(), result.end(),
        [](const Pane* a, const Pane* b) {
            return a->getOrderIndex() < b->getOrderIndex();
        });
    return result;
}

std::vector<const Pane*> PaneLayoutManager::getPanesInColumn(int column) const
{
    std::vector<const Pane*> result;
    for (const auto& pane : panes_)
    {
        if (pane.getColumnIndex() == column)
        {
            result.push_back(&pane);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const Pane* a, const Pane* b) {
            return a->getOrderIndex() < b->getOrderIndex();
        });
    return result;
}

Pane* PaneLayoutManager::getPane(const PaneId& paneId)
{
    auto it = std::find_if(panes_.begin(), panes_.end(),
        [&paneId](const Pane& p) { return p.getId() == paneId; });

    return it != panes_.end() ? &(*it) : nullptr;
}

const Pane* PaneLayoutManager::getPane(const PaneId& paneId) const
{
    auto it = std::find_if(panes_.begin(), panes_.end(),
        [&paneId](const Pane& p) { return p.getId() == paneId; });

    return it != panes_.end() ? &(*it) : nullptr;
}

void PaneLayoutManager::movePane(const PaneId& paneId, int newIndex)
{
    auto it = std::find_if(panes_.begin(), panes_.end(),
        [&paneId](const Pane& p) { return p.getId() == paneId; });

    if (it == panes_.end())
        return;

    // Get the pane
    Pane pane = *it;
    panes_.erase(it);

    // Insert at new position
    newIndex = std::clamp(newIndex, 0, static_cast<int>(panes_.size()));
    panes_.insert(panes_.begin() + newIndex, pane);

    // Update order indices
    for (size_t i = 0; i < panes_.size(); ++i)
    {
        panes_[i].setOrderIndex(static_cast<int>(i));
    }

    notifyPaneOrderChanged();
}

void PaneLayoutManager::movePaneToColumn(const PaneId& paneId, int targetColumn, int positionInColumn)
{
    Pane* pane = getPane(paneId);
    if (!pane)
        return;

    int numColumns = getColumnCount();
    targetColumn = std::clamp(targetColumn, 0, numColumns - 1);

    // Get target column panes BEFORE moving (excluding the pane being moved)
    // This ensures we can insert at the correct position
    std::vector<Pane*> targetPanes;
    for (auto& p : panes_)
    {
        if (p.getColumnIndex() == targetColumn && p.getId() != paneId)
            targetPanes.push_back(&p);
    }
    std::sort(targetPanes.begin(), targetPanes.end(),
        [](Pane* a, Pane* b) { return a->getOrderIndex() < b->getOrderIndex(); });

    positionInColumn = std::clamp(positionInColumn, 0, static_cast<int>(targetPanes.size()));

    // Insert pane at the requested position in target column
    targetPanes.insert(targetPanes.begin() + positionInColumn, pane);
    pane->setColumnIndex(targetColumn);

    // Recalculate order indices for all panes
    int globalIndex = 0;
    for (int col = 0; col < numColumns; ++col)
    {
        if (col == targetColumn)
        {
            // Use our ordered list for target column
            for (auto* p : targetPanes)
            {
                p->setOrderIndex(globalIndex++);
            }
        }
        else
        {
            auto panesInCol = getPanesInColumn(col);
            for (auto* p : panesInCol)
            {
                p->setOrderIndex(globalIndex++);
            }
        }
    }

    notifyPaneOrderChanged();
}

int PaneLayoutManager::getPaneCountInColumn(int column) const
{
    return static_cast<int>(std::count_if(panes_.begin(), panes_.end(),
        [column](const Pane& p) { return p.getColumnIndex() == column; }));
}

int PaneLayoutManager::getColumnForPane(int paneIndex) const
{
    if (paneIndex >= 0 && static_cast<size_t>(paneIndex) < panes_.size())
    {
        return panes_[static_cast<size_t>(paneIndex)].getColumnIndex();
    }

    // Fallback: row-major distribution
    int numColumns = getColumnCount();
    if (numColumns <= 1)
        return 0;

    return paneIndex % numColumns;
}

juce::Rectangle<int> PaneLayoutManager::getPaneBounds(int paneIndex, juce::Rectangle<int> availableArea) const
{
    if (panes_.empty() || paneIndex < 0 || static_cast<size_t>(paneIndex) >= panes_.size())
        return {};

    const Pane& pane = panes_[static_cast<size_t>(paneIndex)];
    int numColumns = getColumnCount();

    // Guard against division by zero
    if (numColumns <= 0)
        return {};

    // Calculate column width
    int colWidth = availableArea.getWidth() / numColumns;
    int column = pane.getColumnIndex();

    // Get panes in this column
    auto columnPanes = getPanesInColumn(column);

    // Calculate total height ratio for this column
    float totalRatio = 0.0f;
    int panePositionInColumn = -1;
    for (size_t i = 0; i < columnPanes.size(); ++i)
    {
        totalRatio += columnPanes[i]->getHeightRatio();
        if (columnPanes[i]->getId() == pane.getId())
        {
            panePositionInColumn = static_cast<int>(i);
        }
    }

    if (panePositionInColumn < 0)
        return {};

    // Guard against division by zero
    if (totalRatio <= 0.0f)
        return {};

    // Calculate Y position and height
    float yRatio = 0.0f;
    for (int i = 0; i < panePositionInColumn; ++i)
    {
        yRatio += columnPanes[static_cast<size_t>(i)]->getHeightRatio();
    }

    int colX = availableArea.getX() + column * colWidth;
    int paneY = availableArea.getY() + static_cast<int>((yRatio / totalRatio) * static_cast<float>(availableArea.getHeight()));
    int paneHeight = static_cast<int>((pane.getHeightRatio() / totalRatio) * static_cast<float>(availableArea.getHeight()));

    // Add small margin
    const int margin = 2;
    return juce::Rectangle<int>(colX + margin, paneY + margin,
                                colWidth - 2 * margin, paneHeight - 2 * margin);
}

juce::Rectangle<int> PaneLayoutManager::getPaneBoundsInColumn(const PaneId& paneId, juce::Rectangle<int> columnArea) const
{
    const Pane* pane = getPane(paneId);
    if (!pane)
        return {};

    auto columnPanes = getPanesInColumn(pane->getColumnIndex());

    // Calculate total height ratio
    float totalRatio = 0.0f;
    int panePosition = -1;
    for (size_t i = 0; i < columnPanes.size(); ++i)
    {
        totalRatio += columnPanes[i]->getHeightRatio();
        if (columnPanes[i]->getId() == paneId)
        {
            panePosition = static_cast<int>(i);
        }
    }

    if (panePosition < 0)
        return {};

    // Guard against division by zero
    if (totalRatio <= 0.0f)
        return {};

    // Calculate Y position
    float yRatio = 0.0f;
    for (int i = 0; i < panePosition; ++i)
    {
        yRatio += columnPanes[static_cast<size_t>(i)]->getHeightRatio();
    }

    int paneY = columnArea.getY() + static_cast<int>((yRatio / totalRatio) * static_cast<float>(columnArea.getHeight()));
    int paneHeight = static_cast<int>((pane->getHeightRatio() / totalRatio) * static_cast<float>(columnArea.getHeight()));

    const int margin = 2;
    return juce::Rectangle<int>(columnArea.getX() + margin, paneY + margin,
                                columnArea.getWidth() - 2 * margin, paneHeight - 2 * margin);
}

void PaneLayoutManager::redistributePanes()
{
    if (panes_.empty())
        return;

    int numColumns = getColumnCount();

    // Guard against division/modulo by zero
    if (numColumns <= 0)
        return;

    // PRD: Round-robin distribution to balance columns
    for (size_t i = 0; i < panes_.size(); ++i)
    {
        int newCol = static_cast<int>(i) % numColumns;
        panes_[i].setColumnIndex(newCol);
        panes_[i].setOrderIndex(static_cast<int>(i));
    }

    notifyPaneOrderChanged();
}

juce::ValueTree PaneLayoutManager::toValueTree() const
{
    juce::ValueTree state(PaneIds::Panes);

    state.setProperty(PaneIds::ColumnLayout, static_cast<int>(columnLayout_), nullptr);

    for (const auto& pane : panes_)
    {
        state.appendChild(pane.toValueTree(), nullptr);
    }

    return state;
}

void PaneLayoutManager::fromValueTree(const juce::ValueTree& state)
{
    panes_.clear();

    // PaneIds::Panes and StateIds::Panes are both "Panes" (interned by juce::Identifier)
    if (!state.hasType(PaneIds::Panes))
        return;

    // Load column layout
    if (state.hasProperty(PaneIds::ColumnLayout))
    {
        int layout = state.getProperty(PaneIds::ColumnLayout, 1);
        columnLayout_ = static_cast<ColumnLayout>(std::clamp(layout, 1, 3));
    }

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(PaneIds::Pane))
        {
            panes_.emplace_back(child);
        }
    }

    sortPanesByOrder();

    // Normalize invalid column indices, preserve valid saved positions
    if (!panes_.empty())
    {
        int numColumns = getColumnCount();
        for (auto& pane : panes_)
        {
            if (pane.getColumnIndex() < 0)
            {
                pane.setColumnIndex(0);
            }
            else if (pane.getColumnIndex() >= numColumns)
            {
                pane.setColumnIndex(pane.getColumnIndex() % numColumns);
            }
        }
    }
}

void PaneLayoutManager::sortPanesByOrder()
{
    std::sort(panes_.begin(), panes_.end(),
        [](const Pane& a, const Pane& b) {
            return a.getOrderIndex() < b.getOrderIndex();
        });
}

void PaneLayoutManager::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void PaneLayoutManager::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void PaneLayoutManager::notifyColumnLayoutChanged()
{
    listeners_.call([this](Listener& l) { l.columnLayoutChanged(columnLayout_); });
}

void PaneLayoutManager::notifyPaneOrderChanged()
{
    listeners_.call([](Listener& l) { l.paneOrderChanged(); });
}

void PaneLayoutManager::notifyPaneAdded(const PaneId& paneId)
{
    listeners_.call([&paneId](Listener& l) { l.paneAdded(paneId); });
}

void PaneLayoutManager::notifyPaneRemoved(const PaneId& paneId)
{
    listeners_.call([&paneId](Listener& l) { l.paneRemoved(paneId); });
}

} // namespace oscil
