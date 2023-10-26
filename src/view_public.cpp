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
	ctx->dockingState.rootWindowDockNodes.insert(std::make_pair(window, dockNode));

	return dockNode;
}

HDockNode getRootDockNode(HWindow window)
{
	return ctx->dockingState.rootWindowDockNodes[window];
}

void deleteRootDockNode(HWindow window)
{
	auto node = ctx->dockingState.rootWindowDockNodes[window];

	if (node)
	{
		ctx->dockingState.rootWindowDockNodes.erase(window);
		delete node;
	}
}

HView createView(HDockNode targetNode, DockType dock, const char* title, f32 size, ViewType viewType, u64 userData, HImage icon)
{
	auto targetNodePtr = (DockNode*)targetNode;
	auto newView = new View();

	newView->title = title;
	newView->viewType = viewType;
	newView->userData = userData;
	newView->icon = icon;

	dockView(newView, targetNode, dock);

	return newView;
}

void deleteView(HView view)
{
	View* viewObj = (View*)view;
	DockNode* node = viewObj->dockNode;

	node->removeView(viewObj);

	if (!node->parent)
	{
		destroyWindow(node->window);
		ctx->dockingState.rootWindowDockNodes.erase(node->window);
		delete node;
	}
}

HWindow getViewWindow(HView view)
{
	View* viewObj = (View*)view;

	return viewObj->dockNode->window;
}

HDockNode getViewDockNode(HView view)
{
	View* viewObj = (View*)view;

	return viewObj->dockNode;
}

bool dockView(HView view, HDockNode targetNode, DockType dockType, u32 tabIndex)
{
	auto viewObj = (View*)view;
	auto source = viewObj->dockNode;
	DockNode* target = (DockNode*)targetNode;
	auto targetIsRoot = !target->parent;

	if (source == target)
		return false;

	// if this is the root and its empty of any children and views
	if (targetIsRoot && target->children.empty() && target->views.empty() && target->type == DockNode::Type::None)
	{
		target->views.push_back(viewObj);
		viewObj->dockNode = target;
		
		switch (dockType)
		{
		case DockType::Left:
		case DockType::Right:
		case DockType::RootLeft:
		case DockType::RootRight:
			target->type = DockNode::Type::Horizontal;
			break;
		case DockType::Top:
		case DockType::Bottom:
		case DockType::RootTop:
		case DockType::RootBottom:
			target->type = DockNode::Type::Vertical;
			break;
		default:
			break;
		}
		target->selectedTabIndex = 0;
		return true;
	}

	DockNode* sourceNode = nullptr;
	DockNode* targetParent = nullptr;

	if (!target->parent)
	{
		targetParent = target;
	}
	else
	{
		targetParent = target->parent;
	}

	auto checkRelocateViewsOfNode = [](DockNode* targetParent)
	{
		DockNode* newNode = nullptr;

		// if there are views but no children nodes, create one
		if (!targetParent->views.empty() && targetParent->children.empty())
		{
			newNode = new DockNode();

			newNode->parent = targetParent;
			newNode->rect = targetParent->rect;
			newNode->selectedTabIndex = targetParent->selectedTabIndex;
			newNode->type = targetParent->type;
			newNode->views = targetParent->views;
			newNode->window = targetParent->window;
			for (auto& v : newNode->views) { v->dockNode = newNode; }
			targetParent->views.clear();
			targetParent->children.push_back(newNode);

			return newNode;
		}

		return newNode;
	};

	switch (dockType)
	{
	case hui::DockType::Left:
	{	// just insert at the target site
		if (targetParent->type == DockNode::Type::Horizontal)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);
			
			if (newTarget) target = newTarget;

			auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

			// if there is just one view in the source node, move the node and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;

				// insert it before target
				if (iter != targetParent->children.end())
				{
					targetParent->children.insert(iter, source);
				}

				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				if (source)	source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				targetParent->children.insert(iter, newNode);
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && targetParent) targetParent->computeRect();
		}
		else if (targetParent->type == DockNode::Type::Vertical)
		{
			// if source has one view, remove source from its parent
			// and just relocate to target parent node
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				sourceNode = source;
			}
			else
			{
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->type = DockNode::Type::ViewTabs;
			}

			// relocate target's content into new node
			DockNode* newTargetNode = new DockNode();
			*newTargetNode = *target;
			newTargetNode->parent = target;
			for (auto& c : newTargetNode->children) c->parent = newTargetNode;
			target->children.clear();
			target->children.push_back(sourceNode);
			target->children.push_back(newTargetNode);

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = target->rect.width * ctx->settings.dockNodeDockingSizeRatio;
			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;
			target->rect.width -= size;
			target->rect.x -= size;
		}

		break;
	}
	case hui::DockType::Right:
	{	// just insert at the target site
		if (targetParent->type == DockNode::Type::Horizontal)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);

			if (newTarget) target = newTarget;

			auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

			// we will insert after it
			++iter;

			// if there is just one view in the source node, move the node and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;
				
				if (iter != targetParent->children.end())
				{
					targetParent->children.insert(iter, source);
				}
				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				if (source) source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				targetParent->children.insert(iter, newNode);
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && targetParent) targetParent->computeRect();
		}
		else if (targetParent->type == DockNode::Type::Vertical)
		{
			// if source has one view, remove source from its parent
			// and just relocate to target parent node
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				sourceNode = source;
			}
			else
			{
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->type = DockNode::Type::ViewTabs;
			}

			// relocate target's content into new node
			DockNode* newNode = new DockNode();
			*newNode = *target;
			newNode->parent = targetParent;
			for (auto& c : newNode->children) c->parent = newNode;
			target->children.clear();
			target->children.push_back(newNode);
			target->children.push_back(sourceNode); // insert last

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = target->rect.width * ctx->settings.dockNodeDockingSizeRatio;
			sourceNode->rect.x = target->rect.right() - size;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;
			target->rect.width -= size;
		}
		break;
	}
	case hui::DockType::Top:
	{	// just insert at the target site
		if (targetParent->type == DockNode::Type::Vertical)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);

			if (newTarget) target = newTarget;

			auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;
				
				// insert it before target
				targetParent->children.insert(iter, source);
				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				if (source) source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				targetParent->children.insert(iter, newNode);
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && targetParent) targetParent->computeRect();
		}
		else if (targetParent->type == DockNode::Type::Horizontal)
		{
			// if source has one view, remove source from its parent
			// and just relocate to target parent node
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = target;
				sourceNode = source;
			}
			else
			{
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = target;
				sourceNode->type = DockNode::Type::ViewTabs;
			}

			// relocate target's content into new node
			DockNode* newNode = new DockNode();
			*newNode = *target;
			newNode->parent = target;
			for (auto& c : newNode->children) c->parent = newNode;
			target->children.clear();
			target->children.push_back(sourceNode);
			target->children.push_back(newNode);

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = target->rect.height * ctx->settings.dockNodeDockingSizeRatio;
			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = target->rect.width;
			sourceNode->rect.height = size;
			target->rect.height -= size;
			target->rect.y -= size;
		}
		break;
	}
	case hui::DockType::Bottom:
	{	// just insert at the target site
		if (targetParent->type == DockNode::Type::Vertical)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);

			if (newTarget) target = newTarget;

			auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

			// we will insert after it
			++iter;

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;

				if (iter != targetParent->children.end())
				{
					targetParent->children.insert(iter, source);
				}
				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				if (source) source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				targetParent->children.insert(iter, newNode);
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && targetParent) targetParent->computeRect();
		}
		else if (targetParent->type == DockNode::Type::Horizontal)
		{
			// if source has one view, remove source from its parent
			// and just relocate to target parent node
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = target;
				sourceNode = source;
			}
			else
			{
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = target;
				sourceNode->type = DockNode::Type::ViewTabs;
			}

			// relocate target's content into new node
			DockNode* newNode = new DockNode();
			*newNode = *target;
			newNode->parent = target;
			for (auto& c : newNode->children) c->parent = newNode;
			target->children.clear();
			target->children.push_back(newNode);
			target->children.push_back(sourceNode); // insert last

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = target->rect.height * ctx->settings.dockNodeDockingSizeRatio;
			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.bottom() - size;
			sourceNode->rect.width = target->rect.width;
			sourceNode->rect.height = size;
			target->rect.height -= size;
		}
		break;
	}
	case hui::DockType::RootLeft:
	{	// if we only have a view in the source node, grab the node itself
		if (source && source->views.size() == 1)
		{
			source->removeFromParent();
			sourceNode = source;
		}
		else
		{
			// remove from the source node
			if (source) source->removeView(viewObj);

			// create new node
			sourceNode = new DockNode();
			sourceNode->type = DockNode::Type::ViewTabs;
			sourceNode->views.push_back(viewObj);
			viewObj->dockNode = sourceNode;
		}

		// reparent the source node
		sourceNode->parent = target;
		sourceNode->window = target->window;
		DockNode* newNode = nullptr;

		// if same layout, add the new node to the left
		if (target->type == DockNode::Type::Horizontal)
		{
			target->children.insert(target->children.begin(), sourceNode);
		}
		// if layout is different
		else if (target->type == DockNode::Type::Vertical)
		{
			// reparent all the children to a new node
			newNode = new DockNode();
			newNode->parent = target;
			newNode->children = target->children;
			newNode->type = target->type;
			newNode->window = target->window;

			for (auto& c : newNode->children) c->parent = newNode;

			target->children.clear();
			target->children.push_back(sourceNode);
			target->children.push_back(newNode);
		}
		// if target is tabs, create a container node
		else
		{
			newNode = new DockNode();
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

		if (ctx->settings.dockNodeProportionalResize) target->computeRect();

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& target->type != DockNode::Type::None)
		{
			f32 size = target->rect.width * ctx->settings.dockNodeDockingSizeRatio;

			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;

			if (newNode)
			{
				newNode->rect.x = target->rect.x + size;
				newNode->rect.y = target->rect.y;
				newNode->rect.width = target->rect.width - size;
				newNode->rect.height = target->rect.height;
			}
		}

		break;
	}
	case hui::DockType::RootRight:
	{	// if we only have a view in the source node, grab the node itself
		if (source && source->views.size() == 1)
		{
			source->removeFromParent();
			sourceNode = source;
		}
		else
		{
			// remove from the source node
			if (source) source->removeView(viewObj);

			// create new node
			sourceNode = new DockNode();
			sourceNode->type = DockNode::Type::ViewTabs;
			sourceNode->views.push_back(viewObj);
			viewObj->dockNode = sourceNode;
		}

		// reparent the source node
		sourceNode->parent = target;
		sourceNode->window = target->window;
		DockNode* newNode = nullptr;

		// if same layout, add the new node to the left
		if (target->type == DockNode::Type::Horizontal)
		{
			target->children.push_back(sourceNode);
		}
		// if layout is different
		else if (target->type == DockNode::Type::Vertical)
		{
			// reparent all the children to a new node
			newNode = new DockNode();
			newNode->parent = target;
			newNode->children = target->children;
			newNode->type = target->type;
			newNode->window = target->window;

			for (auto& c : newNode->children) c->parent = newNode;

			target->children.clear();
			target->children.push_back(newNode);
			target->children.push_back(sourceNode);
		}
		// if target is tabs, create a container node
		else
		{
			newNode = new DockNode();
			newNode->parent = target;
			newNode->views = target->views;
			newNode->type = target->type;
			newNode->window = target->window;
			newNode->selectedTabIndex = target->selectedTabIndex;

			for (auto& c : newNode->children) c->parent = newNode;

			target->type = DockNode::Type::Horizontal;
			target->children.push_back(newNode);
			target->children.push_back(sourceNode);
		}

		if (ctx->settings.dockNodeProportionalResize) target->computeRect();

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& target->type != DockNode::Type::None)
		{
			f32 size = target->rect.width * ctx->settings.dockNodeDockingSizeRatio;

			sourceNode->rect.x = target->rect.right() - size;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;

			if (newNode)
			{
				newNode->rect.x = target->rect.x;
				newNode->rect.y = target->rect.y;
				newNode->rect.width = target->rect.width - size;
				newNode->rect.height = target->rect.height;
			}
		}
		break;
	}
	case hui::DockType::RootTop:
	{	// if we only have a view in the source node, grab the node itself
		if (source && source->views.size() == 1)
		{
			source->removeFromParent();
			sourceNode = source;
		}
		else
		{
			// remove from the source node
			if (source) source->removeView(viewObj);

			// create new node
			sourceNode = new DockNode();
			sourceNode->type = DockNode::Type::ViewTabs;
			sourceNode->views.push_back(viewObj);
			viewObj->dockNode = sourceNode;
		}

		// reparent the source node
		sourceNode->parent = target;
		sourceNode->window = target->window;
		DockNode* newNode = nullptr;

		// if same layout, add the new node to the left
		if (target->type == DockNode::Type::Vertical)
		{
			target->children.insert(target->children.begin(), sourceNode);
		}
		// if layout is different
		else if (target->type == DockNode::Type::Horizontal)
		{
			// reparent all the children to a new node
			newNode = new DockNode();
			newNode->parent = target;
			newNode->children = target->children;
			newNode->type = target->type;
			newNode->window = target->window;

			for (auto& c : newNode->children) c->parent = newNode;

			target->children.clear();
			target->children.push_back(sourceNode);
			target->children.push_back(newNode);
		}
		// if target is tabs, create a container node
		else
		{
			newNode = new DockNode();
			newNode->parent = target;
			newNode->views = target->views;
			newNode->type = target->type;
			newNode->window = target->window;
			newNode->selectedTabIndex = target->selectedTabIndex;

			for (auto& c : newNode->children) c->parent = newNode;

			target->type = DockNode::Type::Vertical;
			target->children.push_back(sourceNode);
			target->children.push_back(newNode);
		}

		if (ctx->settings.dockNodeProportionalResize) target->computeRect();

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& target->type != DockNode::Type::None)
		{
			f32 size = target->rect.height * ctx->settings.dockNodeDockingSizeRatio;

			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = target->rect.width;
			sourceNode->rect.height = size;

			if (newNode)
			{
				newNode->rect.x = target->rect.x;
				newNode->rect.y = target->rect.y + size;
				newNode->rect.width = target->rect.width;
				newNode->rect.height = target->rect.height - size;
			}
		}
		break;
	}
	case hui::DockType::RootBottom:
	{	// if we only have a view in the source node, grab the node itself
		if (source && source->views.size() == 1)
		{
			source->removeFromParent();
			sourceNode = source;
		}
		else
		{
			// remove from the source node
			if (source) source->removeView(viewObj);

			// create new node
			sourceNode = new DockNode();
			sourceNode->type = DockNode::Type::ViewTabs;
			sourceNode->views.push_back(viewObj);
			viewObj->dockNode = sourceNode;
		}

		// reparent the source node
		sourceNode->parent = target;
		sourceNode->window = target->window;
		DockNode* newNode = nullptr;

		// if same layout, add the new node to the bottom
		if (target->type == DockNode::Type::Vertical)
		{
			target->children.insert(target->children.begin(), sourceNode);
		}
		// if layout is different
		else if (target->type == DockNode::Type::Horizontal)
		{
			// reparent all the children to a new node
			newNode = new DockNode();
			newNode->parent = target;
			newNode->children = target->children;
			newNode->type = target->type;
			newNode->window = target->window;

			for (auto& c : newNode->children) c->parent = newNode;

			target->children.clear();
			target->children.push_back(newNode);
			target->children.push_back(sourceNode);
		}
		// if target is tabs, create a container node
		else
		{
			newNode = new DockNode();
			newNode->parent = target;
			newNode->views = target->views;
			newNode->type = target->type;
			newNode->window = target->window;
			newNode->selectedTabIndex = target->selectedTabIndex;

			for (auto& c : newNode->children) c->parent = newNode;

			target->type = DockNode::Type::Vertical;
			target->children.push_back(newNode);
			target->children.push_back(sourceNode);
		}

		if (ctx->settings.dockNodeProportionalResize) target->computeRect();

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& target->type != DockNode::Type::None)
		{
			f32 size = target->rect.height * ctx->settings.dockNodeDockingSizeRatio;

			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.bottom() - size;
			sourceNode->rect.width = target->rect.width;
			sourceNode->rect.height = size;

			if (newNode)
			{
				newNode->rect.x = target->rect.x;
				newNode->rect.y = target->rect.y;
				newNode->rect.width = target->rect.width;
				newNode->rect.height = target->rect.height - size;
			}
		}
		break;
	}
	case hui::DockType::AsTab:
	{
		auto iter = std::next(target->views.begin(), tabIndex);
		target->views.insert(iter, viewObj);
		target->type = DockNode::Type::ViewTabs;
		target->selectedTabIndex = tabIndex;

		// if we only have a view in the source node, delete the node
		if (source && source->views.size() == 1)
		{
			source->removeFromParent();
			delete source;
		}
		else
		{
			// remove from the source node
			if (source) source->removeView(viewObj);
		}
		break;
	}
	default:
		break;
	}

	return true;
}

ViewType dockNodeTabs(DockNode* node)
{
	if (ctx->layoutStack.back().width <= (node->views.size() * (ctx->paneGroupState.tabWidth + ctx->paneGroupState.sideSpacing)) * ctx->globalScale)
	{
		ctx->paneGroupState.forceTabWidth = ctx->layoutStack.back().width / (f32)node->views.size();
		ctx->paneGroupState.forceSqueezeTabs = true;
	}
	else
	{
		ctx->paneGroupState.forceSqueezeTabs = false;
	}

	u32 closeTabIndex = ~0;

	ctx->drawingViewPaneTabs = true;
	beginTabGroup(node->selectedTabIndex);

	for (size_t i = 0; i < node->views.size(); i++)
	{
		hui::tab(node->views[i]->title.c_str(), node->views[i]->icon);
		node->views[i]->tabRect = ctx->widget.rect;

		if (ctx->widget.hovered
			&& ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Middle)
		{
			// close tab
			closeTabIndex = i;
			ctx->event.type = InputEvent::Type::None;
			//TODO: issue some event on tab close ?
		}
	}

	u32 selectedIndex = hui::endTabGroup();

	ctx->drawingViewPaneTabs = false;

	if (closeTabIndex != ~0)
	{
		node->removeView(node->views[closeTabIndex]);

		if (selectedIndex >= node->views.size())
			selectedIndex = node->views.size() - 1;
	}

	if (!node->views.empty())
	{
		if (node->selectedTabIndex != selectedIndex)
		{
			node->selectedTabIndex = selectedIndex;
			hui::forceRepaint();
		}

		return node->views[node->selectedTabIndex]->viewType;
	}

	return ~0;
}

ViewType beginDockNode(HDockNode node)
{
	DockNode* nodeObj = (DockNode*)node;

	beginContainer(nodeObj->rect);

	return dockNodeTabs(nodeObj);
}

void endDockNode()
{
	endContainer();
}

void setViewUserData(HView view, u64 userData)
{
	((View*)view)->userData = userData;
}

u64 getViewUserData(HView view)
{
	return ((View*)view)->userData;
}

void setViewTitle(HView view, const char* title)
{
	auto viewObj = (View*)view;

	viewObj->title = title;
}

const char* getViewTitle(HView view)
{
	auto viewObj = (View*)view;

	return viewObj->title.c_str();
}

ViewType getViewType(HView view)
{
	auto viewObj = (View*)view;

	return viewObj->viewType;
}

void setViewIcon(HView view, HImage icon)
{
	View* viewObj = (View*)view;

	viewObj->icon = icon;
}

f32 getRemainingDockNodeClientHeight(HDockNode node)
{
	DockNode* nodeObj = (DockNode*)node;

	return round((f32)nodeObj->rect.height - (ctx->penPosition.y - nodeObj->rect.y));
}

Rect getViewClientRect(HView view)
{
	auto rc = ((View*)view)->dockNode->rect;
	auto tabHeight = ctx->theme->getElement(WidgetElementId::TabBodyActive).normalState().height;
	rc.y += tabHeight;
	rc.height -= tabHeight;

	return rc;
}

bool beginView(HView view)
{
	View* viewObj = (View*)view;

	if (viewObj->dockNode->getViewIndex(viewObj) != viewObj->dockNode->selectedTabIndex)
	{
		return false;
	}

	auto rc = getViewClientRect(view);
	beginContainer(rc);

	return true;
}

void endView()
{
	endContainer();
}

void printInfo(int level, HWindow wnd, DockNode* node)
{
	std::string spaces(level, '\t');
	printf("%snode: %p nativeWnd: %p parentNode: %p type: %u rc: %0.0f %0.0f %0.0f %0.0f\n", spaces.c_str(), node, wnd, node->parent, node->type, node->rect.x, node->rect.y, node->rect.width, node->rect.height);
	printf("%sviews:\n", spaces.c_str());
	for (auto& v : node->views)
	{
		printf("%s  `%s` %p dockNode: %p icon: %u viewType: %d userData: %p tabRect: %f %f %f %f\n", spaces.c_str(), v->title.c_str(), v, v->dockNode, v->icon, v->viewType, v->userData, v->tabRect.x, v->tabRect.y, v->tabRect.width, v->tabRect.height);
	}

	for (auto& child : node->children)
	{
		printInfo(level + 1, wnd, child);
	}
}

void debugViews()
{
	printf("Debug Views:\n");
	printf("%d windows\n", ctx->dockingState.rootWindowDockNodes.size());

	for (auto& pair : ctx->dockingState.rootWindowDockNodes)
	{
		printInfo(0, pair.first, pair.second);
	}
}

}