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

void setDockNodeRect(HDockNode node, const Rect& rect)
{
	DockNode* nodeObj = (DockNode*)node;

	nodeObj->rect = rect;
	nodeObj->computeRect();
}

DragDockNodeInfo findDockNodeDragInfoAtMousePos(HWindow window, const Point& mousePos)
{
	auto node = ctx->dockingState.rootWindowDockNodes[window];
	DragDockNodeInfo info;
	
	if (!node) return info;

	info.node = node->findResizeNode(mousePos);

	if (info.node)
	{
		DockNode* nodeObj = (DockNode*)info.node;
		
		if (nodeObj->parent)
		{
			switch (nodeObj->parent->type)
			{
			case DockNode::Type::Horizontal: info.dragSide = DragDockNodeInfo::DragSide::Right; break;
			case DockNode::Type::Vertical: info.dragSide = DragDockNodeInfo::DragSide::Bottom; break;
			}
		}
	}

	return info;
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
		target->type = DockNode::Type::ViewTabs;
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
			//newNode->rect = targetParent->rect;
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
		if (targetParent->type == DockNode::Type::Horizontal || targetParent->type == DockNode::Type::ViewTabs)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);
			
			if (newTarget) target = newTarget;

			// if there is just one view in the source node, move the node and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.begin(), source);
				}

				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				if (source)	source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				newNode->rect = viewRect;

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					targetParent->children.insert(iter, newNode);
				}
				else
				{
					targetParent->children.insert(targetParent->children.begin(), newNode);
				}

				sourceNode = newNode;
			}
			
			targetParent->type = DockNode::Type::Horizontal;
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
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->type = DockNode::Type::ViewTabs;
				sourceNode->window = targetParent->window;
				sourceNode->rect = viewRect;
			}

			if (targetParent->type == DockNode::Type::Horizontal)
			{
				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.begin(), source);
				}
			}
			else
			{
				// relocate target's content into new node
				DockNode* newTargetNode = new DockNode();
				*newTargetNode = *target;
				newTargetNode->parent = target;
				sourceNode->parent = target;
				for (auto& c : newTargetNode->children) c->parent = newTargetNode;
				for (auto& v : newTargetNode->views) v->dockNode = newTargetNode;
				target->views.clear();
				target->children.clear();
				target->children.push_back(sourceNode);
				target->children.push_back(newTargetNode);
				target->type = DockNode::Type::Horizontal;
			}
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
		if (targetParent->type == DockNode::Type::Horizontal || targetParent->type == DockNode::Type::ViewTabs)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);

			if (newTarget) target = newTarget;

			// if there is just one view in the source node, move the node and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					// we will insert after it
					if (iter != targetParent->children.end())
						++iter;

					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.end(), source);
				}

				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				if (source) source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				newNode->rect = viewRect;

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					// we will insert after it
					if (iter != targetParent->children.end())
						++iter;

					targetParent->children.insert(iter, newNode);
				}
				else
				{
					targetParent->children.insert(targetParent->children.end(), newNode);
				}
				
				sourceNode = newNode;
			}

			targetParent->type = DockNode::Type::Horizontal;
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
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->window = targetParent->window;
				sourceNode->type = DockNode::Type::ViewTabs;
				sourceNode->rect = viewRect;
			}

			if (targetParent->type == DockNode::Type::Horizontal)
			{
				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					// we will insert after it
					if (iter != targetParent->children.end())
						++iter;
	
					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.end(), source);
				}
			}
			else
			{
				// relocate target's content into new node
				DockNode* newNode = new DockNode();
				*newNode = *target;
				newNode->parent = target;
				sourceNode->parent = target;
				for (auto& c : newNode->children) c->parent = newNode;
				for (auto& v : newNode->views) v->dockNode = newNode;
				target->views.clear();
				target->children.clear();
				target->children.push_back(newNode);
				target->children.push_back(sourceNode); // insert last
				target->type = DockNode::Type::Horizontal;
			}
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
		if (targetParent->type == DockNode::Type::Vertical || targetParent->type == DockNode::Type::ViewTabs)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);

			if (newTarget) target = newTarget;

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.begin(), source);
				}

				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				if (source) source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				newNode->rect = viewRect;

				if (targetParent != target)
				{
					// treat docking to root node
					if (targetParent != target)
					{
						auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);
						targetParent->children.insert(iter, newNode);
					}
					else
					{
						targetParent->children.insert(targetParent->children.begin(), newNode);
					}
				}
				else
				{
					targetParent->children.insert(targetParent->children.begin(), newNode);
				}

				sourceNode = newNode;
			}

			targetParent->type = DockNode::Type::Vertical;
		}
		else if (targetParent->type == DockNode::Type::Horizontal)
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
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->window = targetParent->window;
				sourceNode->type = DockNode::Type::ViewTabs;
				sourceNode->rect = viewRect;
			}

			if (targetParent->type == DockNode::Type::Vertical)
			{
				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);
					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.begin(), source);
				}
			}
			else
			{
				// relocate target's content into new node
				DockNode* newNode = new DockNode();
				*newNode = *target;
				newNode->parent = target;
				sourceNode->parent = target;
				for (auto& c : newNode->children) c->parent = newNode;
				for (auto& v : newNode->views) v->dockNode = newNode;
				target->views.clear();
				target->children.clear();
				target->children.push_back(sourceNode);
				target->children.push_back(newNode);
				target->type = DockNode::Type::Vertical;
			}
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
		if (targetParent->type == DockNode::Type::Vertical || targetParent->type == DockNode::Type::ViewTabs)
		{
			// if there is no children nodes but has views, relocate to new node
			auto newTarget = checkRelocateViewsOfNode(targetParent);

			if (newTarget) target = newTarget;

			// if there is just one view in the source node, move the node also and remove from current parent
			if (source && source->views.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->window = target->window;

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					// we will insert after it
					if (iter != targetParent->children.end())
						++iter;

					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.end(), source);
				}

				sourceNode = source;
			}
			else
			// we create a new node to hold the view
			{
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				if (source) source->removeView(viewObj);
				DockNode* newNode = new DockNode();
				newNode->views.push_back(viewObj);
				viewObj->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::ViewTabs;
				newNode->window = target->window;
				newNode->rect = viewRect;

				auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					// we will insert after it
					if (iter != targetParent->children.end())
						++iter;

					targetParent->children.insert(iter, newNode);
				}
				else
				{
					targetParent->children.insert(targetParent->children.end(), newNode);
				}

				sourceNode = newNode;
			}

			targetParent->type = DockNode::Type::Vertical;
		}
		else if (targetParent->type == DockNode::Type::Horizontal)
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
				auto viewRect = viewObj->dockNode ? viewObj->dockNode->rect : Rect();
				// remove the view from parent node
				if (source) source->removeView(viewObj);
				// create new node for the view
				sourceNode = new DockNode();
				sourceNode->views.push_back(viewObj);
				viewObj->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->window = targetParent->window;
				sourceNode->type = DockNode::Type::ViewTabs;
				sourceNode->rect = viewRect;
			}

			if (targetParent->type == DockNode::Type::Vertical)
			{
				// treat docking to root node
				if (targetParent != target)
				{
					auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

					// we will insert after it
					if (iter != targetParent->children.end())
						++iter;

					targetParent->children.insert(iter, source);
				}
				else
				{
					targetParent->children.insert(targetParent->children.end(), source);
				}
			}
			else
			{
				// relocate target's content into new node
				DockNode* newNode = new DockNode();
				*newNode = *target;
				newNode->parent = target;
				sourceNode->parent = target;
				for (auto& c : newNode->children) c->parent = newNode;
				for (auto& v : newNode->views) v->dockNode = newNode;
				target->views.clear();
				target->children.clear();
				target->children.push_back(newNode);
				target->children.push_back(sourceNode); // insert last
				target->type = DockNode::Type::Vertical;
			}
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

		viewObj->dockNode = target;

		break;
	}
	default:
		break;
	}

	if (ctx->settings.dockNodeProportionalResize)
	{
		auto node = (DockNode*)getRootDockNode(target->window);
		node->checkRedundancy();
		node->computeRect();
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

	//beginContainer(nodeObj->rect);

	//auto ret = dockNodeTabs(nodeObj);
	auto info = hui::findDockNodeDragInfoAtMousePos(hui::getWindow(), hui::getMousePosition());
	
	if (info.node == node)
		hui::setLineColor(hui::Color::red);
	else
		hui::setLineColor(hui::Color::white);

	hui::drawRectangle(nodeObj->rect);
	hui::setFont(hui::getFont("normal-bold"));

	if (nodeObj->views.size())
	{
		std::string viewNames;

		for (auto& v : nodeObj->views) viewNames += v->title + ";";

		hui::drawTextInBox(viewNames.c_str(), nodeObj->rect, hui::HAlignType::Center, hui::VAlignType::Center);
	}

	for (auto& c : nodeObj->children)
	{
		beginDockNode(c);
		endDockNode();
	}

	return 0;
}

void endDockNode()
{
	//endContainer();
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

std::string enumTypeToStr(DockNode::Type type)
{
	switch (type)
	{
	case hui::DockNode::Type::None:
		return "None";
		break;
	case hui::DockNode::Type::ViewTabs:
		return "ViewTabs";
		break;
	case hui::DockNode::Type::Vertical:
		return "Vertical";
		break;
	case hui::DockNode::Type::Horizontal:
		return "Horizontal";
		break;
	default:
		return "";
		break;
	}
}

void printInfo(int level, HWindow wnd, DockNode* node)
{
	std::string spaces(level, '\t');
	printf("%snode: %p wnd: %p parent: %p type: %s rc: %0.0f %0.0f %0.0f %0.0f\n", spaces.c_str(), node, wnd, node->parent, enumTypeToStr(node->type).c_str(), node->rect.x, node->rect.y, node->rect.width, node->rect.height);

	for (auto& v : node->views)
	{
		printf("%s `%s` %p node: %p icon: %u viewType: %d usrData: %p tabRect: %f %f %f %f\n", spaces.c_str(), v->title.c_str(), v, v->dockNode, v->icon, v->viewType, v->userData, v->tabRect.x, v->tabRect.y, v->tabRect.width, v->tabRect.height);
	}

	for (auto& child : node->children)
	{
		printInfo(level + 1, wnd, child);
	}
}

void debugViews()
{
	printf("------------------------------------------------------------------------------------------------------------\n");
	printf("Debug Views:\n\n");
	printf("%d windows\n", ctx->dockingState.rootWindowDockNodes.size());

	for (auto& pair : ctx->dockingState.rootWindowDockNodes)
	{
		printInfo(0, pair.first, pair.second);
	}
}

}