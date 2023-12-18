#include "horus.h"
#include "docking.h"
#include "dock_node.h"
#include "docking_system.h"
#include <cstring>
#include <algorithm>
#include <math.h>
#include "context.h"

namespace hui
{
HOsWindow createOsWindow(const std::string& title, OsWindowFlags flags, const Rect& rect)
{
	auto wnd = ctx->providers->input->createWindow(title.c_str(), flags, rect);

	if (!ctx->renderer)
	{
		ctx->initializeGraphics();
	}

	ctx->osWindows.push_back(wnd);

	return wnd;
}

void destroyOsWindow(HOsWindow wnd)
{
	HORUS_INPUT->destroyWindow(wnd);
	auto iter = std::find(ctx->osWindows.begin(), ctx->osWindows.end(), wnd);

	if (iter != ctx->osWindows.end())
	{
		ctx->osWindows.erase(iter);
	}
}

DockNode* createRootDockNode(HOsWindow osWindow)
{
	auto size = HORUS_INPUT->getWindowClientSize(osWindow);
	Rect rect = { 0, 0, size.x, size.y };
	auto dockNode = new DockNode();

	dockNode->type = DockNode::Type::None;
	dockNode->rect.set(0, 0, rect.width, rect.height);
	dockNode->osWindow = osWindow;
	ctx->dockingState.rootOsWindowDockNodes.insert(std::make_pair(osWindow, dockNode));

	return dockNode;
}

DockNode* getRootDockNode(HOsWindow window)
{
	return ctx->dockingState.rootOsWindowDockNodes[window];
}

DragDockNodeInfo findDockNodeDragInfoAtMousePos(HOsWindow window, const Point& mousePos)
{
	auto node = ctx->dockingState.rootOsWindowDockNodes[window];
	DragDockNodeInfo info;
	
	if (!node) return info;

	info.node = node->findDockNode(mousePos);

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

void deleteRootDockNode(HOsWindow window)
{
	auto node = ctx->dockingState.rootOsWindowDockNodes[window];

	if (node)
	{
		ctx->dockingState.rootOsWindowDockNodes.erase(window);
		delete node;
	}
}

Window* createWindow(const std::string& id, DockNode* targetNode, DockType dockType, const std::string& title, Rect* initialRect, HOsWindow osWnd)
{
	auto targetNodePtr = (DockNode*)targetNode;
	auto newWnd = new Window();
	Rect defaultRect = { 10, 10, 500, 300 };

	newWnd->title = title;

	if (!targetNode)
	{
		if (!osWnd)
		{
			osWnd = createOsWindow(title, OsWindowFlags::Resizable, initialRect ? *initialRect : defaultRect);
		}
		
		newWnd->dockNode = createRootDockNode(osWnd);
	}

	ctx->dockingState.windows[id] = newWnd;
	ctx->osWindows.push_back(osWnd);

	if (targetNode)
		dockWindow(newWnd, targetNode, dockType, 0);

	return newWnd;
}

void deleteWindow(Window* wnd)
{
	DockNode* node = wnd->dockNode;

	node->removeWindow(wnd);

	if (!node->parent)
	{
		destroyOsWindow(node->osWindow);
		ctx->dockingState.rootOsWindowDockNodes.erase(node->osWindow);
		delete node;
	}
}

bool dockWindow(Window* wnd, DockNode* targetNode, DockType dockType, u32 tabIndex)
{
	auto source = wnd->dockNode;
	DockNode* target = (DockNode*)targetNode;
	auto targetIsRoot = !target->parent;

	if (source == target)
		return false;

	// if this is the root and its empty of any children and windows
	if (targetIsRoot && target->children.empty() && target->windows.empty() && target->type == DockNode::Type::None)
	{
		target->windows.push_back(wnd);
		wnd->dockNode = target;
		target->type = DockNode::Type::Tabs;
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

	auto checkRelocateWindowsOfNode = [](DockNode* targetParent)
	{
		DockNode* newNode = nullptr;

		// if there are windows but no children nodes, create one
		if (!targetParent->windows.empty() && targetParent->children.empty())
		{
			newNode = new DockNode();

			newNode->parent = targetParent;
			newNode->selectedTabIndex = targetParent->selectedTabIndex;
			newNode->type = targetParent->type;
			newNode->windows = targetParent->windows;
			newNode->osWindow = targetParent->osWindow;
			for (auto& w : newNode->windows) { w->dockNode = newNode; }
			targetParent->windows.clear();
			targetParent->children.push_back(newNode);

			return newNode;
		}

		return newNode;
	};

	switch (dockType)
	{
	case hui::DockType::Left:
	{	// just insert at the target site
		if (targetParent->type == DockNode::Type::Horizontal || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkRelocateWindowsOfNode(targetParent);
			
			if (newTarget) target = newTarget;

			// if there is just one window in the source node, move the node and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->osWindow = target->osWindow;

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
			// we create a new node to hold the window
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (source)	source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->osWindow = target->osWindow;
				newNode->rect = wndRect;

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
			// if source has one window, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				// remove the window from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the window
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->osWindow = targetParent->osWindow;
				sourceNode->rect = wndRect;
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
				for (auto& w : newTargetNode->windows) w->dockNode = newTargetNode;
				target->windows.clear();
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
		if (targetParent->type == DockNode::Type::Horizontal || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkRelocateWindowsOfNode(targetParent);

			if (newTarget) target = newTarget;

			// if there is just one window in the source node, move the node and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->osWindow = target->osWindow;

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
			// we create a new node to hold the window
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (source) source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->osWindow = target->osWindow;
				newNode->rect = wndRect;

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
			// if source has one window, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				// remove the window from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the window
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->osWindow = targetParent->osWindow;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->rect = wndRect;
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
				for (auto& w : newNode->windows) w->dockNode = newNode;
				target->windows.clear();
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
		if (targetParent->type == DockNode::Type::Vertical || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkRelocateWindowsOfNode(targetParent);

			if (newTarget) target = newTarget;

			// if there is just one window in the source node, move the node also and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->osWindow = target->osWindow;

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
			// we create a new node to hold the window
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (source) source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->osWindow = target->osWindow;
				newNode->rect = wndRect;

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
			// if source has one window, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				// remove the window from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the window
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->osWindow = targetParent->osWindow;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->rect = wndRect;
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
				for (auto& w : newNode->windows) w->dockNode = newNode;
				target->windows.clear();
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
		if (targetParent->type == DockNode::Type::Vertical || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkRelocateWindowsOfNode(targetParent);

			if (newTarget) target = newTarget;

			// if there is just one window in the source node, move the node also and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				source->osWindow = target->osWindow;

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
			// we create a new node to hold the window
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (source) source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->osWindow = target->osWindow;
				newNode->rect = wndRect;

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
			// if source has one window, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source->removeFromParent();
				source->parent = targetParent;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				// remove the window from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the window
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->osWindow = targetParent->osWindow;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->rect = wndRect;
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
				for (auto& w : newNode->windows) w->dockNode = newNode;
				target->windows.clear();
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
		auto iter = std::next(target->windows.begin(), tabIndex);
		target->windows.insert(iter, wnd);
		target->type = DockNode::Type::Tabs;
		target->selectedTabIndex = tabIndex;

		// if we only have a window in the source node, delete the node
		if (source && source->windows.size() == 1)
		{
			source->removeFromParent();
			delete source;
		}
		else
		{
			// remove from the source node
			if (source) source->removeWindow(wnd);
		}

		wnd->dockNode = target;

		break;
	}
	default:
		break;
	}

	if (ctx->settings.dockNodeProportionalResize)
	{
		auto node = (DockNode*)getRootDockNode(target->osWindow);
		node->checkRedundancy();
		node->computeRect();
	}

	return true;
}

void dockNodeTabs(DockNode* node)
{
	if (ctx->layoutStack.back().width <= (node->windows.size() * (ctx->paneGroupState.tabWidth + ctx->paneGroupState.sideSpacing)) * ctx->globalScale)
	{
		ctx->paneGroupState.forceTabWidth = ctx->layoutStack.back().width / (f32)node->windows.size();
		ctx->paneGroupState.forceSqueezeTabs = true;
	}
	else
	{
		ctx->paneGroupState.forceSqueezeTabs = false;
	}

	u32 closeTabIndex = ~0;

	ctx->dockingState.drawingWindowTabs = true;
	beginTabGroup(node->selectedTabIndex);

	for (size_t i = 0; i < node->windows.size(); i++)
	{
		hui::tab(node->windows[i]->title.c_str(), 0/*node->windows[i]->icon*/);
		node->windows[i]->tabRect = ctx->widget.rect;

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

	ctx->dockingState.drawingWindowTabs = false;

	if (closeTabIndex != ~0)
	{
		node->removeWindow(node->windows[closeTabIndex]);

		if (selectedIndex >= node->windows.size())
			selectedIndex = node->windows.size() - 1;
	}

	if (!node->windows.empty())
	{
		if (node->selectedTabIndex != selectedIndex)
		{
			node->selectedTabIndex = selectedIndex;
			hui::forceRepaint();
		}
	}
}

void beginDockNode(DockNode* node)
{
	//beginContainer(node->rect);

	//auto ret = dockNodeTabs(node);
	auto info = hui::findDockNodeDragInfoAtMousePos(node->osWindow, hui::getMousePosition());
	
	if (info.node == node)
		hui::setLineColor(hui::Color::red);
	else
		hui::setLineColor(hui::Color::white);

	hui::drawRectangle(node->rect);
	hui::setFont(hui::getFont("normal-bold"));

	if (node->windows.size())
	{
		std::string windowNames;

		for (auto& w : node->windows) windowNames += w->title + ";";

		hui::drawTextInBox(windowNames.c_str(), node->rect, hui::HAlignType::Center, hui::VAlignType::Center);
	}

	for (auto& c : node->children)
	{
		beginDockNode(c);
		//endDockNode();
	}
}

void endDockNode()
{
	//endContainer();
}

f32 getRemainingDockNodeClientHeight(HDockNode node)
{
	DockNode* nodeObj = (DockNode*)node;

	return round((f32)nodeObj->rect.height - (ctx->penPosition.y - nodeObj->rect.y));
}

Rect getWindowClientRect(Window* window)
{
	auto rc = window->dockNode->rect;
	auto tabHeight = ctx->theme->getElement(WidgetElementId::TabBodyActive).normalState().height;
	rc.y += tabHeight;
	rc.height -= tabHeight;

	return rc;
}

std::string enumTypeToStr(DockNode::Type type)
{
	switch (type)
	{
	case hui::DockNode::Type::None:
		return "None";
		break;
	case hui::DockNode::Type::Tabs:
		return "Tabs";
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

void printInfo(int level, DockNode* node)
{
	std::string spaces(level, '\t');
	printf("%snode: %p osWnd: %p parent: %p type: %s rc: %0.0f %0.0f %0.0f %0.0f\n", spaces.c_str(), node, node->osWindow, node->parent, enumTypeToStr(node->type).c_str(), node->rect.x, node->rect.y, node->rect.width, node->rect.height);

	for (auto& w : node->windows)
	{
		printf("%s `%s` %p node: %p windowType: %d usrData: %p tabRect: %f %f %f %f\n", spaces.c_str(), w->title.c_str(), w, w->dockNode, w->tabRect.x, w->tabRect.y, w->tabRect.width, w->tabRect.height);
	}

	for (auto& child : node->children)
	{
		printInfo(level + 1, child);
	}
}

void debugwindows()
{
	printf("------------------------------------------------------------------------------------------------------------\n");
	printf("Debug windows:\n\n");
	printf("%d windows\n", ctx->dockingState.rootOsWindowDockNodes.size());

	for (auto& pair : ctx->dockingState.rootOsWindowDockNodes)
	{
		printInfo(0, pair.second);
	}
}

}