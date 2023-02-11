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

HDockNode createView(HDockNode targetNode, DockType dock, const char* title, f32 size, ViewType viewType, u64 userData)
{
	auto targetNodePtr = (DockNode*)targetNode;
	auto newView = new View();

	newView->title = title;
	newView->viewType = viewType;
	newView->userData = userData;

	dockView(newView, targetNode, dock);

	return newView;
}

bool dockView(HView view, HDockNode targetNode, DockType dockType)
{
	auto viewObj = (View*)view;
	auto source = viewObj->parent;
	DockNode* target = (DockNode*)targetNode;

	if (source == target)
		return false;


	if (target->children.empty() && target->views.empty())
	{
		target->views.push_back(viewObj);
		target->type = DockNode::Type::ViewTabs;
		target->selectedTabIndex = 0;
		//TODO: recalc layout
		return true;
	}

	DockNode* sourceNode = nullptr;

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
		
		if (source->views.size() == 1)
		{
			sourceNode = source;
		}
		else
		{
			source->removeView(viewObj);
			sourceNode = new DockNode();
			sourceNode->type = DockNode::Type::ViewTabs;
			sourceNode->views.push_back(viewObj);
		}

		sourceNode->parent = target;

		// if same layout
		if (target->type == DockNode::Type::Horizontal)
		{
			target->children.insert(target->children.begin(), sourceNode);
		}
		// if layout is different
		else if (target->type == DockNode::Type::Vertical)
		{
			newNode->parent = target;
			newNode->children = target->children;
			for (auto& c : newNode->children) c->parent = newNode;
			newNode->type = target->type = DockNode::Type::Vertical;
			newNode->window = target->window;
			target->children.clear();
			target->children.push_back(source);
			target->children.push_back(newNode);
		}
		// if target is a view or tabs, create a horizontal container node
		else
		{
			DockNode* newNode = new DockNode();
			newNode->parent = target;
			newNode->view = target->view;
			newNode->type = target->type;
			newNode->window = target->window;
			newNode->views = target->views;
			target->type = DockNode::Type::Horizontal;
			target->children.push_back(source);
			target->children.push_back(newNode);
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
