#include "horus.h"
#include "view.h"
#include "docking_system.h"
#include <cstring>
#include <algorithm>
#include <math.h>
#include "context.h"

namespace hui
{
HDockNode createRootDockNode(HWindow window)
{
	auto rect = getWindowRect(window);
	auto dockNode = new DockNode();

	dockNode->type = DockNode::Type::None;
	dockNode->rect.set(0, 0, rect.width, rect.height);
	dockNode->window = window;
	ctx->dockingData.rootWindowDockNodes.insert(std::make_pair(window, dockNode));

	return dockNode;
}

HView createView(HDockNode targetNode, DockType dock, const char* title, f32 size, ViewType viewType, u64 userData)
{
	auto targetNodePtr = (DockNode*)targetNode;
	auto newView = new View();

	newView->title = title;
	newView->viewType = viewType;
	newView->userData = userData;
	newView->size = size;

	dockView(newView, targetNode, dock);

	return newView;
}

bool dockNode(HDockNode sourceNode, HDockNode targetNode, DockType dockType)
{
	DockNode* source = (DockNode*)sourceNode;
	DockNode* target = (DockNode*)targetNode;

	if (target->children.empty())
	{
		target->children = source->children;
		target->rect = source->rect;
		target->selectedTabIndex = source->selectedTabIndex;
		target->type = source->type;
		target->view = source->view;
		target->window = source->window;
		delete source; //TODO: destroy source's window also
		//TODO: recalc layout
		return true;
	}

	switch (dockType)
	{
	case hui::DockType::Left:
		target->type = DockNode::Type::Horizontal;
		target->children.insert(target->children.begin(), source);
		break;
	case hui::DockType::Right:
		target->type = DockNode::Type::Horizontal;
		target->children.push_back(source);
		break;
	case hui::DockType::Top:
		target->type = DockNode::Type::Vertical;
		target->children.insert(target->children.begin(), source);
		break;
	case hui::DockType::Bottom:
		target->type = DockNode::Type::Vertical;
		target->children.push_back(source);
		break;
	case hui::DockType::RootLeft:
		if (target->type == DockNode::Type::Horizontal)
		{
			
		}

		break;
	case hui::DockType::RootRight:
		break;
	case hui::DockType::RootTop:
		break;
	case hui::DockType::RootBottom:
		break;
	case hui::DockType::AsTab:
		break;
	default:
		break;
	}

	return true;
}

}
