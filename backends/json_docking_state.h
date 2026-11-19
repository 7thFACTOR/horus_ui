#pragma once
#include <horus.h>

namespace hui
{
/// Serialize a docking layout state to a JSON string
/// \param dockingState the state to serialize
HUI_API std::string dockingStateToJson(const WindowsDockingState& dockingState);
/// Parse a docking layout state from a JSON string
/// \param dockingState the state to fill
/// \param json the JSON text
/// \return true if the JSON was valid
HUI_API bool dockingStateFromJson(WindowsDockingState& dockingState, const char* json);
/// Save the current docking layout to a JSON file, uses the Horus file I/O services
/// \param filename the JSON filename, relative to executable
/// \return true if the file was written
HUI_API bool dockingStateSaveToFile(const char* filename);
/// Load a docking layout from a JSON file and apply it, uses the Horus file I/O services
/// \param filename the JSON filename, relative to executable
/// \return true if the file was read and the layout was loaded
HUI_API bool dockingStateLoadFromFile(const char* filename);
}
