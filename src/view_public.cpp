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
	auto source = viewObj->dockNode;
	DockNode* target = (DockNode*)targetNode;

	if (source == target)
		return false;

	// if this is the root and its empty
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
		// just insert at the target site
		if (target->parent->type == DockNode::Type::Horizontal)
		{
			auto iter = std::find(target->parent->children.begin(), target->parent->children.end(), target);
			
			// if there is just one view in the source node, move the node also and remove from current parent
			if (source->views.size() == 1)
			{
				source->parent->removeFromParent();
				source->parent = target->parent;
				source->window = target->window;
				// insert it before target
				target->parent->children.insert(iter, source);
			}
			else
			// we create a new node to hold the view
			{
				source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = target->parent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				target->parent->children.insert(iter, newNode);
			}
		}
		else if (target->parent->type == DockNode::Type::Vertical)
		{
			if (source->views.size() == 1)
			{
				source->parent->removeFromParent();
				source->parent = target;
				sourceNode = source;
			}
			else
			{
				source->removeView(viewObj);
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = target;
				sourceNode->type = DockNode::Type::ViewTabs;
			}

			// move target content into new node
			DockNode* newNode = new DockNode();
			*newNode = *target;
			newNode->parent = target;
			for (auto& c : newNode->children) c->parent = newNode;
			target->children.clear();
			target->children.push_back(sourceNode);
			target->children.push_back(newNode);
		}
		break;
	case hui::DockType::Right:
		break;
	case hui::DockType::Top:
		break;
	case hui::DockType::Bottom:
		break;
	case hui::DockType::RootLeft:
		// if we only have a view in the source node, grab the node itself
		if (source->views.size() == 1)
		{
			sourceNode = source;
		}
		else
		{
			// remove from the source node
			source->removeView(viewObj);

			// create new node
			sourceNode = new DockNode();
			sourceNode->type = DockNode::Type::ViewTabs;
			sourceNode->views.push_back(viewObj);
		}

		// reparent the source node
		sourceNode->parent = target;
		sourceNode->window = target->window;

		// if same layout, add the new node to the left
		if (target->type == DockNode::Type::Horizontal)
		{
			target->children.insert(target->children.begin(), sourceNode);
		}
		// if layout is different
		else if (target->type == DockNode::Type::Vertical)
		{
			// reparent all the children to a new node
			DockNode* newNode = new DockNode();
			newNode->parent = target;
			newNode->children = target->children;
			newNode->type = target->type;
			newNode->window = target->window;

			for (auto& c : newNode->children) c->parent = newNode;
			
			target->children.clear();
			target->children.push_back(sourceNode);
			target->children.push_back(newNode);
		}
		// if target is a view or tabs, create a horizontal container node
		else
		{
			DockNode* newNode = new DockNode();
			newNode->parent = target;
			newNode->views = target->views;
			newNode->type = target->type;
			newNode->window = target->window;
			newNode->selectedTabIndex = target->selectedTabIndex;

			for (auto& c : newNode->children) c->parent = newNode;

			target->type = DockNode::Type::Horizontal;
			target->children.push_back(sourceNode);
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
