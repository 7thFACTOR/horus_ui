#include "undo.h"
#include <algorithm>

namespace hui
{

UndoStack::UndoStack(std::size_t maxHistory) noexcept
    : m_maxHistory(maxHistory), m_history(), m_pos(npos)
{
}

void UndoStack::push(const State& state)
{
    // If there is redo history (m_pos not at end) truncate it.
    if (m_pos != npos && m_pos + 1 < m_history.size())
        m_history.erase(m_history.begin() + (m_pos + 1), m_history.end());

    m_history.push_back(state);

    // Enforce max history length
    if (m_history.size() > m_maxHistory)
    {
        // drop the oldest entry
        m_history.erase(m_history.begin());
        // adjust position (current becomes last element)
        m_pos = m_history.size() - 1;
    }
    else
    {
        m_pos = m_history.size() - 1;
    }
}

void UndoStack::push(State&& state)
{
    if (m_pos != npos && m_pos + 1 < m_history.size())
        m_history.erase(m_history.begin() + (m_pos + 1), m_history.end());

    m_history.push_back(std::move(state));

    if (m_history.size() > m_maxHistory)
    {
        m_history.erase(m_history.begin());
        m_pos = m_history.size() - 1;
    }
    else
    {
        m_pos = m_history.size() - 1;
    }
}

bool UndoStack::canUndo() const noexcept
{
    return m_history.size() > 0 && m_pos != npos && m_pos > 0;
}

bool UndoStack::canRedo() const noexcept
{
    return m_history.size() > 0 && m_pos != npos && (m_pos + 1) < m_history.size();
}

std::optional<UndoStack::State> UndoStack::undo()
{
    if (!canUndo()) return std::nullopt;
    --m_pos;
    return m_history[m_pos];
}

std::optional<UndoStack::State> UndoStack::redo()
{
    if (!canRedo()) return std::nullopt;
    ++m_pos;
    return m_history[m_pos];
}

std::optional<UndoStack::State> UndoStack::current() const noexcept
{
    if (m_history.empty() || m_pos == npos) return std::nullopt;
    return m_history[m_pos];
}

void UndoStack::clear() noexcept
{
    m_history.clear();
    m_pos = npos;
}

} // namespace hui