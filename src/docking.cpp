#include <cstring>
#include <assert.h>
#include <algorithm>
#include <math.h>
#include "horus.h"
#include "docking.h"
#include "context.h"
#include "theme.h"

namespace hui
{
DockNode::DockNode()
{
	if (ctx)
	{
		id = ctx->dockingState.nextDockNodeId++;
	}
}

void DockNode::copyFrom(DockNode* other)
{
	parent = other->parent;
	children = other->children;
	windows = other->windows;
	nativeWindow = other->nativeWindow;
	type = other->type;
	minSize = other->minSize;
	rect = other->rect;
	selectedTabIndex = other->selectedTabIndex;
}

void DockNode::adoptChildren()
{
	for (auto& c : children)
	{
		c->parent = this;
	}
}

void DockNode::adoptWindows()
{
	for (auto& w : windows)
	{
		w->dockNode = this;
	}
}

bool DockNode::hasSingleWindow() const
{
	return windows.size() <= 1;
}

void DockNode::removeWindowsAndDeleteChildrenRecursive()
{
	for (auto& wnd : windows)
	{
		ctx->dockingState.windowsToDelete.insert(wnd);
	}

	windows.clear();

	for (auto& child : children)
	{
		child->removeWindowsAndDeleteChildrenRecursive();
		ctx->dockingState.dockNodesToDelete.insert(child);
	}

	children.clear();
}

DockNode* DockNode::removeFromParent()
{
	if (parent)
	{
		auto iter = std::find(parent->children.begin(), parent->children.end(), this);
		
		if (iter != parent->children.end())
		{
			auto prev = parent->findPrevSiblingOf(this);
			DockNode* sibling = nullptr;

			if (prev == parent->children.rend())
			{
				auto next = parent->findNextSiblingOf(this);

				if (next != parent->children.end())
					sibling = *next;
			}
			else
			{
				sibling = *prev;
			}

			if (sibling)
			{
				// donate size to sibling
				if (parent->type == DockNode::Type::Vertical)
					sibling->rect.height += rect.height + ctx->settings.dockNodeSpacing;

				if (parent->type == DockNode::Type::Horizontal)
					sibling->rect.width += rect.width + ctx->settings.dockNodeSpacing;
			}

			parent->children.erase(iter);
			parent->computeRect();
			parent = nullptr;
		}

		return this;
	}
	else
	{
		if (createdByUndocking)
		{
			// this is a root node and removing it we must destroy the window too
			ctx->dockingState.nativeWindowsToDelete.insert(nativeWindow);

			// we need to remove this now, it will interfere with redudancy checks
			auto iter = std::find(ctx->nativeWindows.begin(), ctx->nativeWindows.end(), nativeWindow);

			assert(iter != ctx->nativeWindows.end());

			if (iter != ctx->nativeWindows.end()) ctx->nativeWindows.erase(iter);

			nativeWindow = nullptr;
			
			return this;
		}
		else
		{
			// we need to keep this node as the root, as a dock site
			// and create a copy of it and return it
			DockNode* copy = new DockNode();

			copy->copyFrom(this);
			copy->adoptChildren();
			copy->adoptWindows();

			windows.clear();
			children.clear();
			type = Type::Tabs;

			return copy;
		}
	}
}

void DockNode::removeWindow(Window* window)
{
	auto iter = std::find(windows.begin(), windows.end(), window);

	if (iter != windows.end())
	{
		auto idx = std::distance(windows.begin(), iter);

		(*iter)->dockNode = nullptr;
		windows.erase(iter);

		// make sure we leave a proper selected index for tabs
		if (selectedTabIndex >= windows.size() && !windows.empty())
		{
			selectedTabIndex = windows.size() - 1;
		}
 	}
}

void DockNode::gatherWindowTabsNodes(std::vector<DockNode*>& outNodes)
{
	if (type == Type::Tabs)
	{
		outNodes.push_back(this);
		return;
	}

	for (auto& c : children)
	{
		if (!c->hasSingleWindow())
			continue;

		if (c->type == DockNode::Type::Tabs)
		{
			outNodes.push_back(c);
		}
		else
		{
			c->gatherWindowTabsNodes(outNodes);
		}
	}
}

void DockNode::computeRect()
{
	if (!parent)
	{
		auto size = HORUS_INPUT->getWindowSize(nativeWindow);
		rect = { 0, 0, size.x, size.y };

	}

	switch (type)
	{
	case DockNode::Type::None:
	case DockNode::Type::Tabs:
	{
		auto tabGroupHeight = ctx->theme ? ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState().height : 0;

		for (auto& wnd : windows)
		{
			wnd->clientRect = {
				rect.x, rect.y + tabGroupHeight, rect.width, rect.height - tabGroupHeight
			};
		}
		break;
	}
	case DockNode::Type::Vertical:
	{
		auto childCount = children.size();
		f32 availableSpace = rect.height - ctx->settings.dockNodeSpacing * (f32)(childCount - 1);
		f32 averageSpace = availableSpace / (f32)childCount;
		f32 totalSpace = 0;

		for (auto& child : children)
		{
			if (child->rect.height <= 0.0f)
			{
				child->rect.height = averageSpace;
			}

			totalSpace += child->rect.height;
		}

		f32 currentY = rect.y;

		for (auto& child : children)
		{
			f32 height = child->rect.height / totalSpace * availableSpace;

			child->rect.y = currentY;
			child->rect.x = rect.x;
			child->rect.width = rect.width;
			child->rect.height = height;

			// recursive, could be done with a vector and a while loop though
			child->computeRect();

			currentY += height;
			currentY += ctx->settings.dockNodeSpacing;
		}
	}
		break;
	case DockNode::Type::Horizontal:
	{
		auto childCount = children.size();
		f32 availableSpace = rect.width - ctx->settings.dockNodeSpacing * (f32)(childCount - 1);
		availableSpace = (i32)availableSpace;
		f32 averageSpace = availableSpace / (f32)childCount;
		f32 totalSpace = 0;

		averageSpace = (i32)averageSpace;

		for (auto& child : children)
		{
			if (child->rect.width <= 0.0f)
			{
				child->rect.width = averageSpace;
			}

			totalSpace += child->rect.width;
		}

		f32 currentX = rect.x;

		totalSpace = totalSpace;

		for (auto& child : children)
		{
			f32 width = child->rect.width / totalSpace * availableSpace;

			currentX = (i32)currentX;
			width = (i32)width;
			child->rect.x = currentX;
			child->rect.y = rect.y;
			child->rect.height = rect.height;
			child->rect.width = width;

			// recursive, could be done with a vector and a while loop though
			child->computeRect();

			currentX += width;
			currentX += ctx->settings.dockNodeSpacing;
		}
	}
		break;
	default:
		break;
	}
}

void DockNode::computeMinSize()
{
	minSize.set(ctx->settings.dockNodeMinSize, ctx->settings.dockNodeMinSize);

	for (auto& child : children)
	{
		child->computeMinSize();
	};

	switch (type)
	{
	case DockNode::Type::None:
		break;
	case DockNode::Type::Tabs:
		break;
	case DockNode::Type::Vertical:
	{
		f32 total = 0;

		for (auto& child : children)
		{
			total += child->minSize.y;
		};

		if (total > minSize.y) minSize.y = total;

		break;
	}
	case DockNode::Type::Horizontal:
	{
		f32 total = 0;

		for (auto& child : children)
		{
			total += child->minSize.x;
		};

		if (total > minSize.x) minSize.x = total;

		break;
	}
	break;
	default:
		break;
	}
}

bool DockNode::checkRedundancy()
{
	// first do leafs for redundant nesting of dock nodes
	// collapse starting at the leaf nodes
	// use a copy of the children vector since it might get modified
	auto copyOfChildren = children;

	for (auto& c : copyOfChildren)
	{
		c->checkRedundancy();
	}

	// if we have just 1 child, delete it, move its contents to us
	if (children.size() == 1)
	{
		auto child = children[0];
		
		children = child->children;
		windows = child->windows;

		adoptChildren();
		adoptWindows();
		selectedTabIndex = child->selectedTabIndex;
		type = child->type;

		ctx->dockingState.dockNodesToDelete.insert(child);
	}

	if (parent)
	{
		bool deleteThis = false;

		// if same type as parent, merge its nodes and windows into parent
		if (parent->type == type)
		{
			auto iterPosThis = std::find(parent->children.begin(), parent->children.end(), this);

			parent->children.insert(iterPosThis, children.begin(), children.end());
			
			// find it again, remove it, leaving children in the parent node
			iterPosThis = std::find(parent->children.begin(), parent->children.end(), this);

			assert(iterPosThis != parent->children.end());

			parent->children.erase(iterPosThis);
			parent->adoptChildren();
			
			deleteThis = true;
		}

		if (deleteThis)
		{
			ctx->dockingState.dockNodesToDelete.insert(this);
		}

		return true;
	}

	return false;
}

void DockNode::debug(i32 level)
{
	std::string tabs(level, '\t');
	std::string name = "";

	switch (type)
	{
	case Type::None: name = "None"; break;
	case Type::Vertical: name = "Vertical"; break;
	case Type::Horizontal: name = "Horizontal"; break;
	case Type::Tabs: name = "Tabs"; break;
	default: break;
	}

	printf("%s%s rect(%d,%d,%d,%d) tabIdx:%d nativeWnd:%p\n", tabs.c_str(), name.c_str(), (i32)rect.x, (i32)rect.y, (i32)rect.width, (i32)rect.height, (u32)selectedTabIndex, nativeWindow);

	if (!windows.empty())
	{
		printf("%s\tViews:\n", tabs.c_str());
		for (auto& w : windows)
			printf("%s\t\t%s\n", tabs.c_str(), w->title.c_str());
	}

	for (auto& c : children) c->debug(level + 1);
}

DockNode* DockNode::findResizeDockNode(const Point& pt)
{
	if (type != Type::None)
	{
		if (!rect.contains(pt))
		{
			return nullptr;
		}

		switch (type)
		{
		case Type::Horizontal:
			for (auto& child : children)
			{
				if (child != children.back())
				{
					if (pt.x >= child->rect.right() + ctx->settings.dockNodeSpacing/2 - ctx->settings.dockNodeResizeSplitterHitSize/2
						&& pt.x <= child->rect.right() + ctx->settings.dockNodeSpacing / 2 + ctx->settings.dockNodeResizeSplitterHitSize/2)
					{
						return child;
					}
				}
			}
			break;
		case Type::Vertical:
			for (auto& child : children)
			{
				if (child != children.back())
				{
					if (pt.y >= child->rect.bottom() + ctx->settings.dockNodeSpacing / 2 - ctx->settings.dockNodeResizeSplitterHitSize / 2
						&& pt.y <= child->rect.bottom() + ctx->settings.dockNodeSpacing / 2 + ctx->settings.dockNodeResizeSplitterHitSize / 2)
					{
						return child;
					}
				}
			}
			break;
		default:
			break;
		}

		for (auto& child : children)
		{
			auto foundChild = child->findResizeDockNode(pt);

			if (foundChild)
			{
				return foundChild;
			}
		}
	}

	return nullptr;
}

DockNode* DockNode::findTargetDockNode(const Point& pt)
{
	if ((type == Type::None || type == Type::Tabs))
	{
		Rect rectWithDockSpacing = rect;
		
		rectWithDockSpacing.width += ctx->settings.dockNodeSpacing;
		rectWithDockSpacing.height += ctx->settings.dockNodeSpacing;

		if (rectWithDockSpacing.contains(pt))
		{
			return this;
		}
	}
	else
	{
		for (auto& child : children)
		{
			auto foundChild = child->findTargetDockNode(pt);

			if (foundChild)
			{
				return foundChild;
			}
		}
	}

	return nullptr;
}

size_t DockNode::getWindowIndex(Window* window)
{
	auto iter = std::find(windows.begin(), windows.end(), window);
	
	if (iter == windows.end()) return -1;

	return std::distance(windows.begin(), iter);
}

DockNode* DockNode::findDockNode(const Point& pt)
{
	if (rect.contains(pt)) return this;

	for (auto& child : children)
	{
		auto node = child->findDockNode(pt);
		
		if (node)
			return node;
	}

	return nullptr;
}

std::vector<DockNode*>::iterator DockNode::findNextSiblingOf(DockNode* node)
{
	auto iter = std::find(children.begin(), children.end(), node);

	if (iter != children.end())
		++iter;

	return iter;
}

std::vector<DockNode*>::reverse_iterator DockNode::findPrevSiblingOf(DockNode* node)
{
	auto iter = std::find(children.rbegin(), children.rend(), node);

	if (iter != children.rend())
		++iter;

	return iter;
}

std::vector<DockNode*>::iterator DockNode::getIteratorOf(DockNode* node)
{
	auto iter = std::find(children.begin(), children.end(), node);

	return iter;
}

std::vector<DockNode*>::reverse_iterator DockNode::getReverseIteratorOf(DockNode* node)
{
	auto iter = std::find(children.rbegin(), children.rend(), node);

	return iter;
}

void DockNode::insertTabSpaceAt(const Point& mousePos, f32 spaceWidth)
{
	for (auto i = 0; i < windows.size(); i++)
	{
		if (windows[i]->dockingNow)
			continue;

		if (windows[i]->tabRect.contains(mousePos))
		{
			dockingTabSpaceIndex = i;

			if (windows[i]->tabRect.x + windows[i]->tabRect.width / 2 < mousePos.x)
			{
				dockingTabSpaceIndex = i + 1;
			}

			dockingTabSpaceWidth = spaceWidth;
			return;
		}
	}
}

void DockNode::moveWindowTabAt(const Point& mousePos, Window* window)
{
	auto wndIndex = getWindowIndex(window);

	for (auto i = 0; i < windows.size(); i++)
	{
		if (windows[i]->dockingNow)
			continue;

		// swap the tab with the overlapped one
		if (windows[i]->tabRect.x + windows[i]->tabRect.width / 2.0f < mousePos.x
			&& windows[i]->tabRect.right() > mousePos.x)
		{
			dockingTabSpaceIndex = i;

			auto tmp = windows[wndIndex];

			windows[wndIndex] = windows[dockingTabSpaceIndex];
			windows[dockingTabSpaceIndex] = tmp;
			selectedTabIndex = ~0;

			return;
		}
	}
}

void DockNode::removeTabSpace()
{
	dockingTabSpaceWidth = 0;
	dockingTabSpaceIndex = ~0;
}

bool saveDockingState(const char* filename)
{
	//TODO
	return true;
}

u8* saveDockingStateToMemory(size_t& outStateInfoSize)
{
	//TODO
	return 0;
}

bool loadDockingState(const char* filename)
{
	//TODO
	return false;
}

bool loadDockingStateFromMemory(const u8* stateInfo, size_t stateInfoSize)
{
	//TODO
	return true;
}

HNativeWindow createNativeWindow(const std::string& title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect)
{
	auto wnd = ctx->providers->input->createWindow(title.c_str(), flags, state, rect);

	if (!ctx->renderer)
	{
		ctx->initializeGraphics();
	}

	ctx->nativeWindows.push_back(wnd);

	return wnd;
}

void destroyNativeWindow(HNativeWindow nativeWnd)
{
	auto dockNode =	ctx->dockingState.rootNativeWindowDockNodes[nativeWnd];

	if (dockNode)
		ctx->dockingState.dockNodesToDelete.insert(dockNode);
	
	if (nativeWnd)
		ctx->dockingState.nativeWindowsToDelete.insert(nativeWnd);
}

DockNode* createNativeWindowRootDockNode(HNativeWindow nativeWindow)
{
	auto size = HORUS_INPUT->getWindowSize(nativeWindow);
	Rect rect = { 0, 0, size.x, size.y };
	auto dockNode = new DockNode();

	dockNode->type = DockNode::Type::Tabs;
	dockNode->rect.set(0, 0, rect.width, rect.height);
	dockNode->nativeWindow = nativeWindow;
	ctx->dockingState.rootNativeWindowDockNodes.insert(std::make_pair(nativeWindow, dockNode));

	return dockNode;
}

DockNode* getRootDockNode(HNativeWindow nativeWindow)
{
	assert(nativeWindow);

	if (!nativeWindow) return nullptr;

	return ctx->dockingState.rootNativeWindowDockNodes[nativeWindow];
}

void deleteRootDockNode(HNativeWindow nativeWindow)
{
	assert(nativeWindow);

	auto node = ctx->dockingState.rootNativeWindowDockNodes[nativeWindow];

	if (node)
	{
		ctx->dockingState.nativeWindowsToDelete.insert(nativeWindow);
		ctx->dockingState.dockNodesToDelete.insert(node);
	}
}

Window* createWindow(const std::string& id, DockNode* targetNode, DockType dockType, const std::string& title, Rect* initialRect, HNativeWindow nativeWindow, HImage icon)
{
	auto targetNodePtr = (DockNode*)targetNode;
	auto newWnd = new Window();
	Rect defaultRect = { 100, 100, 1500, 1300 };
	
	newWnd->title = title;
	newWnd->id = id;
	newWnd->icon = icon;

	if (!targetNode)
	{
		if (!nativeWindow)
		{
			nativeWindow = createNativeWindow(title, NativeWindowFlags::Resizable, NativeWindowState::Normal, initialRect ? *initialRect : defaultRect);
		}
		
		newWnd->dockNode = createNativeWindowRootDockNode(nativeWindow);
		newWnd->dockNode->createdByUndocking = true;
		newWnd->dockNode->windows.push_back(newWnd);
		newWnd->clientRect = newWnd->dockNode->rect;
		ctx->dockingState.focusedWindow = newWnd;
	}

	ctx->dockingState.windows[id] = newWnd;

	if (targetNode)
	{
		dockWindow(newWnd, targetNode, dockType);
	}

	return newWnd;
}

void deleteWindow(Window* wnd)
{
	DockNode* node = wnd->dockNode;

	node->removeWindow(wnd);

	if (!node->parent)
	{
		destroyNativeWindow(node->nativeWindow);
	}
}

void closeWindow(Window* wnd)
{
	DockNode* node = wnd->dockNode;

	// if the nativeWindow is in a root dock node
	if (!node->parent
		&& node->children.empty()
		&& node->windows.size() == 1)
	{
		destroyNativeWindow(node->nativeWindow);
		node->nativeWindow = nullptr;
	}
}

bool dockWindow(Window* wnd, DockNode* targetNode, DockType dockType, u32 tabIndex, const Point* undockedWindowPos)
{
	auto source = wnd->dockNode;
	DockNode* target = targetNode;
	auto targetIsRoot = target ? !target->parent : true;
	auto sourceIsTarget = source == target;

	wnd->dockingNow = false;

	// if this is the root and its empty of any children and windows
	if (targetIsRoot && target && target->children.empty() && target->windows.empty() && target->type == DockNode::Type::None)
	{
		target->windows.push_back(wnd);
		wnd->dockNode = target;
		target->type = DockNode::Type::Tabs;
		target->selectedTabIndex = 0;
		
		return true;
	}

	DockNode* sourceNode = nullptr;
	DockNode* targetParent = nullptr;

	if (target)
	{
		if (!target->parent)
		{
			targetParent = target;
		}
		else
		{
			targetParent = target->parent;
		}
	}

	auto checkAndRelocateWindowsOfNode = [](DockNode* targetParent)
	{
		DockNode* newNode = nullptr;

		// if there are windows but no children nodes, create one
		if (!targetParent->windows.empty() && targetParent->children.empty())
		{
			newNode = new DockNode();

			newNode->copyFrom(targetParent);
			newNode->adoptWindows();
			newNode->parent = targetParent;
			targetParent->windows.clear();
			targetParent->children.push_back(newNode);

			return newNode;
		}

		return newNode;
	};

	if (target && !targetParent)
		return false;

	// check to see if we dock inside the same docknode which contains only one nativeWindow which is the same nativeWindow itself
	if (target && wnd->dockNode == target && target->children.empty() && target->windows.size() == 1 && target->windows[0] == wnd)
		return false;

	auto sizeToShare = 0;
	Rect origTargetRc = target ? target->rect : Rect();

	switch (dockType)
	{
	case hui::DockType::Left:
	{	
		if (!targetParent)
			break;

		// just insert at the target site
		if (targetParent->type == DockNode::Type::Horizontal || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkAndRelocateWindowsOfNode(targetParent);
			
			if (newTarget)
			{
				target = newTarget;
			}

			// if there is just one nativeWindow in the source node, move the node and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;

				// treat docking to root node
				if (targetParent != target && target)
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
			// we create a new node to hold the nativeWindow
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				if (source)	source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->nativeWindow = target ? target->nativeWindow : nullptr;
				newNode->rect = wndRect;

				// treat docking to root node
				if (targetParent != target && target)
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
			// if source has one nativeWindow, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				// remove the nativeWindow from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the nativeWindow
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->nativeWindow = targetParent->nativeWindow;
				sourceNode->rect = wndRect;
			}

			if (targetParent->type == DockNode::Type::Horizontal)
			{
				// treat docking to root node
				if (targetParent != target && target)
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
				
				if (target) newTargetNode->copyFrom(target);
				
				newTargetNode->parent = target;
				sourceNode->parent = target;
				newTargetNode->adoptChildren();
				newTargetNode->adoptWindows();
				
				if (target)
				{
					target->windows.clear();
					target->children.clear();
					target->children.push_back(sourceNode);
					target->children.push_back(newTargetNode);
					target->type = DockNode::Type::Horizontal;
				}

				target = newTargetNode;
			}
		}

		if (sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = origTargetRc.width * ctx->settings.dockNodeDockingSizeRatio;
			
			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;
			target->rect.x += size;
			target->rect.width -= size;
		}

		break;
	}
	case hui::DockType::Right:
	{
		if (!targetParent)
			break;

		// just insert at the target site
		if (targetParent->type == DockNode::Type::Horizontal || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkAndRelocateWindowsOfNode(targetParent);

			if (newTarget)
			{
				target = newTarget;
			}

			// if there is just one nativeWindow in the source node, move the node and remove from current parent
			if (source && source->windows.size() == 1)
			{				
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;

				// treat docking to root node
				if (targetParent != target && target)
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
			// we create a new node to hold the nativeWindow
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				if (source) source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->nativeWindow = target ? target->nativeWindow : nullptr;
				newNode->rect = wndRect;

				// treat docking to root node
				if (targetParent != target && target)
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
			// if source has one nativeWindow, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				// remove the nativeWindow from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the nativeWindow
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->nativeWindow = targetParent->nativeWindow;
				sourceNode->rect = wndRect;
			}

			if (targetParent->type == DockNode::Type::Horizontal)
			{
				// treat docking to root node
				if (targetParent != target && target)
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
				DockNode* newTargetNode = new DockNode();

				if (target) newTargetNode->copyFrom(target);

				newTargetNode->parent = target;
				sourceNode->parent = target;
				newTargetNode->adoptChildren();
				newTargetNode->adoptWindows();
				
				if (target)
				{
					target->windows.clear();
					target->children.clear();
					target->children.push_back(newTargetNode);
					target->children.push_back(sourceNode); // insert last
					target->type = DockNode::Type::Horizontal;
				}

				target = newTargetNode;
			}
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = origTargetRc.width * ctx->settings.dockNodeDockingSizeRatio;

			sourceNode->rect.x = target->rect.right() - size;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = size;
			sourceNode->rect.height = target->rect.height;
			target->rect.width -= size;
		}

		break;
	}
	case hui::DockType::Top:
	{
		if (!targetParent)
			break;

		// just insert at the target site
		if (targetParent->type == DockNode::Type::Vertical || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkAndRelocateWindowsOfNode(targetParent);

			if (newTarget)
			{
				target = newTarget;
			}

			// if there is just one nativeWindow in the source node, move the node also and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;

				// treat docking to root node
				if (targetParent != target && target)
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
			// we create a new node to hold the nativeWindow
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				if (source) source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->nativeWindow = target ? target->nativeWindow : target;
				newNode->rect = wndRect;

				if (targetParent != target && target)
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
			// if source has one nativeWindow, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				// remove the nativeWindow from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the nativeWindow
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->nativeWindow = targetParent->nativeWindow;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->rect = wndRect;
			}

			if (targetParent->type == DockNode::Type::Vertical)
			{
				// treat docking to root node
				if (targetParent != target && target)
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
			
				if (target)	newTargetNode->copyFrom(target);
				
				newTargetNode->parent = target;
				sourceNode->parent = target;
				
				newTargetNode->adoptChildren();
				newTargetNode->adoptWindows();
				
				if (target)
				{
					target->windows.clear();
					target->children.clear();
					target->children.push_back(sourceNode);
					target->children.push_back(newTargetNode);
					target->type = DockNode::Type::Vertical;
				}

				target = newTargetNode;
			}
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = origTargetRc.height * ctx->settings.dockNodeDockingSizeRatio;

			sourceNode->rect.x = target->rect.x;
			sourceNode->rect.y = target->rect.y;
			sourceNode->rect.width = target->rect.width;
			sourceNode->rect.height = size;

			target->rect.height -= size;
			target->rect.y += size;
		}
		break;
	}
	case hui::DockType::Bottom:
	{
		if (!targetParent)
			break;

		// just insert at the target site
		if (targetParent->type == DockNode::Type::Vertical || targetParent->type == DockNode::Type::Tabs)
		{
			// if there is no children nodes but has windows, relocate to new node
			auto newTarget = checkAndRelocateWindowsOfNode(targetParent);

			if (newTarget)
			{
				target = newTarget;
			}

			// if there is just one nativeWindow in the source node, move the node also and remove from current parent
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;

				// treat docking to root node
				if (targetParent != target && target)
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
			// we create a new node to hold the nativeWindow
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				if (source) source->removeWindow(wnd);
				DockNode* newNode = new DockNode();
				newNode->windows.push_back(wnd);
				wnd->dockNode = newNode;
				newNode->parent = targetParent;
				newNode->type = DockNode::Type::Tabs;
				newNode->nativeWindow = target ? target->nativeWindow : nullptr;
				newNode->rect = wndRect;

				auto iter = std::find(targetParent->children.begin(), targetParent->children.end(), target);

				// treat docking to root node
				if (targetParent != target && target)
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
			// if source has one nativeWindow, remove source from its parent
			// and just relocate to target parent node
			if (source && source->windows.size() == 1)
			{
				source = source->removeFromParent();
				source->parent = targetParent;
				source->nativeWindow = targetParent->nativeWindow;
				sourceNode = source;
			}
			else
			{
				auto wndRect = wnd->dockNode ? wnd->dockNode->rect : Rect();
				if (sourceIsTarget) target->removeWindow(wnd);
				// remove the nativeWindow from parent node
				if (source) source->removeWindow(wnd);
				// create new node for the nativeWindow
				sourceNode = new DockNode();
				sourceNode->windows.push_back(wnd);
				wnd->dockNode = sourceNode;
				sourceNode->parent = targetParent;
				sourceNode->nativeWindow = targetParent->nativeWindow;
				sourceNode->type = DockNode::Type::Tabs;
				sourceNode->rect = wndRect;
			}

			if (targetParent->type == DockNode::Type::Vertical)
			{
				// treat docking to root node
				if (targetParent != target && target)
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
				DockNode* newTargetNode = new DockNode();
			
				if (target) newTargetNode->copyFrom(target);
				
				newTargetNode->parent = target;
				sourceNode->parent = target;
				
				newTargetNode->adoptChildren();
				newTargetNode->adoptWindows();
				
				if (target)
				{
					target->windows.clear();
					target->children.clear();
					target->children.push_back(newTargetNode);
					target->children.push_back(sourceNode); // insert last
					target->type = DockNode::Type::Vertical;
				}

				target = newTargetNode;
			}
		}

		// if not proportional docking resize, then resize the target and compute size from it for source
		if (sourceNode
			&& (targetParent->type == DockNode::Type::Vertical
				|| targetParent->type == DockNode::Type::Horizontal))
		{
			f32 size = origTargetRc.height * ctx->settings.dockNodeDockingSizeRatio;

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
		if (!target)
			break;

		auto iter = std::next(target->windows.begin(), tabIndex);
		
		target->windows.insert(iter, wnd);
		target->type = DockNode::Type::Tabs;
		target->selectedTabIndex = tabIndex;

		// if we only have a nativeWindow in the source node, delete the node
		if (source && source->windows.size() == 1)
		{
			source = source->removeFromParent();
			ctx->dockingState.dockNodesToDelete.insert(source);
		}
		else
		{
			// remove from the source node if its from another dock node
			if (source && !sourceIsTarget)
				source->removeWindow(wnd);
		}

		wnd->dockNode = target;

		break;
	}
	case DockType::Floating:
	{
		if (source)
		{
			if (source->windows.size() == 1 && source->nativeWindow && !source->parent)
			{
				// already a floating single nativeWindow, skip
				return true;
			}

			if (source->windows.size() == 1)
			{
				source = source->removeFromParent();
				ctx->dockingState.dockNodesToDelete.insert(source);
				source = nullptr;
			}
			else
			{
				source->removeWindow(wnd);
			}
		}

		Rect rcWnd = wnd->clientRect;
		
		if (undockedWindowPos) {
			rcWnd.x = undockedWindowPos->x;
			rcWnd.y = undockedWindowPos->y;
		}

		auto nativeWnd = createNativeWindow(wnd->title, NativeWindowFlags::Resizable, NativeWindowState::Normal, rcWnd);
				
		wnd->dockNode = createNativeWindowRootDockNode(nativeWnd);
		wnd->dockNode->createdByUndocking = true;
		wnd->dockNode->windows.push_back(wnd);
		wnd->clientRect = wnd->dockNode->rect;

		break;
	}
	default:
		break;
	}

	ctx->dockingState.focusedWindow = wnd;

	for (auto& pair : ctx->dockingState.rootNativeWindowDockNodes)
	{
		auto node = pair.second;

		if (node->nativeWindow)
		{
			node->checkRedundancy();
			node->computeRect();
		}
	}

	return true;
}

void dockNodeTabs(DockNode* node)
{
	if (node->windows.size())
	{
		pushLayoutPadding(0);
		beginContainer(node->rect);
		// pop the clip rect, we dont want clipping since draw tabs bar beyond the node rect width
		auto oldClipRect = ctx->renderer->getClipRect();
		ctx->renderer->popClipRect();

		//TODO: not use ? panes
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
		u32 selectedIndex = 0;

		ctx->dockingState.drawingWindowTabs = true;

		// set clip rect for whole native window
		auto& rc = ctx->dockingState.rootNativeWindowDockNodes[node->nativeWindow]->rect;
		ctx->renderer->pushClipRect(rc, false);
		beginTabGroup(node->selectedTabIndex);

		for (auto i = 0; i < node->windows.size(); i++)
		{
			if (node->windows[i]->dockingNow)
			{
				ctx->penPosition.x += node->windows[i]->tabRect.width;

				continue;
			}

			if (node->dockingTabSpaceIndex == i)
			{
				ctx->penPosition.x += node->dockingTabSpaceWidth;
			}

			ctx->currentWindow = node->windows[i];
			hui::tab(node->windows[i]->title.c_str(), node->windows[i]->icon);
			node->windows[i]->tabRect = ctx->widget.rect;

			if (ctx->widget.hovered
				&& ctx->event.type == InputEvent::Type::MouseDown
				&& ctx->event.mouse.button == MouseButton::Middle)
			{
				closeTabIndex = i;
				ctx->event.type = InputEvent::Type::None;
				//TODO: issue some event on tab close ?
			}
		}

		selectedIndex = hui::endTabGroup();
		ctx->renderer->popClipRect();
		ctx->dockingState.drawingWindowTabs = false;

		if (closeTabIndex != ~0)
		{
			ctx->dockingState.windowsToDelete.insert(node->windows[closeTabIndex]);
			node->windows.erase(node->windows.begin() + closeTabIndex);

			if (node->windows.empty())
			{
				// this is an empty root dock node, destroy and close OS nativeWindow too
				if (!node->parent && node->createdByUndocking)
				{
					ctx->dockingState.dockNodesToDelete.insert(node);
					ctx->dockingState.nativeWindowsToDelete.insert(node->nativeWindow);
				}
				else
				{
					node = node->removeFromParent();
					ctx->dockingState.dockNodesToDelete.insert(node);
				}
			}

			auto wndCount = node->windows.size();

			if (selectedIndex >= wndCount && wndCount)
				selectedIndex = wndCount - 1;
		}

		if (!node->windows.empty())
		{
			if (node->selectedTabIndex != selectedIndex)
			{
				node->selectedTabIndex = selectedIndex;
				hui::forceRepaint();
			}
		}

		// just push the old clip rect so endContainer can pop it
		ctx->renderer->pushClipRect(oldClipRect, false);
		endContainer();
		popLayoutPadding();
	}

	for (auto& child : node->children)
	{
		dockNodeTabs(child);
	}
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
	printf("%snode: %p nativeWindow: %p parent: %p type: %s rc: %0.0f %0.0f %0.0f %0.0f\n", spaces.c_str(), node, node->nativeWindow, node->parent, enumTypeToStr(node->type).c_str(), node->rect.x, node->rect.y, node->rect.width, node->rect.height);

	for (auto& w : node->windows)
	{
		printf("%s `%s` %p node: %p tabRect: %f %f %f %f rc %f %f %f %f\n", spaces.c_str(), w->title.c_str(), w, w->dockNode, w->tabRect.x, w->tabRect.y, w->tabRect.width, w->tabRect.height, w->clientRect.x, w->clientRect.y, w->clientRect.width, w->clientRect.height);
	}

	for (auto& child : node->children)
	{
		printInfo(level + 1, child);
	}
}

void debugWindows()
{
	printf("------------------------------------------------------------------------------------------------------------\n");
	printf("Debug windows:\n\n");
	printf("%d windows\n", (u32)ctx->dockingState.rootNativeWindowDockNodes.size());

	for (auto& pair : ctx->dockingState.rootNativeWindowDockNodes)
	{
		printInfo(0, pair.second);
	}
}

void handleDockingMouseDown(const InputEvent& event, DockNode* node)
{
	if (event.mouse.button != MouseButton::Left)
		return;

	auto& ds = ctx->dockingState;
	const Point& mousePos = ctx->mousePosition;

	ds.lastMousePosSinceMouseDown = mousePos;
	ds.lastMousePos = mousePos;
	ds.resizingNode = node->findResizeDockNode(mousePos);
	ds.dragWindow = nullptr;
	ds.dragStarted = false;

	//TODO: we could check just the tabs of the current os window clicked on
	for (auto& wnd : ds.windows)
	{
		if (wnd.second->dockNode->nativeWindow != event.window)
			continue;

		if (wnd.second->clientRect.contains(mousePos) && wnd.second->dockNode->selectedTabIndex == wnd.second->dockNode->getWindowIndex(wnd.second))
		{
			ds.focusedWindow = wnd.second;
		}

		// return if the widget is not visible, that is outside current clip rect
		if (wnd.second->tabRect.outside(wnd.second->dockNode->rect))
		{
			continue;
		}

		Rect clippedRect = wnd.second->tabRect.clipInside(wnd.second->dockNode->rect);

		if (clippedRect.contains(mousePos.x, mousePos.y))
		{
			ds.dragWindow = wnd.second;
			ds.focusedWindow = wnd.second;
			ds.dragWindowMouseDelta = mousePos - ds.dragWindow->tabRect.topLeft();
			break;
		}
	}
}

void handleDockingMouseUp()
{
	auto& ds = ctx->dockingState;

	if (ds.dragWindow)
	{
		ds.dragWindow->dockingNow = false;
	}

	if (ds.dragWindow && ds.dragStarted)
	{
		size_t tabIndex = 0;
		bool allowDock = true;

		if (ds.dockToNode)
		{
			if (ds.dockType == DockType::AsTab)
			{
				tabIndex = ds.dockToNode->dockingTabSpaceIndex;

				// dock after last tab
				if (tabIndex == ~0)
					tabIndex = ds.dockToNode->windows.size();

				// if its trying to dock to the same dock node, just dont dock
				if (ds.dockToNode == ds.dragWindow->dockNode)
				{
					allowDock = false;
				}
			}

			if (ds.dockToNode->windows.size() == 1)
			{
				ds.dockToNode->selectedTabIndex = 0;
			}
		}

		// dock only if we dragged to a different dock node
		if (allowDock)
		{
			Point* wndPos = nullptr;
			Point pos;

			// undock the window if there is more than one in the dock node
			// and if the dock node is not a root node of the window
			if (ds.dockType == DockType::Floating && ctx->settings.dockAllowUndockingToNewNativeWindow)
			{
				auto& rc = ds.dragWindow->dockNode->rect;

				// we use the current screen mouse pos to undock the window to
				pos = HORUS_INPUT->getAbsoluteMousePosition();
				// put the window in the middle of the mouse coordinates
				pos.x -= rc.width / 2.0f;
				pos.y -= rc.height / 2.0f;
				wndPos = &pos;
			}

			dockWindow(ds.dragWindow, ds.dockToNode, ds.dockType, tabIndex, wndPos);
		}
		else
		{
			if (ds.dockToNode)
			{
				ds.dockToNode->selectedTabIndex = ds.dockToNode->dockingTabSpaceIndex;

				if (ds.dockToNode->selectedTabIndex == ~0 && ds.dockToNode->windows.size())
				{
					ds.dragWindow->dockNode->selectedTabIndex = ds.dockToNode->windows.size() - 1;
				}
			}
		}

		if (ds.dockToNode)
			ds.dockToNode->removeTabSpace();

		hui::forceRepaint();
	}

	if (ds.dragIndicatorNativeWindow)
	{
		HORUS_INPUT->destroyWindow(ds.dragIndicatorNativeWindow);
		ds.dragIndicatorNativeWindow = nullptr;
	}

	ds.dockToNode = nullptr;
	ds.dragWindow = nullptr;
	ds.resizingNode = nullptr;
	ds.dragStarted = false;
}

void handleDockNodeResize(DockNode* node)
{
	auto& ds = ctx->dockingState;
	auto& mousePos = ctx->mousePosition;
	DockNode* hoveredResizingNode = nullptr;

	hoveredResizingNode = node->findResizeDockNode(ctx->mousePosition);

	if (ds.resizingNode || (hoveredResizingNode && hoveredResizingNode->parent))
	{
		DockNode::Type nodeType = DockNode::Type::None;

		if (ds.resizingNode)
			nodeType = ds.resizingNode->parent->type;
		else
			if (hoveredResizingNode) nodeType = hoveredResizingNode->parent->type;

		switch (nodeType)
		{
		case DockNode::Type::Horizontal:
		{
			ctx->mouseCursor = MouseCursorType::SizeWE;
			break;
		}
		case DockNode::Type::Vertical:
		{
			ctx->mouseCursor = MouseCursorType::SizeNS;
			break;
		}
		default:
			break;
		}
	}

	if (ds.resizingNode && ds.resizingNode->parent)
	{
		switch (ds.resizingNode->parent->type)
		{
		case DockNode::Type::Vertical:
		{
			f32 pushAmount = 0;

			// moving up
			if (ds.mouseDragDelta.y < 0)
			{
				auto iterPrev = ds.resizingNode->parent->findPrevSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.y < ds.resizingNode->rect.bottom())
					ds.resizingNode->rect.height += ds.mouseDragDelta.y;

				if (ds.resizingNode->rect.height < ds.resizingNode->minSize.y)
				{
					pushAmount = ds.mouseDragDelta.y;

					// dont push y if first node
					if (ds.resizingNode->parent->children[0] != ds.resizingNode)
						ds.resizingNode->rect.y += pushAmount;

					ds.resizingNode->rect.height = ds.resizingNode->minSize.y;
				}

				while (iterPrev != ds.resizingNode->parent->children.rend())
				{
					(*iterPrev)->computeMinSize();
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(*iterPrev);

					(*iterPrev)->rect.height = (*iterNext)->rect.y - (*iterPrev)->rect.y - ctx->settings.dockNodeSpacing;

					if (*iterPrev != ds.resizingNode->parent->children[0]
						&& (*iterPrev)->rect.height <= (*iterPrev)->minSize.y)
					{
						(*iterPrev)->rect.y += pushAmount;
						(*iterPrev)->rect.height = (*iterPrev)->minSize.y;
					}
					else
					{
						// if first one, stop all from moving if its min size
						if (*iterPrev == ds.resizingNode->parent->children[0]
							&& (*iterPrev)->rect.height < (*iterPrev)->minSize.y)
						{
							auto iter = ds.resizingNode->parent->children.begin();
							auto crtY = (*iter)->rect.y;
							auto iterLast = ds.resizingNode->parent->getIteratorOf(ds.resizingNode);
							++iterLast; // need to advance to next

							while (true)
							{
								(*iter)->rect.y = crtY;
								(*iter)->rect.height = (*iter)->minSize.y;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtY += (*iter)->minSize.y + ctx->settings.dockNodeSpacing;
							};
						}
						(*iterPrev)->computeRect();
						break;
					}

					(*iterPrev)->computeRect();
					++iterPrev;
				}

				// resize node height after the resizing one
				{
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
					auto oldBottom = (*iterNext)->rect.bottom();

					(*iterNext)->rect.y = ds.resizingNode->rect.bottom() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.height = oldBottom - (*iterNext)->rect.y;
					(*iterNext)->computeRect();
				}
			}

			// moving down
			if (ds.mouseDragDelta.y > 0)
			{
				auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.y > ds.resizingNode->rect.bottom())
				{
					ds.resizingNode->rect.height += ds.mouseDragDelta.y;
					pushAmount = ds.mouseDragDelta.y;
				}

				auto prevNode = ds.resizingNode;

				while (iterNext != ds.resizingNode->parent->children.end())
				{
					(*iterNext)->computeMinSize();
					auto bottom = (*iterNext)->rect.bottom();
					(*iterNext)->rect.y = prevNode->rect.bottom() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.height = bottom - (*iterNext)->rect.y;
					prevNode = (*iterNext);

					if (*iterNext != ds.resizingNode->parent->children.back()
						&& (*iterNext)->rect.height < (*iterNext)->minSize.y)
					{
						(*iterNext)->rect.height = (*iterNext)->minSize.y;
					}
					else
					{
						// if last one, stop all from moving if min size
						if (*iterNext == ds.resizingNode->parent->children.back()
							&& (*iterNext)->rect.height < (*iterNext)->minSize.y)
						{
							auto iter = ds.resizingNode->parent->children.rbegin();
							auto crtY = (*iterNext)->parent->rect.bottom() - (*iterNext)->minSize.y;
							auto iterLast = ds.resizingNode->parent->getReverseIteratorOf(ds.resizingNode);

							while (true)
							{
								(*iter)->computeMinSize();
								(*iter)->rect.y = crtY;
								(*iter)->rect.height = (*iter)->minSize.y;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtY -= (*iter)->minSize.y + ctx->settings.dockNodeSpacing;
							};

							auto iterNext2 = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

							ds.resizingNode->rect.height = (*iterNext2)->rect.y - ds.resizingNode->rect.y - ctx->settings.dockNodeSpacing;

						}

						(*iterNext)->computeRect();
						break;
					}

					(*iterNext)->computeRect();
					++iterNext;
				}
			}

			break;
		}
		case DockNode::Type::Horizontal:
		{
			f32 pushAmount = 0;

			// moving left
			if (ds.mouseDragDelta.x < 0)
			{
				auto iterPrev = ds.resizingNode->parent->findPrevSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.x < ds.resizingNode->rect.right())
					ds.resizingNode->rect.width += ds.mouseDragDelta.x;

				if (ds.resizingNode->rect.width < ds.resizingNode->minSize.x)
				{
					pushAmount = ds.mouseDragDelta.x;

					// dont push x if first node
					if (ds.resizingNode->parent->children[0] != ds.resizingNode)
						ds.resizingNode->rect.x += pushAmount;

					ds.resizingNode->rect.width = ds.resizingNode->minSize.x;
				}

				while (iterPrev != ds.resizingNode->parent->children.rend())
				{
					(*iterPrev)->computeMinSize();
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(*iterPrev);

					(*iterPrev)->rect.width = (*iterNext)->rect.x - (*iterPrev)->rect.x - ctx->settings.dockNodeSpacing;

					if (*iterPrev != ds.resizingNode->parent->children[0]
						&& (*iterPrev)->rect.width <= (*iterPrev)->minSize.x)
					{
						(*iterPrev)->rect.x += pushAmount;
						(*iterPrev)->rect.width = (*iterPrev)->minSize.x;
					}
					else
					{
						// if first one, stop all from moving if its min size
						if (*iterPrev == ds.resizingNode->parent->children[0]
							&& (*iterPrev)->rect.width < (*iterPrev)->minSize.x)
						{
							auto iter = ds.resizingNode->parent->children.begin();
							auto crtX = (*iter)->rect.x;
							auto iterLast = ds.resizingNode->parent->getIteratorOf(ds.resizingNode);
							++iterLast; // need to advance to next

							while (true)
							{
								(*iter)->rect.x = crtX;
								(*iter)->rect.width = (*iter)->minSize.x;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtX += (*iter)->minSize.x + ctx->settings.dockNodeSpacing;
							};
						}
						(*iterPrev)->computeRect();
						break;
					}

					(*iterPrev)->computeRect();
					++iterPrev;
				}

				// resize node width after the resizing one
				{
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
					auto oldRight = (*iterNext)->rect.right();

					(*iterNext)->rect.x = ds.resizingNode->rect.right() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.width = oldRight - (*iterNext)->rect.x;
					(*iterNext)->computeRect();
				}
			}

			// moving right
			if (ds.mouseDragDelta.x > 0)
			{
				auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.x > ds.resizingNode->rect.right())
				{
					ds.resizingNode->rect.width += ds.mouseDragDelta.x;
					pushAmount = ds.mouseDragDelta.x;
				}

				auto prevNode = ds.resizingNode;

				while (iterNext != ds.resizingNode->parent->children.end())
				{
					(*iterNext)->computeMinSize();
					auto right = (*iterNext)->rect.right();
					(*iterNext)->rect.x = prevNode->rect.right() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.width = right - (*iterNext)->rect.x;

					prevNode = (*iterNext);

					if (*iterNext != ds.resizingNode->parent->children.back()
						&& (*iterNext)->rect.width < (*iterNext)->minSize.x)
					{
						(*iterNext)->rect.width = (*iterNext)->minSize.x;
					}
					else
					{
						// if last one, stop all from moving if min size
						if (*iterNext == ds.resizingNode->parent->children.back()
							&& (*iterNext)->rect.width < (*iterNext)->minSize.x)
						{
							auto iter = ds.resizingNode->parent->children.rbegin();
							auto crtX = (*iterNext)->parent->rect.right() - (*iterNext)->minSize.x;
							auto iterLast = ds.resizingNode->parent->getReverseIteratorOf(ds.resizingNode);

							while (true)
							{
								(*iter)->computeMinSize();
								(*iter)->rect.x = crtX;
								(*iter)->rect.width = (*iter)->minSize.x;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtX -= (*iter)->minSize.x + ctx->settings.dockNodeSpacing;
							};

							auto iterNext2 = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

							ds.resizingNode->rect.width = (*iterNext2)->rect.x - ds.resizingNode->rect.x - ctx->settings.dockNodeSpacing;

						}
						(*iterNext)->computeRect();
						break;
					}

					(*iterNext)->computeRect();
					++iterNext;
				}
			}

			break;
		}
		}

		ds.resizingNode->computeRect();
	}
}

void handleDockingMouseMove(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->dockingState;

	if (ds.dragWindow && ds.dragStarted)
	{
		ds.dragWindow->dockingNow = true;

		auto& tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
		const Point mousePos = ctx->mousePosition;

		if (ctx->lastHoveredNativeWindow == node->nativeWindow)
			ds.hoveredNode = node->findTargetDockNode(mousePos);

		bool isSameNode = ds.hoveredNode == ds.dragWindow->dockNode;
		bool isSingleWindow = ds.dragWindow->dockNode && ds.dragWindow->dockNode->windows.size() == 1;

		auto rootNode = node;
		auto& rootRect = rootNode->rect;
		auto parentRect = ds.hoveredNode ? ds.hoveredNode->rect : rootRect;

		// do check hit tests only if this is the hovered window
		if (ds.hoveredNode && ctx->lastHoveredNativeWindow == node->nativeWindow)
		{
			ds.hitBoxLeft = parentRect;
			ds.hitBoxRight = parentRect;
			ds.hitBoxTop = parentRect;
			ds.hitBoxBottom = parentRect;
			ds.hitBoxTabs = parentRect;

			if (ctx->settings.dockingStyle != DockingGuidesStyle::InsideNativeWindows)
			{
				ds.hitBoxLeft.width *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				ds.hitBoxRight.x += parentRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio);
				ds.hitBoxRight.width *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				ds.hitBoxTop.height *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				ds.hitBoxBottom.y += parentRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio);
				ds.hitBoxBottom.height *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				ds.hitBoxTabs.width += ctx->settings.dockNodeSpacing;
				ds.hitBoxTabs.height = tabGroupElem.normalState().height * 2.0f;

				ds.hitBoxRootLeft = rootNode->rect;
				ds.hitBoxRootRight = rootNode->rect;
				ds.hitBoxRootTop = rootNode->rect;
				ds.hitBoxRootBottom = rootNode->rect;

				ds.hitBoxRootLeft.width = ctx->settings.dockNodeRootDockingHitSize;

				ds.hitBoxRootRight.x = rootNode->rect.right() - ctx->settings.dockNodeRootDockingHitSize;
				ds.hitBoxRootRight.width = ctx->settings.dockNodeRootDockingHitSize;

				ds.hitBoxRootTop.height = ctx->settings.dockNodeRootDockingHitSize;
				ds.hitBoxRootTop.y -= 40;

				ds.hitBoxRootBottom.y = rootNode->rect.bottom() - ctx->settings.dockNodeRootDockingHitSize;
				ds.hitBoxRootBottom.height = ctx->settings.dockNodeRootDockingHitSize + 40;
			}
			else
			{
				// use width of the images to compute hit box
				// the guide images are considered to be square
				auto boxSizeV = ctx->theme->getElement(WidgetElementId::WindowDockGuideVerticalSplit).normalState().image->width * ctx->globalScale * ctx->settings.dockIndicatorBoxScale;
				auto boxSizeH = ctx->theme->getElement(WidgetElementId::WindowDockGuideHorizontalSplit).normalState().image->width * ctx->globalScale * ctx->settings.dockIndicatorBoxScale;
				auto boxSizeT = ctx->theme->getElement(WidgetElementId::WindowDockGuideAsTab).normalState().image->width * ctx->globalScale * ctx->settings.dockIndicatorBoxScale;

				auto boxGap = ctx->settings.dockIndicatorBoxSpacing;

				ds.hitBoxLeft = Rect(
					parentRect.x + parentRect.width / 2.0f - boxSizeV / 2.0f - boxGap - boxSizeV,
					parentRect.y + parentRect.height / 2.0f - boxSizeV / 2.0f,
					boxSizeV, boxSizeV);

				ds.hitBoxRight = Rect(
					parentRect.x + parentRect.width / 2.0f + boxSizeV / 2.0f + boxGap,
					parentRect.y + parentRect.height / 2.0f - boxSizeV / 2.0f,
					boxSizeV, boxSizeV);

				ds.hitBoxTop = Rect(
					parentRect.x + parentRect.width / 2.0f - boxSizeH / 2.0f,
					parentRect.y + parentRect.height / 2.0f - boxSizeH / 2.0f - boxSizeH - boxGap,
					boxSizeH, boxSizeH);

				ds.hitBoxBottom = Rect(
					parentRect.x + parentRect.width / 2.0f - boxSizeH / 2.0f,
					parentRect.y + parentRect.height / 2 + boxSizeH / 2 + boxGap,
					boxSizeH, boxSizeH);

				ds.hitBoxTabs = Rect(
					parentRect.x + parentRect.width / 2.0f - boxSizeT / 2.0f,
					parentRect.y + parentRect.height / 2.0f - boxSizeT / 2.0f,
					boxSizeT, boxSizeT);

				ds.hitBoxTabsBar = Rect(
					parentRect.x,
					parentRect.y,
					parentRect.width,
					tabGroupElem.normalState().height);

				ds.hitBoxRootLeft = Rect(
					boxGap,
					rootRect.y + rootRect.height / 2.0f - boxSizeV / 2.0f,
					boxSizeV, boxSizeV);

				ds.hitBoxRootRight = Rect(
					rootRect.right() - boxGap - boxSizeV,
					rootRect.y + rootRect.height / 2.0f - boxSizeV / 2.0f,
					boxSizeV, boxSizeV);

				ds.hitBoxRootTop = Rect(
					rootRect.x + rootRect.width / 2.0f - boxSizeH / 2.0f,
					rootRect.y + boxGap,
					boxSizeH, boxSizeH);

				ds.hitBoxRootBottom = Rect(
					rootRect.x + rootRect.width / 2.0f - boxSizeH / 2.0f,
					rootRect.bottom() - boxSizeH / 2.0f - boxGap,
					boxSizeH, boxSizeH);
			}

			ds.isHitBoxLeftHovered = ds.hitBoxLeft.contains(mousePos);
			ds.isHitBoxRightHovered = ds.hitBoxRight.contains(mousePos);
			ds.isHitBoxTopHovered = ds.hitBoxTop.contains(mousePos);
			ds.isHitBoxBottomHovered = ds.hitBoxBottom.contains(mousePos);
			ds.isHitBoxTabsHovered = ds.hitBoxTabs.contains(mousePos);
			ds.isHitBoxTabsBarHovered = ds.hitBoxTabsBar.contains(mousePos);

			ds.isHitBoxRootLeftHovered = ds.hitBoxRootLeft.contains(mousePos);
			ds.isHitBoxRootRightHovered = ds.hitBoxRootRight.contains(mousePos);
			ds.isHitBoxRootTopHovered = ds.hitBoxRootTop.contains(mousePos);
			ds.isHitBoxRootBottomHovered = ds.hitBoxRootBottom.contains(mousePos);

			if (ds.isHitBoxLeftHovered)
			{
				ds.dockToNode = ds.hoveredNode;
				ds.dockType = DockType::Left;
				ds.dragRect = parentRect;
				ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRightHovered)
			{
				ds.dockToNode = ds.hoveredNode;
				ds.dockType = DockType::Right;
				ds.dragRect = parentRect;
				ds.dragRect.x += ds.dragRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio);
				ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxTopHovered)
			{
				ds.dockToNode = ds.hoveredNode;
				ds.dockType = DockType::Top;
				ds.dragRect = parentRect;
				ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxBottomHovered)
			{
				ds.dockToNode = ds.hoveredNode;
				ds.dockType = DockType::Bottom;
				ds.dragRect = parentRect;
				ds.dragRect.y += floorf(ds.dragRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio));
				ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootLeftHovered)
			{
				ds.dockToNode = rootNode;
				ds.dockType = DockType::Left;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootRightHovered)
			{
				ds.dockToNode = rootNode;
				ds.dockType = DockType::Right;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.x += ds.dragRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio);
				ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootTopHovered)
			{
				ds.dockToNode = rootNode;
				ds.dockType = DockType::Top;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootBottomHovered)
			{
				ds.dockToNode = rootNode;
				ds.dockType = DockType::Bottom;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.y += floorf(ds.dragRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio));
				ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxTabsHovered || ds.isHitBoxTabsBarHovered)
			{
				ds.dockToNode = ds.hoveredNode;
				ds.dockType = DockType::AsTab;

				if (ctx->settings.dockingStyle == DockingGuidesStyle::NativeWindows || ds.isHitBoxTabsBarHovered)
				{
					ds.dragRect = parentRect;
					ds.dragRect.x = mousePos.x - ds.dragWindowMouseDelta.x;
					ds.dragRect.width = ds.dragWindow->tabRect.width;
					ds.dragRect.height = tabGroupElem.normalState().height;
				}
				else
				{
					ds.dragRect = parentRect;
				}

				if (isSameNode)
				{
					if (ds.hoveredNode->windows.size() > 1)
						ds.hoveredNode->moveWindowTabAt(mousePos, ds.dragWindow);
				}
				else
				{
					ds.hoveredNode->insertTabSpaceAt(mousePos, ds.dragWindow->tabRect.width);
				}
			}
		}
	}

	handleDockNodeResize(node);
}

void handleDockNodeEvents(DockNode* node)
{
	auto& rect = node->rect;
	auto& event = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view pane had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		// node is disabled for input
		return;
	}

	// is the event for this window ?
	if (ctx->lastHoveredNativeWindow == node->nativeWindow)
	{
		if (event.type == InputEvent::Type::MouseDown)
			handleDockingMouseDown(event, node);
	};

	handleDockingMouseMove(event, node);
}

void drawDockGuides()
{
	auto& ds = ctx->dockingState;

	auto& dockGuideVerticalSplitNormalElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideVerticalSplit).normalState();
	auto& dockGuideHorizontalSplitNormalElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideHorizontalSplit).normalState();
	auto& dockGuideAsTabNormalElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideAsTab).normalState();

	auto& dockGuideVerticalSplitHoveredElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideVerticalSplit).hoveredState();
	auto& dockGuideHorizontalSplitHoveredElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideHorizontalSplit).hoveredState();
	auto& dockGuideAsTabHoveredElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideAsTab).hoveredState();

	// draw the guides
	ctx->renderer->cmdSetColor(ds.isHitBoxLeftHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxLeftHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.hitBoxLeft);

	ctx->renderer->cmdSetColor(ds.isHitBoxRightHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxRightHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.hitBoxRight);

	ctx->renderer->cmdSetColor(ds.isHitBoxTopHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxTopHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxTop);

	ctx->renderer->cmdSetColor(ds.isHitBoxBottomHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxBottomHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxBottom);

	// root node guides
	ctx->renderer->cmdSetColor(ds.isHitBoxRootLeftHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer->cmdDrawImageBordered(
		ds.isHitBoxRootLeftHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.isHitBoxRootLeftHovered ?
		dockGuideVerticalSplitHoveredElem.border : dockGuideVerticalSplitNormalElem.border,
		ds.hitBoxRootLeft, ctx->globalScale);

	ctx->renderer->cmdSetColor(ds.isHitBoxRootRightHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxRootRightHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.hitBoxRootRight);

	ctx->renderer->cmdSetColor(ds.isHitBoxRootTopHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxRootTopHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxRootTop);

	ctx->renderer->cmdSetColor(ds.isHitBoxRootBottomHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxRootBottomHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxRootBottom);

	ctx->renderer->cmdSetColor(ds.isHitBoxTabsHovered ? dockGuideAsTabHoveredElem.color : dockGuideAsTabNormalElem.color);
	ctx->renderer->cmdDrawImage(
		ds.isHitBoxTabsHovered ?
		dockGuideAsTabHoveredElem.image : dockGuideAsTabNormalElem.image,
		ds.hitBoxTabs);
}

void drawDockPreview(Window* window, const Rect& windowRect)
{
	auto& ds = ctx->dockingState;

	ctx->renderer->pushClipRect(windowRect, false);

	auto& windowElem = ctx->theme->getElement(WidgetElementId::WindowBody).normalState();

	Color tintColor;

	if (ctx->settings.dockingStyle == DockingGuidesStyle::NativeWindows)
	{
		auto tintColorStr = hui::getThemeUserSetting(ctx->theme, "dockPreviewNativeWindowsColorTint");

		if (tintColorStr && strcmp(tintColorStr, ""))
			tintColor = getColorFromText(tintColorStr);
	}
	else
	{
		auto tintColorStr = hui::getThemeUserSetting(ctx->theme, "dockPreviewInsideWindowsColorTint");

		if (tintColorStr && strcmp(tintColorStr, ""))
			tintColor = getColorFromText(tintColorStr);
	}

	ctx->renderer->cmdSetColor(windowElem.color * tintColor);
	ctx->renderer->cmdDrawImageBordered(windowElem.image, windowElem.border, windowRect, ctx->globalScale);
	pushLayoutPadding(0);
	beginContainer(windowRect);
	beginTabGroup(0);
	hui::tab(window->title.c_str(), window->icon);
	endTabGroup();
	endContainer();
	popLayoutPadding();
	ctx->renderer->popClipRect();
}

void updateDockingSystem()
{
	auto copyOfRootNativeWindowDockNodes = ctx->dockingState.rootNativeWindowDockNodes;
	auto& ds = ctx->dockingState;
	const auto& mousePos = ctx->mousePosition;

	if (ds.dragWindow)
	{
		ds.dragStarted = (ds.lastMousePosSinceMouseDown - mousePos).getLength() > ctx->settings.dragStartDistance;
	}

	ds.mouseDragDelta = mousePos - ds.lastMousePos;

	auto screenMousePos = HORUS_INPUT->getAbsoluteMousePosition();
	Rect screenRect;

	// remember last valid hovered node to remove insertion space when changed
	ds.lastHoveredNode = ds.hoveredNode ? ds.hoveredNode : ds.lastHoveredNode;
	ds.dockToNode = nullptr;
	ds.hoveredNode = nullptr;
	ds.dockType = DockType::Floating;

	for (auto& wnd : copyOfRootNativeWindowDockNodes)
	{
		handleDockNodeEvents(wnd.second);
	}

	if (ctx->event.type == InputEvent::Type::WindowResized || ctx->event.type == InputEvent::Type::WindowMoved)
	{
		for (auto& pair : ctx->dockingState.rootNativeWindowDockNodes)
		{
			pair.second->computeRect();
		}
	}

	if (ds.dragWindow 
		&& ds.dragStarted
		&& ctx->settings.dockingStyle == DockingGuidesStyle::NativeWindows
		&& !ds.dragIndicatorNativeWindow
		&& ctx->lastHoveredNativeWindow)
	{
		Point wndPos = HORUS_INPUT->getWindowPosition(ctx->lastHoveredNativeWindow);

		screenRect = ds.dragRect + wndPos;

		ds.dragIndicatorNativeWindow = HORUS_INPUT->createWindow(ds.dragWindow->title.c_str(), NativeWindowFlags::NoInput | NativeWindowFlags::NoDecoration | NativeWindowFlags::Resizable, NativeWindowState::Normal, screenRect);

		ds.dragWindow->dockingNow = true;
	}

	// if we release the mouse button, wherever it is, over a window or not
	// then force a mouse up button
	if (!HORUS_INPUT->isMouseButtonDownNow(MouseButton::Left)
		&& ds.dragWindow)
	{
		ctx->event.type = InputEvent::Type::MouseUp;
	}

	if (ctx->event.type == InputEvent::Type::MouseUp)
	{
		handleDockingMouseUp();
	}

	if (ds.lastHoveredNode && ds.lastHoveredNode != ds.hoveredNode)
	{
		ds.lastHoveredNode->removeTabSpace();
	}

	screenRect = ds.dragRect;

	if (ds.dragIndicatorNativeWindow && ds.dragWindow && ds.dragStarted)
	{
		// if we try to dock on dock nodes sides or tabs area
		if (ctx->lastHoveredNativeWindow
			&& ds.dockType != DockType::None
			&& ds.dockType != DockType::Floating
			&& ds.hoveredNode)
		{
			auto pos = HORUS_INPUT->getWindowPosition(ds.hoveredNode->nativeWindow);

			screenRect = ds.dragRect;
			screenRect += pos;
		}
		else
		{
			// resize window as floating window
			auto mousePosAbs = HORUS_INPUT->getAbsoluteMousePosition();
			screenRect = ds.dragWindow->dockNode->rect;
			screenRect *= 0.6f; // scale back a bit from original size
			screenRect.x = mousePosAbs.x - screenRect.width / 2;
			screenRect.y = mousePosAbs.y - screenRect.height / 2;
		}

		ds.dockType = DockType::Floating;
		HORUS_INPUT->setWindowPosition(ds.dragIndicatorNativeWindow, screenRect.topLeft());
		HORUS_INPUT->setWindowSize(ds.dragIndicatorNativeWindow, screenRect.getSize());
	}

	if (ds.dragWindow
		&& ctx->settings.dockingStyle != DockingGuidesStyle::NativeWindows
		&& ds.dockType == DockType::Floating)
	{
		ds.dragRect = ds.dragWindow->dockNode->rect;
		ds.dragRect *= 0.6f; // scale back a bit from original size
		ds.dragRect.x = mousePos.x - ds.dragWindowMouseDelta.x;
		ds.dragRect.y = mousePos.y - ds.dragWindowMouseDelta.y;
	}

	if (ds.dragWindow)
	{
		if (ctx->settings.dockingStyle == DockingGuidesStyle::NativeWindows)
		{
			if (ctx->dockingState.dragIndicatorNativeWindow)
			{
				HORUS_INPUT->setCurrentWindow(ds.dragIndicatorNativeWindow);
				ctx->renderer->disableRendering = false;
				ctx->renderer->setCurrentNativeWindow(ds.dragIndicatorNativeWindow);
				ctx->renderer->setWindowSize(screenRect.getSize());
				ctx->renderer->begin();
				// the rect is in screen coords, just make it relative to our dragged indicator window
				screenRect.x = screenRect.y = 0;
				drawDockPreview(ds.dragWindow, screenRect);
				ctx->renderer->end();
				ctx->renderer->executeDrawCommands(ds.dragIndicatorNativeWindow);
				HORUS_INPUT->presentWindow(ds.dragIndicatorNativeWindow);
			}
		}
		else
		{
			if (ds.hoveredNode)
			{
				auto rc = ds.rootNativeWindowDockNodes[ds.hoveredNode->nativeWindow]->rect;

				HORUS_INPUT->setCurrentWindow(ds.hoveredNode->nativeWindow);
				ctx->renderer->disableRendering = false;
				ctx->renderer->setCurrentNativeWindow(ds.hoveredNode->nativeWindow);
				ctx->renderer->setWindowSize(rc.getSize());
				ctx->renderer->begin();
				// we need to render last, so choose the highest z order
				auto oldZOrder = ctx->renderer->setZOrder(~0);
				drawDockPreview(ds.dragWindow, ds.dragRect);
				drawDockGuides();
				ctx->renderer->end();
				// restore z order
				ctx->renderer->setZOrder(oldZOrder);
				ctx->renderer->executeDrawCommands(ds.hoveredNode->nativeWindow);
			}
		}
	}

	if (ctx->event.type == InputEvent::Type::WindowClose)
	{
		auto node = ctx->dockingState.rootNativeWindowDockNodes[ctx->event.window];

		if (node)
		{
			node->removeWindowsAndDeleteChildrenRecursive();
		}

		ctx->dockingState.nativeWindowsToDelete.insert(ctx->event.window);
	}

	ds.lastMousePos = mousePos;
	ds.lastHoveredNode = ds.hoveredNode;
}

}
