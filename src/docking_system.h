#pragma once
#include "types.h"
#include "dock_node.h"

namespace hui
{
void handleDockNodeEvents(DockNode* node);
void handleDockNodeResize(DockNode* node);
};