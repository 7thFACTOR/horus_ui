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

HDockNode createView(HDockNode targetNode, DockType dock, const char* title, f32 size, ViewType viewType, u64 userData, HImage icon)
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

bool dockView(HView view, HDockNode targetNode, DockType dockType, u32 tabIndex)
{
	auto viewObj = (View*)view;
	auto source = viewObj->dockNode;
	DockNode* target = (DockNode*)targetNode;

	if (source == target)
		return false;

	// if this is the root and its empty
	if (!target->parent && target->children.empty() && target->views.empty() && target->type == DockNode::Type::None)
	{
		target->views.push_back(viewObj);
		target->type = DockNode::Type::ViewTabs;
		target->selectedTabIndex = 0;
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
				source->removeFromParent();
				source->parent = target->parent;
				source->window = target->window;
				// insert it before target
				target->parent->children.insert(iter, source);
				sourceNode = source;
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
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && target->parent) target->parent->computeRect();
		}
		else if (target->parent->type == DockNode::Type::Vertical)
		{
			if (source->views.size() == 1)
			{
				source->removeFromParent();
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

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (target->parent->type == DockNode::Type::Vertical
			|| target->parent->type == DockNode::Type::Horizontal))
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
	case hui::DockType::Right:
		// just insert at the target site
		if (target->parent->type == DockNode::Type::Horizontal)
		{
			auto iter = std::find(target->parent->children.begin(), target->parent->children.end(), target);

			// we will insert after it
			++iter;

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = target->parent;
				source->window = target->window;
				target->parent->children.insert(iter, source);
				sourceNode = source;
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
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && target->parent) target->parent->computeRect();
		}
		else if (target->parent->type == DockNode::Type::Vertical)
		{
			if (source->views.size() == 1)
			{
				source->removeFromParent();
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
			target->children.push_back(newNode);
			target->children.push_back(sourceNode); // insert last

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (target->parent->type == DockNode::Type::Vertical
				|| target->parent->type == DockNode::Type::Horizontal))
		{
			f32 size = target->rect.width * ctx->settings.dockNodeDockingSizeRatio;
			sourceNode->rect.x = target->rect.right() - size;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;
			target->rect.width -= size;
		}
		break;
	case hui::DockType::Top:
		// just insert at the target site
		if (target->parent->type == DockNode::Type::Vertical)
		{
			auto iter = std::find(target->parent->children.begin(), target->parent->children.end(), target);

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = target->parent;
				source->window = target->window;
				// insert it before target
				target->parent->children.insert(iter, source);
				sourceNode = source;
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
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && target->parent) target->parent->computeRect();
		}
		else if (target->parent->type == DockNode::Type::Horizontal)
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
	
			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (target->parent->type == DockNode::Type::Vertical
				|| target->parent->type == DockNode::Type::Horizontal))
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
	case hui::DockType::Bottom:
		// just insert at the target site
		if (target->parent->type == DockNode::Type::Vertical)
		{
			auto iter = std::find(target->parent->children.begin(), target->parent->children.end(), target);

			// we will insert after it
			++iter;

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = target->parent;
				source->window = target->window;
				target->parent->children.insert(iter, source);
				sourceNode = source;
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
				sourceNode = newNode;
			}

			if (ctx->settings.dockNodeProportionalResize && target->parent) target->parent->computeRect();
		}
		else if (target->parent->type == DockNode::Type::Horizontal)
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
			target->children.push_back(newNode);
			target->children.push_back(sourceNode); // insert last

			if (ctx->settings.dockNodeProportionalResize) target->computeRect();
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (!ctx->settings.dockNodeProportionalResize
			&& sourceNode
			&& (target->parent->type == DockNode::Type::Vertical
				|| target->parent->type == DockNode::Type::Horizontal))
		{
			f32 size = target->rect.height * ctx->settings.dockNodeDockingSizeRatio;
			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.bottom() - size;
			sourceNode->rect.width = target->rect.width;
			sourceNode->rect.height = size;
			target->rect.height -= size;
		}
		break;
	case hui::DockType::RootLeft:
		// if we only have a view in the source node, grab the node itself
		if (source->views.size() == 1)
		{
			source->removeFromParent();
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
	case hui::DockType::RootRight:
		// if we only have a view in the source node, grab the node itself
		if (source->views.size() == 1)
		{
			source->removeFromParent();
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
	case hui::DockType::RootTop:
		// if we only have a view in the source node, grab the node itself
		if (source->views.size() == 1)
		{
			source->removeFromParent();
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
	case hui::DockType::RootBottom:
		// if we only have a view in the source node, grab the node itself
		if (source->views.size() == 1)
		{
			source->removeFromParent();
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
	case hui::DockType::AsTab:
		{
			auto iter = std::next(target->views.begin(), tabIndex);
			target->views.insert(iter, viewObj);
			target->type = DockNode::Type::ViewTabs;
			target->selectedTabIndex = 0;

			// if we only have a view in the source node, delete the node
			if (source->views.size() == 1)
			{
				source->removeFromParent();
				delete source;
			}
			else
			{
				// remove from the source node
				source->removeView(viewObj);
			}
	}
		break;
	default:
		break;
	}

	return true;
}

}