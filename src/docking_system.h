#pragma once
#include "types.h"
#include "dock_node.h"

namespace hui
{
void updateDockingSystem();
void handleDockNodeEvents(DockNode* node);
void handleDockingMouseUp();
};