#pragma once
#include "types.h" // for Utf32String and i32
#include <vector>
#include <optional>
#include <cstddef>

namespace hui
{

// Snapshot for text widgets: full text + caret position
struct UndoSnapshot
{
    Utf32String text;
    i32 caretLine = 0;
    i32 caretColumn = 0;
};

// Simple undo / redo stack that stores full snapshots.
// - push(state) appends a new state, truncating any redo history.
// - undo() moves to the previous snapshot and returns it (std::nullopt if none).
// - redo() moves forward and returns it (std::nullopt if none).
// The implementation keeps a bounded history (maxHistory).
struct UndoStack
{
    using State = UndoSnapshot;

    explicit UndoStack(std::size_t maxHistory = 200) noexcept;

    // Push a new snapshot. This clears any redo history.
    void push(const State& state);
    void push(State&& state);

    // Can we undo/redo?
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    // Perform undo / redo. Returns the new current state if operation succeeded.
    std::optional<State> undo();
    std::optional<State> redo();

    // Access current state without changing stack.
    std::optional<State> current() const noexcept;

    // Utility
    void clear() noexcept;
    std::size_t maxHistory() const noexcept { return m_maxHistory; }
    std::size_t historySize() const noexcept { return m_history.size(); }
    // current index in history (std::string::npos when empty)
    std::size_t currentIndex() const noexcept { return m_pos; }

private:
    std::size_t m_maxHistory;
    std::vector<State> m_history;
    // m_pos is index of current active state in m_history.
    // if m_history.empty() then m_pos == npos.
    std::size_t m_pos;
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
};

} // namespace hui