#include <cstring>
#include <algorithm>
#include <math.h>
#include "context.h"
#include "docking.h"
#include "theme.h"

namespace hui
{
static DockNode* dockNodeGetAreaNode();
bool dockNodeHasChromeWindow(DockNode* node);

DockNode::DockNode()
{
	if (ctx)
	{
		id = ctx->docking.nextDockNodeId++;
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
		ctx->docking.windowsToDelete.insert(wnd);
	}

	windows.clear();

	for (auto& child : children)
	{
		child->removeWindowsAndDeleteChildrenRecursive();
		ctx->docking.dockNodesToDelete.insert(child);
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
		if (createdByDockingSystem)
		{
			// this is a root node and removing it we must destroy the window too
			ctx->docking.nativeWindowsToDelete.insert(nativeWindow);

			// we need to remove this now, it will interfere with redudancy checks
			auto iter = std::find(ctx->nativeWindows.begin(), ctx->nativeWindows.end(), nativeWindow);

			HUI_ASSERT(iter != ctx->nativeWindows.end());

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

bool dockNodeShowsTabBar(DockNode* node)
{
	return !(node->windows.size() == 1 && !!(node->windows[0]->flags & WindowFlags::NoTab));
}

bool dockNodeHasNoResize(DockNode* node)
{
	for (auto& wnd : node->windows)
	{
		if (!!(wnd->flags & WindowFlags::NoResize))
			return true;
	}

	return false;
}

f32 dockNodeFixedWidth(DockNode* node)
{
	for (auto& wnd : node->windows)
	{
		if (wnd->fixedSize.x > 0.0f)
			return wnd->fixedSize.x * ctx->scale;
	}

	return 0.0f;
}

f32 dockNodeFixedHeight(DockNode* node)
{
	for (auto& wnd : node->windows)
	{
		if (wnd->fixedSize.y > 0.0f)
			return wnd->fixedSize.y * ctx->scale;
	}

	return 0.0f;
}

void dockNodeUpdateDockSites(DockNode* node)
{
	if (node->type == DockNode::Type::Vertical || node->type == DockNode::Type::Horizontal)
	{
		// drop the previous empty dock sites, they are added back below only if still needed
		for (auto it = node->children.begin(); it != node->children.end(); )
		{
			auto child = *it;

			if (child->emptyDockSite && child->windows.empty() && child->children.empty())
			{
				ctx->docking.dockNodesToDelete.insert(child);
				it = node->children.erase(it);
			}
			else
			{
				++it;
			}
		}

		bool allFixed = !node->children.empty();

		for (auto& child : node->children)
		{
			f32 fixed = node->type == DockNode::Type::Vertical
				? dockNodeFixedHeight(child)
				: dockNodeFixedWidth(child);

			if (fixed <= 0.0f)
			{
				allFixed = false;
				break;
			}
		}

		// when all children have a fixed size and they leave empty space behind, add an empty dock site
		// so that windows can still be docked back into that empty area
		if (allFixed)
		{
			auto lastChild = node->children.back();
			f32 leftover = node->type == DockNode::Type::Vertical
				? node->rect.bottom() - lastChild->rect.bottom()
				: node->rect.right() - lastChild->rect.right();

			if (leftover > ctx->settings.dockNodeSpacing)
			{
				auto dockSite = new DockNode();

				dockSite->parent = node;
				dockSite->nativeWindow = node->nativeWindow;
				dockSite->type = DockNode::Type::None;
				dockSite->emptyDockSite = true;
				node->children.push_back(dockSite);
			}
		}
	}

	for (auto& child : node->children)
		dockNodeUpdateDockSites(child);
}

void dockNodeRecalculateLayout(DockNode* root)
{
	if (!root)
		return;

	root->checkRedundancy();
	root->computeRect();
	dockNodeUpdateDockSites(root);
	root->checkRedundancy();
	root->computeRect();
}

void DockNode::computeRect()
{
	if (!parent)
	{
		auto size = ctx->settings.services.getWindowSize(nativeWindow);
		rect = { 0, 0, size.x, size.y };

	}

	switch (type)
	{
	case DockNode::Type::None:
	case DockNode::Type::Tabs:
	{
		auto tabGroupHeight = ctx->theme ? ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState().height * ctx->scale : 0;
		auto topOffset = dockNodeShowsTabBar(this) ? tabGroupHeight : 0.0f;

		for (auto& wnd : windows)
		{
			wnd->clientRect = {
				rect.x, rect.y + topOffset, rect.width, rect.height - topOffset
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
		f32 fixedSpace = 0;

		for (auto& child : children)
		{
			f32 fixed = dockNodeFixedHeight(child);

			if (fixed > 0.0f)
			{
				fixedSpace += fixed;
				continue;
			}

			if (child->rect.height <= 0.0f)
			{
				child->rect.height = averageSpace;
			}

			totalSpace += child->rect.height;
		}

		f32 autoSpace = fmaxf(availableSpace - fixedSpace, 0.0f);

		f32 currentY = rect.y;

		for (auto& child : children)
		{
			f32 fixed = dockNodeFixedHeight(child);
			f32 height = fixed > 0.0f
				? fixed
				: (totalSpace > 0.0f ? child->rect.height / totalSpace * autoSpace : autoSpace);

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
		f32 fixedSpace = 0;

		averageSpace = (i32)averageSpace;

		for (auto& child : children)
		{
			f32 fixed = dockNodeFixedWidth(child);

			if (fixed > 0.0f)
			{
				fixedSpace += fixed;
				continue;
			}

			if (child->rect.width <= 0.0f)
			{
				child->rect.width = averageSpace;
			}

			totalSpace += child->rect.width;
		}

		f32 autoSpace = fmaxf(availableSpace - fixedSpace, 0.0f);

		f32 currentX = rect.x;

		for (auto& child : children)
		{
			f32 fixed = dockNodeFixedWidth(child);
			f32 width = fixed > 0.0f
				? fixed
				: (totalSpace > 0.0f ? child->rect.width / totalSpace * autoSpace : autoSpace);

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

	// a node holding a fixed size window cannot be made smaller than that size
	f32 fixedWidth = dockNodeFixedWidth(this);
	f32 fixedHeight = dockNodeFixedHeight(this);

	if (fixedWidth > 0.0f)
		minSize.x = fixedWidth;

	if (fixedHeight > 0.0f)
		minSize.y = fixedHeight;

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

		// the root must keep a chrome child as its own node, collapsing it makes the
		// chrome window fill the whole native window
		bool collapsingChromeRootChild = !parent && dockNodeHasChromeWindow(child);

		// the dock area must keep its own node, never collapse it into its parent
		bool isDockArea = ctx->docking.dockAreaNodeId && child->id == ctx->docking.dockAreaNodeId;

		if (!collapsingChromeRootChild && !isDockArea)
		{
			children = child->children;
			windows = child->windows;

			adoptChildren();
			adoptWindows();
			selectedTabIndex = child->selectedTabIndex;
			type = child->type;

			ctx->docking.dockNodesToDelete.insert(child);
		}
	}

	if (parent)
	{
		bool deleteThis = false;

		// a regular node directly under the root is the content area, never merge it
		// into the root, the chrome windows would become its siblings
		bool isRootContent = !parent->parent && !dockNodeHasChromeWindow(this);

		// if same type as parent, merge its nodes and windows into parent, but never
		// merge the dock area, it must keep its own node so the docking stays restricted
		if (parent->type == type
			&& !isRootContent
			&& (!ctx->docking.dockAreaNodeId || id != ctx->docking.dockAreaNodeId))
		{
			auto iterPosThis = std::find(parent->children.begin(), parent->children.end(), this);

			if (iterPosThis != parent->children.end())
			{
				parent->children.insert(iterPosThis, children.begin(), children.end());
			}

			// find it again, remove it, leaving children in the parent node
			iterPosThis = std::find(parent->children.begin(), parent->children.end(), this);

			HUI_ASSERT(iterPosThis != parent->children.end());

			if (iterPosThis != parent->children.end())
			{
				parent->children.erase(iterPosThis);
				parent->adoptChildren();
				parent->computeRect();
			}
			
			deleteThis = true;
		}

		if (deleteThis)
		{
			ctx->docking.dockNodesToDelete.insert(this);
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
					auto next = findNextSiblingOf(child);

					if (dockNodeHasNoResize(child) || (next != children.end() && dockNodeHasNoResize(*next)))
						continue;

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
					auto next = findNextSiblingOf(child);

					if (dockNodeHasNoResize(child) || (next != children.end() && dockNodeHasNoResize(*next)))
						continue;

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
	for (size_t i = 0; i < windows.size(); i++)
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

static DockNodeType dockingStateFromNodeType(DockNode::Type type)
{
	switch (type)
	{
	case DockNode::Type::Tabs: return DockNodeType::Tabs;
	case DockNode::Type::Vertical: return DockNodeType::Vertical;
	case DockNode::Type::Horizontal: return DockNodeType::Horizontal;
	default: return DockNodeType::None;
	}
}

static DockNode::Type dockingStateToNodeType(DockNodeType type)
{
	switch (type)
	{
	case DockNodeType::Tabs: return DockNode::Type::Tabs;
	case DockNodeType::Vertical: return DockNode::Type::Vertical;
	case DockNodeType::Horizontal: return DockNode::Type::Horizontal;
	default: return DockNode::Type::None;
	}
}

// saves a dock node and its children as flat entries, the parent is referenced by index
static void dockingStateSaveNode(
	const DockNode* node,
	i32 parentIndex,
	i32 nativeWindowIndex,
	WindowsDockingState& dockingState,
	std::unordered_map<DockNodeId, i32>& nodeIndices)
{
	if (!node || (node->emptyDockSite && node->windows.empty() && node->children.empty()
		&& node->id != ctx->docking.dockAreaNodeId))
		return;

	WindowsDockingState::DockNodeInfo info;

	info.parentIndex = parentIndex;
	info.nativeWindowIndex = nativeWindowIndex;
	info.type = dockingStateFromNodeType(node->type);
	info.rect = node->rect;
	info.selectedTabIndex = node->selectedTabIndex;

	for (auto wnd : node->windows)
	{
		if (wnd && !wnd->id.empty())
			info.windows.push_back({ wnd->id, wnd->title, (u32)wnd->flags });
	}

	i32 nodeIndex = (i32)dockingState.dockNodes.size();

	dockingState.dockNodes.push_back(info);
	nodeIndices[node->id] = nodeIndex;

	for (auto child : node->children)
		dockingStateSaveNode(child, nodeIndex, nativeWindowIndex, dockingState, nodeIndices);
}

void dockingStateSaveInMemory(WindowsDockingState& dockingState)
{
	dockingState = WindowsDockingState();

	if (!ctx)
		return;

	// recover a stale dock area before saving, so the restriction persists
	dockNodeGetAreaNode();

	std::unordered_map<DockNodeId, i32> nodeIndices;

	// iterate the native windows in creation order so saving is deterministic
	for (auto nativeWindow : ctx->nativeWindows)
	{
		auto iter = ctx->docking.rootNativeWindowDockNodes.find(nativeWindow);

		if (iter == ctx->docking.rootNativeWindowDockNodes.end() || !iter->second)
			continue;

		DockNode* root = iter->second;

		if (!root->nativeWindow)
			continue;

		WindowsDockingState::NativeWindowInfo info;
		auto pos = ctx->settings.services.getWindowPosition(nativeWindow);
		auto size = ctx->settings.services.getWindowSize(nativeWindow);

		info.floating = root->createdByDockingSystem;
		info.rect.set(pos.x, pos.y, size.x, size.y);

		i32 nativeWindowIndex = (i32)dockingState.nativeWindows.size();

		dockingState.nativeWindows.push_back(info);
		dockingStateSaveNode(root, -1, nativeWindowIndex, dockingState, nodeIndices);
	}

	// remember the dock area node, so the docking restriction survives a load
	if (ctx->docking.dockAreaNodeId)
	{
		auto areaIter = ctx->docking.dockNodeIdsMap.find(ctx->docking.dockAreaNodeId);

		// the dock area must never be one of the chrome nodes
		if (areaIter != ctx->docking.dockNodeIdsMap.end() && areaIter->second
			&& !dockNodeHasChromeWindow(areaIter->second))
		{
			auto iter = nodeIndices.find(ctx->docking.dockAreaNodeId);

			if (iter != nodeIndices.end())
				dockingState.dockAreaNodeIndex = iter->second;
		}
	}

	// the closed windows are saved with their last screen rect and dock node, so
	// they stay hidden and reopen where they were
	for (const auto& id : ctx->docking.hiddenWindows)
	{
		WindowsDockingState::ClosedWindowInfo info;

		info.id = id;

		auto rectIter = ctx->docking.closedWindowsRects.find(id);

		if (rectIter != ctx->docking.closedWindowsRects.end())
			info.rect = rectIter->second;

		auto assignIter = ctx->docking.windowsDockNodeAssignments.find(id);

		if (assignIter != ctx->docking.windowsDockNodeAssignments.end())
		{
			auto indexIter = nodeIndices.find(assignIter->second);

			if (indexIter != nodeIndices.end())
				info.parentIndex = indexIter->second;
		}

		auto slotIter = ctx->docking.hiddenWindowSlots.find(id);

		if (slotIter != ctx->docking.hiddenWindowSlots.end())
		{
			auto slotParentIter = nodeIndices.find(slotIter->second.parentNodeId);

			if (slotParentIter != nodeIndices.end())
			{
				info.slotParentIndex = slotParentIter->second;
				info.slotIndex = (i32)slotIter->second.index;
				info.slotVertical = slotIter->second.verticalParent;
				info.slotFraction = slotIter->second.fraction;
			}
		}

		dockingState.closedWindows.push_back(info);
	}

	std::sort(dockingState.closedWindows.begin(), dockingState.closedWindows.end(),
		[](const WindowsDockingState::ClosedWindowInfo& a, const WindowsDockingState::ClosedWindowInfo& b)
		{
			return a.id < b.id;
		});
}

// deletes a dock node tree, the windows docked in it must be detached first
static void dockingStateDeleteNodeTree(DockNode* node)
{
	if (!node)
		return;

	for (auto child : node->children)
		dockingStateDeleteNodeTree(child);

	ctx->docking.dockNodesToDelete.erase(node);
	ctx->docking.dockNodeIdsMap.erase(node->id);

	delete node;
}

static void dockingStateDetachWindows(DockNode* node)
{
	if (!node)
		return;

	for (auto wnd : node->windows)
	{
		if (wnd)
			wnd->dockNode = nullptr;
	}

	node->windows.clear();

	for (auto child : node->children)
		dockingStateDetachWindows(child);
}

static DockNode* dockingStateFindTabsNode(DockNode* node)
{
	if (!node)
		return nullptr;

	if (node->type == DockNode::Type::Tabs)
		return node;

	for (auto child : node->children)
	{
		DockNode* found = dockingStateFindTabsNode(child);

		if (found)
			return found;
	}

	return nullptr;
}

// moves the regular windows that ended up as siblings of the dock area in the root
// back into the area, used to repair layouts saved with the area restriction broken
static void dockingStateHealRoot(DockNode* root, DockNode* area)
{
	if (!root || !area || area->parent != root)
		return;

	std::vector<DockNode*> orphans;

	for (auto child : root->children)
	{
		if (child != area && !dockNodeHasChromeWindow(child))
			orphans.push_back(child);
	}

	if (orphans.empty())
		return;

	// the area must split vertically along its children, wrap its current content if needed
	if (area->type != DockNode::Type::Vertical)
	{
		if (area->type == DockNode::Type::Horizontal)
		{
			DockNode* content = new DockNode();

			content->type = DockNode::Type::Horizontal;
			content->rect = area->rect;
			content->parent = area;
			content->nativeWindow = area->nativeWindow;
			content->children = area->children;

			for (auto child : content->children)
				child->parent = content;

			ctx->docking.dockNodeIdsMap[content->id] = content;
			area->children.clear();
			area->children.push_back(content);
		}
		else if (!area->windows.empty())
		{
			DockNode* content = new DockNode();

			content->type = DockNode::Type::Tabs;
			content->rect = area->rect;
			content->parent = area;
			content->nativeWindow = area->nativeWindow;
			content->windows = area->windows;

			for (auto wnd : content->windows)
				wnd->dockNode = content;

			ctx->docking.dockNodeIdsMap[content->id] = content;
			area->windows.clear();
			area->children.push_back(content);
		}

		area->type = DockNode::Type::Vertical;
	}

	for (auto orphan : orphans)
	{
		auto iter = std::find(root->children.begin(), root->children.end(), orphan);

		if (iter != root->children.end())
			root->children.erase(iter);

		orphan->parent = area;
		orphan->nativeWindow = area->nativeWindow;
		area->children.push_back(orphan);
	}
}

bool dockingStateLoadFromMemory(const WindowsDockingState& dockingState)
{
	if (!ctx)
		return false;

	i32 nodeCount = (i32)dockingState.dockNodes.size();
	i32 nativeWindowCount = (i32)dockingState.nativeWindows.size();

	if (nodeCount == 0 || nativeWindowCount == 0)
		return false;

	// validate the flat node list before touching the current layout
	std::vector<i32> rootCounts(nativeWindowCount, 0);

	for (i32 i = 0; i < nodeCount; i++)
	{
		const auto& info = dockingState.dockNodes[i];

		if (info.nativeWindowIndex < 0 || info.nativeWindowIndex >= nativeWindowCount)
			return false;

		// the parent must exist and come before the child in the flat list, which
		// also rules out cycles in the tree
		if (info.parentIndex < -1 || info.parentIndex >= i)
			return false;

		if (info.parentIndex >= 0)
		{
			if (dockingState.dockNodes[info.parentIndex].nativeWindowIndex != info.nativeWindowIndex)
				return false;
		}
		else if (++rootCounts[info.nativeWindowIndex] > 1)
		{
			return false;
		}
	}

	for (auto count : rootCounts)
	{
		if (count == 0)
			return false;
	}

	std::unordered_set<std::string> savedWindowIds;

	for (const auto& info : dockingState.dockNodes)
	{
		for (const auto& wndInfo : info.windows)
		{
			// a window can only be docked in one node
			if (wndInfo.id.empty() || !savedWindowIds.insert(wndInfo.id).second)
				return false;
		}
	}

	// apply the pending deletions first so the maps only hold live objects
	deferredDeleteObjects();

	// hide the windows that the state has closed before rebuilding the layout
	for (const auto& info : dockingState.closedWindows)
	{
		if (info.id.empty())
			continue;

		if (ctx->docking.windows.find(info.id) != ctx->docking.windows.end())
			windowSetVisible(info.id.c_str(), false);
	}

	deferredDeleteObjects();

	// the layout is replaced, drop the transient interaction state that may point
	// to the deleted nodes
	ctx->docking.focusedWindow = nullptr;
	ctx->docking.currentDockNode = nullptr;
	ctx->docking.dragWindow = nullptr;
	ctx->docking.dragStarted = false;
	ctx->docking.resizingNode = nullptr;
	ctx->docking.hoveredNode = nullptr;
	ctx->docking.lastHoveredNode = nullptr;
	ctx->docking.dockToNode = nullptr;
	ctx->docking.dockType = DockType::None;

	// the existing root native windows, in creation order, split by who created them
	std::vector<HNativeWindow> userNativeWindows;
	std::vector<HNativeWindow> floatingNativeWindows;

	for (auto nativeWindow : ctx->nativeWindows)
	{
		auto iter = ctx->docking.rootNativeWindowDockNodes.find(nativeWindow);

		if (iter == ctx->docking.rootNativeWindowDockNodes.end() || !iter->second)
			continue;

		if (iter->second->createdByDockingSystem)
			floatingNativeWindows.push_back(nativeWindow);
		else
			userNativeWindows.push_back(nativeWindow);
	}

	// map every saved native window to an existing one or to a newly created one
	struct NativeWindowTarget
	{
		HNativeWindow nativeWindow = 0;
		DockNode* oldRoot = nullptr;
	};

	std::vector<NativeWindowTarget> targets(nativeWindowCount);
	size_t userIndex = 0;
	size_t floatingIndex = 0;

	for (i32 i = 0; i < nativeWindowCount; i++)
	{
		const auto& info = dockingState.nativeWindows[i];
		auto& target = targets[i];

		if (info.floating)
		{
			if (floatingIndex < floatingNativeWindows.size())
			{
				target.nativeWindow = floatingNativeWindows[floatingIndex++];
				ctx->settings.services.setWindowPosition(target.nativeWindow, info.rect.topLeft());
				ctx->settings.services.setWindowSize(target.nativeWindow, info.rect.getSize());
			}
			else
			{
				// the title is set by windowBegin every frame
				target.nativeWindow = nativeWindowCreate("Window", NativeWindowFlags::Resizable, NativeWindowState::Normal, info.rect);
			}
		}
		else if (userIndex < userNativeWindows.size())
		{
			target.nativeWindow = userNativeWindows[userIndex++];
		}
		else
		{
			target.nativeWindow = nativeWindowCreate("Window", NativeWindowFlags::Resizable, NativeWindowState::Normal, info.rect);
		}

		auto iter = ctx->docking.rootNativeWindowDockNodes.find(target.nativeWindow);

		target.oldRoot = iter != ctx->docking.rootNativeWindowDockNodes.end() ? iter->second : nullptr;
	}

	std::unordered_map<HNativeWindow, DockNode*> rebuiltRoots;

	for (auto& target : targets)
	{
		if (target.oldRoot)
			rebuiltRoots[target.nativeWindow] = target.oldRoot;
	}

	// the windows that are not part of the state are left untouched, but when they
	// share a root with the saved ones, remember the saved windows they were tabbed
	// with so they can be reattached to the closest restored node
	struct PreservedWindow
	{
		Window* window = nullptr;
		HNativeWindow nativeWindow = 0;
		std::vector<std::string> anchors;
	};

	std::vector<PreservedWindow> preservedWindows;

	for (auto& pair : ctx->docking.windows)
	{
		Window* wnd = pair.second;

		if (!wnd || !wnd->dockNode || savedWindowIds.count(wnd->id))
			continue;

		DockNode* node = wnd->dockNode;

		while (node->parent)
			node = node->parent;

		if (!rebuiltRoots.count(node->nativeWindow))
			continue;

		PreservedWindow preserved;
		preserved.window = wnd;
		preserved.nativeWindow = node->nativeWindow;

		for (auto otherWnd : wnd->dockNode->windows)
		{
			if (otherWnd && savedWindowIds.count(otherWnd->id))
				preserved.anchors.push_back(otherWnd->id);
		}

		preservedWindows.push_back(preserved);
	}

	// detach and delete the trees that are about to be rebuilt
	for (auto& pair : rebuiltRoots)
	{
		dockingStateDetachWindows(pair.second);
		dockingStateDeleteNodeTree(pair.second);
		ctx->docking.rootNativeWindowDockNodes.erase(pair.first);
	}

	// the saved windows currently docked in a tree that is not rebuilt must be
	// detached as well, they are about to be reattached to the restored tree
	for (auto& pair : ctx->docking.windows)
	{
		Window* wnd = pair.second;

		if (!wnd || !wnd->dockNode || !savedWindowIds.count(wnd->id))
			continue;

		wnd->dockNode->removeWindow(wnd);
	}

	// build the restored dock node trees
	std::vector<DockNode*> newNodes(nodeCount, nullptr);

	for (i32 i = 0; i < nodeCount; i++)
	{
		const auto& info = dockingState.dockNodes[i];
		DockNode* node = new DockNode();

		node->type = dockingStateToNodeType(info.type);
		node->emptyDockSite = info.type == DockNodeType::None;
		node->rect = info.rect;
		node->selectedTabIndex = info.selectedTabIndex;
		node->nativeWindow = targets[info.nativeWindowIndex].nativeWindow;
		newNodes[i] = node;
		ctx->docking.dockNodeIdsMap[node->id] = node;
	}

	for (i32 i = 0; i < nodeCount; i++)
	{
		const auto& info = dockingState.dockNodes[i];
		DockNode* node = newNodes[i];

		if (info.parentIndex >= 0)
		{
			node->parent = newNodes[info.parentIndex];
			node->parent->children.push_back(node);
		}
		else
		{
			node->createdByDockingSystem = dockingState.nativeWindows[info.nativeWindowIndex].floating;
			ctx->docking.rootNativeWindowDockNodes[node->nativeWindow] = node;
		}
	}

	// dock the saved windows, reusing the existing window objects by id
	std::unordered_map<std::string, DockNode*> savedWindowNodes;

	for (i32 i = 0; i < nodeCount; i++)
	{
		const auto& info = dockingState.dockNodes[i];
		DockNode* node = newNodes[i];

		for (const auto& wndInfo : info.windows)
		{
			Window* wnd = nullptr;
			auto iter = ctx->docking.windows.find(wndInfo.id);

			if (iter != ctx->docking.windows.end())
			{
				wnd = iter->second;
			}
			else
			{
				wnd = new Window();
				wnd->id = wndInfo.id;
				ctx->docking.windows[wndInfo.id] = wnd;
			}

			wnd->title = wndInfo.title;
			wnd->flags = (WindowFlags)wndInfo.flags;
			wnd->dockNode = node;
			node->windows.push_back(wnd);
			savedWindowNodes[wndInfo.id] = node;
			ctx->docking.windowsDockNodeAssignments[wndInfo.id] = node->id;

			// the saved windows were visible when the state was saved
			ctx->docking.hiddenWindows.erase(wndInfo.id);
			ctx->docking.hiddenWindowSlots.erase(wndInfo.id);
		}

		if (node->selectedTabIndex >= node->windows.size() && !node->windows.empty())
			node->selectedTabIndex = node->windows.size() - 1;
	}

	// restore the dock area node, windows are docked only inside it
	if (dockingState.dockAreaNodeIndex >= 0 && dockingState.dockAreaNodeIndex < nodeCount
		&& !dockNodeHasChromeWindow(newNodes[dockingState.dockAreaNodeIndex]))
		ctx->docking.dockAreaNodeId = newNodes[dockingState.dockAreaNodeIndex]->id;
	else
		ctx->docking.dockAreaNodeId = 0;

	// keep the saved closed windows hidden, remembering where they should reopen
	for (const auto& info : dockingState.closedWindows)
	{
		if (info.id.empty() || savedWindowIds.count(info.id))
			continue;

		ctx->docking.hiddenWindows.insert(info.id);
		ctx->docking.closedWindowsRects[info.id] = info.rect;

		if (info.parentIndex >= 0 && info.parentIndex < nodeCount
			&& !dockNodeHasChromeWindow(newNodes[info.parentIndex]))
			ctx->docking.windowsDockNodeAssignments[info.id] = newNodes[info.parentIndex]->id;
		else
			ctx->docking.windowsDockNodeAssignments.erase(info.id);

		if (info.slotParentIndex >= 0 && info.slotParentIndex < nodeCount
			&& !dockNodeHasChromeWindow(newNodes[info.slotParentIndex]))
		{
			DockingState::HiddenWindowSlot slot;

			slot.parentNodeId = newNodes[info.slotParentIndex]->id;
			slot.index = (u32)std::max(info.slotIndex, 0);
			slot.verticalParent = info.slotVertical;
			slot.fraction = info.slotFraction;

			ctx->docking.hiddenWindowSlots[info.id] = slot;
		}
		else
		{
			ctx->docking.hiddenWindowSlots.erase(info.id);
		}
	}

	// reattach the windows that were not part of the state but shared a rebuilt root
	for (auto& preserved : preservedWindows)
	{
		DockNode* destination = nullptr;

		for (const auto& anchor : preserved.anchors)
		{
			auto iter = savedWindowNodes.find(anchor);

			if (iter != savedWindowNodes.end())
			{
				destination = iter->second;
				break;
			}
		}

		if (!destination)
		{
			auto iter = ctx->docking.rootNativeWindowDockNodes.find(preserved.nativeWindow);

			if (iter != ctx->docking.rootNativeWindowDockNodes.end())
				destination = dockingStateFindTabsNode(iter->second);
		}

		if (!destination)
		{
			// no tabs node is left in the restored tree, give the window its own
			// floating native window instead of losing it
			auto nativeWindow = nativeWindowCreate(preserved.window->title, NativeWindowFlags::Resizable, NativeWindowState::Normal, preserved.window->clientRect);
			DockNode* root = dockNodeRootCreateInternal(nativeWindow);

			root->type = DockNode::Type::Tabs;
			root->createdByDockingSystem = true;
			root->windows.push_back(preserved.window);
			preserved.window->dockNode = root;
			ctx->docking.windowsDockNodeAssignments[preserved.window->id] = root->id;

			continue;
		}

		destination->windows.push_back(preserved.window);
		preserved.window->dockNode = destination;
		ctx->docking.windowsDockNodeAssignments[preserved.window->id] = destination->id;
	}

	// drop the window assignments that still point to nodes that were deleted
	for (auto iter = ctx->docking.windowsDockNodeAssignments.begin(); iter != ctx->docking.windowsDockNodeAssignments.end(); )
	{
		if (!ctx->docking.dockNodeIdsMap.count(iter->second))
			iter = ctx->docking.windowsDockNodeAssignments.erase(iter);
		else
			++iter;
	}

	// destroy the floating native windows that were emptied when their windows were
	// reattached to the restored layout
	for (auto iter = ctx->docking.rootNativeWindowDockNodes.begin(); iter != ctx->docking.rootNativeWindowDockNodes.end(); )
	{
		DockNode* root = iter->second;

		if (root && root->createdByDockingSystem && root->windows.empty() && root->children.empty())
		{
			ctx->docking.nativeWindowsToDelete.insert(iter->first);
			ctx->docking.dockNodesToDelete.insert(root);
			iter = ctx->docking.rootNativeWindowDockNodes.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	// keep the chrome windows (menu bar, toolbars) first in their root, so old layouts
	// saved with the chrome at the bottom are repaired on load
	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		DockNode* root = pair.second;

		if (!root || root->children.size() < 2)
			continue;

		std::stable_partition(root->children.begin(), root->children.end(),
			[](DockNode* child) { return dockNodeHasChromeWindow(child); });
	}

	// move the regular windows that ended up at the root level back into the dock area
	{
		DockNode* area = dockNodeGetAreaNode();

		if (area && area->parent)
		{
			DockNode* root = area;

			while (root->parent)
				root = root->parent;

			dockingStateHealRoot(root, area);
		}
	}

	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
		dockNodeRecalculateLayout(pair.second);

	return true;
}

HNativeWindow nativeWindowCreate(const std::string& title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect)
{
	auto wnd = ctx->settings.services.createWindow(title.c_str(), flags, state, rect);

	ctx->nativeWindows.push_back(wnd);

	return wnd;
}

void nativeWindowDestroy(HNativeWindow nativeWnd)
{
	auto iterWnd = ctx->docking.rootNativeWindowDockNodes.find(nativeWnd);

	if (iterWnd != ctx->docking.rootNativeWindowDockNodes.end())
	{
		auto dockNode = iterWnd->second;

		if (dockNode)
			ctx->docking.dockNodesToDelete.insert(dockNode);
	}

	if (nativeWnd)
		ctx->docking.nativeWindowsToDelete.insert(nativeWnd);
}

DockNode* dockNodeRootCreateInternal(HNativeWindow nativeWindow)
{
	auto size = ctx->settings.services.getWindowSize(nativeWindow);
	Rect rect = { 0, 0, size.x, size.y };
	auto dockNode = new DockNode();

	dockNode->type = DockNode::Type::Tabs;
	dockNode->rect.set(0, 0, rect.width, rect.height);
	dockNode->nativeWindow = nativeWindow;
	ctx->docking.rootNativeWindowDockNodes.insert(std::make_pair(nativeWindow, dockNode));

	return dockNode;
}

DockNode* dockNodeRootGet(HNativeWindow nativeWindow)
{
	HUI_ASSERT(nativeWindow);

	if (!nativeWindow) return nullptr;

	auto iterWnd = ctx->docking.rootNativeWindowDockNodes.find(nativeWindow);

	if (iterWnd == ctx->docking.rootNativeWindowDockNodes.end())
		return nullptr;

	return iterWnd->second;
}

void dockNodeRootDelete(HNativeWindow nativeWindow)
{
	HUI_ASSERT(nativeWindow);

	auto iterWnd = ctx->docking.rootNativeWindowDockNodes.find(nativeWindow);

	if (iterWnd == ctx->docking.rootNativeWindowDockNodes.end())
	{
		return;
	}

	auto node = iterWnd->second;

	if (node)
	{
		ctx->docking.nativeWindowsToDelete.insert(nativeWindow);
		ctx->docking.dockNodesToDelete.insert(node);
	}
}

Window* windowCreateInternal(const std::string& id, DockNode* targetNode, DockType dockType, const std::string& title, Rect* initialRect, HNativeWindow nativeWindow, HImage img)
{
	auto targetNodePtr = (DockNode*)targetNode;
	auto newWnd = new Window();
	Rect defaultRect = { 100, 100, 1500, 1300 };
	
	newWnd->title = title;
	newWnd->id = id;
	newWnd->image = img;

	if (!targetNode)
	{
		if (!nativeWindow)
		{
			nativeWindow = nativeWindowCreate(title, NativeWindowFlags::Resizable, NativeWindowState::Normal, initialRect ? *initialRect : defaultRect);
		}
		
		newWnd->dockNode = dockNodeRootCreateInternal(nativeWindow);
		newWnd->dockNode->createdByDockingSystem = true;
		newWnd->dockNode->windows.push_back(newWnd);
		newWnd->clientRect = newWnd->dockNode->rect;
		ctx->docking.focusedWindow = newWnd;
	}

	ctx->docking.windows[id] = newWnd;

	if (targetNode)
	{
		windowDockInternal(newWnd, targetNode, dockType);
	}

	return newWnd;
}

void windowDeleteInternal(Window* wnd)
{
	DockNode* node = wnd->dockNode;

	node->removeWindow(wnd);

	if (!node->parent)
	{
		nativeWindowDestroy(node->nativeWindow);
	}
}

void windowCloseInternal(Window* wnd)
{
	DockNode* node = wnd->dockNode;

	// if the nativeWindow is in a root dock node
	if (!node->parent
		&& node->children.empty()
		&& node->windows.size() == 1)
	{
		nativeWindowDestroy(node->nativeWindow);
		node->nativeWindow = nullptr;
	}
}

void windowHideInternal(Window* wnd)
{
	if (!wnd)
		return;

	DockNode* node = wnd->dockNode;
	Rect rect = node ? node->rect : wnd->clientRect;

	// remember the window in screen coordinates, so it opens at the same place
	// when it is shown again, including the floating native windows
	if (node && node->nativeWindow)
	{
		auto pos = ctx->settings.services.getWindowPosition(node->nativeWindow);
		rect.x += pos.x;
		rect.y += pos.y;
	}

	ctx->docking.hiddenWindows.insert(wnd->id);
	ctx->docking.closedWindowsRects[wnd->id] = rect;

	if (node)
	{
		ctx->docking.windowsDockNodeAssignments[wnd->id] = node->id;

		// if the window is the only one in a child node, that node is removed when
		// the window is hidden, remember the slot so it can be docked back
		if (node->parent)
		{
			DockingState::HiddenWindowSlot slot;
			bool vertical = node->parent->type == DockNode::Type::Vertical;

			slot.parentNodeId = node->parent->id;
			slot.verticalParent = vertical;

			// the size of the node relative to its parent, so the layout is restored
			// with the same proportions
			if (vertical)
				slot.fraction = node->parent->rect.height > 0.0f ? node->rect.height / node->parent->rect.height : 0.5f;
			else
				slot.fraction = node->parent->rect.width > 0.0f ? node->rect.width / node->parent->rect.width : 0.5f;

			auto iter = std::find(node->parent->children.begin(), node->parent->children.end(), node);

			if (iter != node->parent->children.end())
				slot.index = (u32)std::distance(node->parent->children.begin(), iter);

			ctx->docking.hiddenWindowSlots[wnd->id] = slot;
		}
		else
		{
			ctx->docking.hiddenWindowSlots.erase(wnd->id);
		}
	}
}

static void windowHideTree(DockNode* node)
{
	if (!node)
		return;

	for (auto wnd : node->windows)
		windowHideInternal(wnd);

	for (auto child : node->children)
		windowHideTree(child);
}

bool windowDockInternal(Window* wnd, DockNode* targetNode, DockType dockType, u32 tabIndex, const Point* undockedWindowPos)
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

			// if there is just one window in the source node, move the node and remove from current parent
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
		target->emptyDockSite = false;

		// if we only have a nativeWindow in the source node, delete the node
		if (source && source->windows.size() == 1)
		{
			source = source->removeFromParent();
			ctx->docking.dockNodesToDelete.insert(source);
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
				ctx->docking.dockNodesToDelete.insert(source);
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

		auto nativeWnd = nativeWindowCreate(wnd->title, NativeWindowFlags::Resizable, NativeWindowState::Normal, rcWnd);
				
		wnd->dockNode = dockNodeRootCreateInternal(nativeWnd);
		wnd->dockNode->createdByDockingSystem = true;
		wnd->dockNode->windows.push_back(wnd);
		wnd->clientRect = wnd->dockNode->rect;

		break;
	}
	default:
		break;
	}

	windowSetFocus(wnd->id.c_str());

	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		auto node = pair.second;

		if (node && node->nativeWindow)
			dockNodeRecalculateLayout(node);
	}

	return true;
}

void dockNodeTabs(DockNode* node)
{
	if (node->windows.size() && dockNodeShowsTabBar(node))
	{
		paddingPush(PaddingType::Layout, Point());
		layoutBegin(node->rect);
		paddingPop(PaddingType::Layout);
		// pop the clip rect, we dont want clipping since draw tabs bar beyond the node rect width
		auto oldClipRect = ctx->renderer.getClipRect();
		ctx->renderer.popClipRect();

		if (ctx->layout.width <= (node->windows.size() * (ctx->tabGroup.tabWidth + ctx->tabGroup.sideSpacing)) * ctx->scale)
		{
			ctx->tabGroup.forceTabWidth = ctx->layout.width / (f32)node->windows.size();
			ctx->tabGroup.forceSqueezeTabs = true;
		}
		else
		{
			ctx->tabGroup.forceSqueezeTabs = false;
		}

		u32 closeTabIndex = ~0;
		u32 selectedIndex = 0;

		ctx->docking.drawingWindowTabs = true;

		// set clip rect for whole native window
		auto rcIt = ctx->docking.rootNativeWindowDockNodes.find(node->nativeWindow);
		Rect rc = rcIt != ctx->docking.rootNativeWindowDockNodes.end() && rcIt->second ? rcIt->second->rect : node->rect;
		ctx->renderer.pushClipRect(rc, false);
		ctx->docking.currentDockNode = node;
		tabGroupBegin(node->selectedTabIndex);

		for (auto i = 0; i < node->windows.size(); i++)
		{
			if (node->windows[i]->dockingNow)
			{
				ctx->position.x += node->windows[i]->tabRect.width;

				continue;
			}

			if (node->dockingTabSpaceIndex == i)
			{
				ctx->position.x += node->dockingTabSpaceWidth;
			}

			ctx->currentWindow = node->windows[i];
			hui::tab(node->windows[i]->title.c_str(), node->windows[i]->image);
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

		selectedIndex = hui::tabGroupEnd();

		ctx->renderer.popClipRect();
		ctx->docking.drawingWindowTabs = false;

		if (closeTabIndex != ~0)
		{
			windowHideInternal(node->windows[closeTabIndex]);
			ctx->docking.windowsToDelete.insert(node->windows[closeTabIndex]);
			node->windows.erase(node->windows.begin() + closeTabIndex);

			if (node->windows.empty() && node->id != ctx->docking.dockAreaNodeId)
			{
				// this is an empty root dock node, destroy and close OS nativeWindow too
				if (!node->parent && node->createdByDockingSystem)
				{
					ctx->docking.dockNodesToDelete.insert(node);
					ctx->docking.nativeWindowsToDelete.insert(node->nativeWindow);
				}
				else
				{
					node = node->removeFromParent();
					ctx->docking.dockNodesToDelete.insert(node);
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
		ctx->renderer.pushClipRect(oldClipRect, false);
		layoutEnd();
	}

	for (auto& child : node->children)
	{
		dockNodeTabs(child);
	}
}

Rect windowGetClientRect(Window* window)
{
	auto rc = window->dockNode->rect;
	auto tabHeight = ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState().height * ctx->scale;
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

void windowDebugPrint()
{
	printf("------------------------------------------------------------------------------------------------------------\n");
	printf("Debug windows:\n\n");
	printf("%d windows\n", (u32)ctx->docking.rootNativeWindowDockNodes.size());

	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		printInfo(0, pair.second);
	}
}

void handleDockingMouseDown(const InputEvent& event, DockNode* node)
{
	if (event.mouse.button != MouseButton::Left)
		return;

	auto& ds = ctx->docking;
	const Point& mousePos = ctx->mousePosition;

	ds.lastMousePosSinceMouseDown = mousePos;
	ds.lastMousePos = mousePos;
	ds.resizingNode = node->findResizeDockNode(mousePos);
	ds.dragWindow = nullptr;
	ds.dragStarted = false;

	// a splitter drag must not also start a tab/window undock
	if (ds.resizingNode)
		return;

	//TODO: we could check just the tabs of the current os window clicked on
	for (auto& wnd : ds.windows)
	{
		if (!wnd.second || !wnd.second->dockNode)
			continue;

		if (wnd.second->dockNode->nativeWindow != event.window)
			continue;

		if (wnd.second->clientRect.contains(mousePos) && wnd.second->dockNode->selectedTabIndex == wnd.second->dockNode->getWindowIndex(wnd.second))
		{
			windowSetFocus(wnd.second->id.c_str());
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
			windowSetFocus(wnd.second->id.c_str());
			ds.dragWindowMouseDelta = mousePos - ds.dragWindow->tabRect.topLeft();
			break;
		}
	}
}

bool dockNodeHasChromeWindow(DockNode* node)
{
	if (!node)
		return false;
	for (auto wnd : node->windows)
	{
		if (wnd && !!(wnd->flags & WindowFlags::NoTab) && !!(wnd->flags & WindowFlags::NoResize))
			return true;
	}

	for (auto child : node->children)
	{
		if (dockNodeHasChromeWindow(child))
			return true;
	}

	return false;
}

// returns the dock area node, or nullptr when docking is not restricted
static DockNode* dockNodeGetAreaNode()
{
	DockNodeId areaId = ctx->docking.dockAreaNodeId;

	if (areaId)
	{
		auto iter = ctx->docking.dockNodeIdsMap.find(areaId);

		// the area must be valid and must not be one of the chrome nodes
		if (iter != ctx->docking.dockNodeIdsMap.end() && iter->second
			&& !dockNodeHasChromeWindow(iter->second))
			return iter->second;
	}

	// the dock area id can go stale when its node is rebuilt, recover it as the largest
	// root child that does not contain chrome windows (menu bar, toolbars)
	bool hasChrome = false;

	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		if (dockNodeHasChromeWindow(pair.second))
		{
			hasChrome = true;
			break;
		}
	}

	if (!hasChrome)
		return nullptr;

	DockNode* bestArea = nullptr;
	f32 bestSize = 0.0f;

	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		DockNode* root = pair.second;

		if (!root)
			continue;

		for (auto child : root->children)
		{
			if (dockNodeHasChromeWindow(child))
				continue;

			f32 size = child->rect.width * child->rect.height;

			if (size > bestSize)
			{
				bestSize = size;
				bestArea = child;
			}
		}
	}

	if (bestArea)
		ctx->docking.dockAreaNodeId = bestArea->id;

	return bestArea;
}

// returns the node a newly created window should dock into by default, nullptr when
// there is no dock area
DockNode* dockNodeGetDefaultDockTarget()
{
	DockNode* area = dockNodeGetAreaNode();

	if (!area)
		return nullptr;

	if (area->type == DockNode::Type::Tabs || area->type == DockNode::Type::None)
		return area;

	DockNode* tabsNode = dockingStateFindTabsNode(area);

	return tabsNode ? tabsNode : area;
}

static bool dockNodeIsInDockArea(DockNode* node)
{
	DockNode* area = dockNodeGetAreaNode();

	if (!area)
		return true;

	DockNode* nodeRoot = node;

	while (nodeRoot->parent)
		nodeRoot = nodeRoot->parent;

	DockNode* areaRoot = area;

	while (areaRoot->parent)
		areaRoot = areaRoot->parent;

	// the restriction applies only inside the native window that owns the dock area,
	// floating windows without chrome can dock anywhere in themselves
	if (nodeRoot != areaRoot)
		return !dockNodeHasChromeWindow(nodeRoot);

	for (DockNode* n = node; n; n = n->parent)
	{
		if (n == area)
			return true;
	}

	return false;
}

// docks a window at the edge of the dock area, keeping the resulting nodes inside the
// area, so the chrome windows outside of it are never displaced
static void dockWindowToAreaEdge(Window* wnd, DockNode* area, DockType dockType, u32 tabIndex, const Point* undockedWindowPos)
{
	if (!wnd || !area)
		return;

	// the window already fills the whole area, nothing to do
	if (wnd->dockNode == area && area->children.empty() && area->windows.size() == 1 && area->windows[0] == wnd)
		return;

	bool vertical = dockType == DockType::Top || dockType == DockType::Bottom;
	DockNode::Type requiredType = vertical ? DockNode::Type::Vertical : DockNode::Type::Horizontal;

	// make the area split along the needed axis, its current content becomes a child
	if (area->type != requiredType)
	{
		std::vector<Window*> windows = area->windows;
		std::vector<DockNode*> children = area->children;
		DockNode::Type oldType = area->type;

		area->windows.clear();
		area->children.clear();
		area->type = requiredType;

		if (!windows.empty() || !children.empty())
		{
			DockNode* content = new DockNode();

			if (!children.empty()
				&& (oldType == DockNode::Type::Vertical || oldType == DockNode::Type::Horizontal))
				content->type = oldType;
			else
				content->type = DockNode::Type::Tabs;

			content->rect = area->rect;
			content->parent = area;
			content->nativeWindow = area->nativeWindow;
			content->windows = windows;
			content->children = children;

			for (auto w : content->windows)
				w->dockNode = content;

			for (auto c : content->children)
				c->parent = content;

			ctx->docking.dockNodeIdsMap[content->id] = content;
			area->children.push_back(content);
		}
	}

	// an empty area simply takes the window
	if (area->children.empty())
	{
		windowDockInternal(wnd, area, DockType::AsTab, tabIndex, undockedWindowPos);
		return;
	}

	DockNode* target = (dockType == DockType::Top || dockType == DockType::Left)
		? area->children.front()
		: area->children.back();

	windowDockInternal(wnd, target, dockType, tabIndex, undockedWindowPos);
}

void handleDockingMouseUp()
{
	auto& ds = ctx->docking;

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
				pos = ctx->settings.services.getAbsoluteMousePosition();
				// put the window in the middle of the mouse coordinates
				pos.x -= rc.width / 2.0f;
				pos.y -= rc.height / 2.0f;
				wndPos = &pos;
			}

			DockNode* areaNode = dockNodeGetAreaNode();

			// never dock outside the dock area, if the hit test produced a node outside
			// of it (root, chrome), dock at the matching edge of the area instead
			bool areaEdgeDock = areaNode
				&& ds.dockToNode
				&& ds.dockType != DockType::AsTab
				&& ds.dockType != DockType::Floating
				&& (ds.dockToNode == areaNode || !dockNodeIsInDockArea(ds.dockToNode));

			if (areaEdgeDock)
				dockWindowToAreaEdge(ds.dragWindow, areaNode, ds.dockType, tabIndex, wndPos);
			else
				windowDockInternal(ds.dragWindow, ds.dockToNode, ds.dockType, tabIndex, wndPos);
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
		ctx->settings.services.destroyWindow(ds.dragIndicatorNativeWindow);
		ds.dragIndicatorNativeWindow = nullptr;
	}

	ds.dockToNode = nullptr;
	ds.dragWindow = nullptr;
	ds.resizingNode = nullptr;
	ds.dragStarted = false;
}

void handleDockNodeResize(DockNode* node)
{
	auto& ds = ctx->docking;
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

		// recompute the parent so the fixed size windows keep their size after the resize
		if (ds.resizingNode->parent)
			ds.resizingNode->parent->computeRect();
		else
			ds.resizingNode->computeRect();
	}
}

void handleDockingMouseMove(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->docking;

	if (ds.dragWindow && ds.dragStarted)
	{
		ds.dragWindow->dockingNow = true;

		auto& tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
		const Point mousePos = ctx->mousePosition;

		if (ctx->lastHoveredNativeWindow == node->nativeWindow)
			ds.hoveredNode = node->findTargetDockNode(mousePos);

		// windows can only be docked inside the dock area, the windows outside of it
		// (menu bar, toolbars) are not dock targets, but dropping on them docks next to
		// the dock area in the chrome direction
		DockNode* chromeNode = nullptr;

		if (ds.hoveredNode && !dockNodeIsInDockArea(ds.hoveredNode))
		{
			chromeNode = ds.hoveredNode;
			ds.hoveredNode = nullptr;
			ds.dockType = DockType::Floating;
			ds.isHitBoxLeftHovered = false;
			ds.isHitBoxRightHovered = false;
			ds.isHitBoxTopHovered = false;
			ds.isHitBoxBottomHovered = false;
			ds.isHitBoxTabsHovered = false;
			ds.isHitBoxTabsBarHovered = false;
			ds.hitBoxTabs = {};
		}

		bool isSameNode = ds.hoveredNode == ds.dragWindow->dockNode;
		bool isSingleWindow = ds.dragWindow->dockNode && ds.dragWindow->dockNode->windows.size() == 1;

		auto rootNode = node;
		auto& rootRect = rootNode->rect;
		auto parentRect = ds.hoveredNode ? ds.hoveredNode->rect : rootRect;

		// when a dock area is set, the window edges dock relative to the dock area, so
		// the chrome windows (menu bar, toolbars) stay full width at the top
		DockNode* areaNode = dockNodeGetAreaNode();
		DockNode* edgeTarget = rootNode;
		Rect edgeRect = rootRect;
		bool edgesEnabled = true;
		bool areaEdges = false;

		if (areaNode)
		{
			bool areaInRoot = false;

			for (DockNode* n = areaNode; n; n = n->parent)
			{
				if (n == rootNode)
				{
					areaInRoot = true;
					break;
				}
			}

			if (areaInRoot)
			{
				edgeTarget = areaNode;
				edgeRect = areaNode->rect;
				areaEdges = true;
			}
			else if (!dockNodeHasChromeWindow(rootNode))
			{
				// floating windows without chrome dock relative to their own root
				edgeTarget = rootNode;
				edgeRect = rootRect;
			}
			else
			{
				edgesEnabled = false;
			}
		}

		// the chrome exists but no content area can be found, do not allow docking,
		// it would place windows around the root and displace the chrome
		if (!areaNode && dockNodeHasChromeWindow(rootNode))
		{
			ds.hoveredNode = nullptr;
			ds.dockType = DockType::Floating;
			ds.hitBoxTabs = {};
			edgesEnabled = false;
		}

		// docking is restricted to another window's dock area, nothing can be docked
		// around this root
		if (!edgesEnabled)
		{
			ds.hitBoxRootLeft = {};
			ds.hitBoxRootRight = {};
			ds.hitBoxRootTop = {};
			ds.hitBoxRootBottom = {};
			ds.isHitBoxRootLeftHovered = false;
			ds.isHitBoxRootRightHovered = false;
			ds.isHitBoxRootTopHovered = false;
			ds.isHitBoxRootBottomHovered = false;
		}

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
				ds.hitBoxTabs.height = tabGroupElem.normalState().height * 2.0f * ctx->scale;

				f32 rootMargin = areaEdges ? 0.0f : 40.0f;

				ds.hitBoxRootLeft = edgeRect;
				ds.hitBoxRootRight = edgeRect;
				ds.hitBoxRootTop = edgeRect;
				ds.hitBoxRootBottom = edgeRect;

				ds.hitBoxRootLeft.width = ctx->settings.dockNodeRootDockingHitSize;

				ds.hitBoxRootRight.x = edgeRect.right() - ctx->settings.dockNodeRootDockingHitSize;
				ds.hitBoxRootRight.width = ctx->settings.dockNodeRootDockingHitSize;

				ds.hitBoxRootTop.height = ctx->settings.dockNodeRootDockingHitSize;
				ds.hitBoxRootTop.y -= rootMargin;

				ds.hitBoxRootBottom.y = edgeRect.bottom() - ctx->settings.dockNodeRootDockingHitSize;
				ds.hitBoxRootBottom.height = ctx->settings.dockNodeRootDockingHitSize + rootMargin;
			}
			else
			{
				// use width of the images to compute hit box
				// the guide images are considered to be square
				auto boxSizeV = ctx->theme->getElement(WidgetElementId::WindowDockGuideVerticalSplit).normalState().image->width * ctx->scale * ctx->settings.dockIndicatorBoxScale;
				auto boxSizeH = ctx->theme->getElement(WidgetElementId::WindowDockGuideHorizontalSplit).normalState().image->width * ctx->scale * ctx->settings.dockIndicatorBoxScale;
				auto boxSizeT = ctx->theme->getElement(WidgetElementId::WindowDockGuideAsTab).normalState().image->width * ctx->scale * ctx->settings.dockIndicatorBoxScale;

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
					tabGroupElem.normalState().height * ctx->scale);

				ds.hitBoxRootLeft = Rect(
					edgeRect.x + boxGap,
					edgeRect.y + edgeRect.height / 2.0f - boxSizeV / 2.0f,
					boxSizeV, boxSizeV);

				ds.hitBoxRootRight = Rect(
					edgeRect.right() - boxGap - boxSizeV,
					edgeRect.y + edgeRect.height / 2.0f - boxSizeV / 2.0f,
					boxSizeV, boxSizeV);

				ds.hitBoxRootTop = Rect(
					edgeRect.x + edgeRect.width / 2.0f - boxSizeH / 2.0f,
					edgeRect.y + boxGap,
					boxSizeH, boxSizeH);

				ds.hitBoxRootBottom = Rect(
					edgeRect.x + edgeRect.width / 2.0f - boxSizeH / 2.0f,
					edgeRect.bottom() - boxSizeH / 2.0f - boxGap,
					boxSizeH, boxSizeH);
			}

			ds.isHitBoxLeftHovered = ds.hitBoxLeft.contains(mousePos);
			ds.isHitBoxRightHovered = ds.hitBoxRight.contains(mousePos);
			ds.isHitBoxTopHovered = ds.hitBoxTop.contains(mousePos);
			ds.isHitBoxBottomHovered = ds.hitBoxBottom.contains(mousePos);
			ds.isHitBoxTabsHovered = ds.hitBoxTabs.contains(mousePos);
			ds.isHitBoxTabsBarHovered = ds.hitBoxTabsBar.contains(mousePos);

			// a node without a tab bar cannot accept a window docked as a tab
			if (!dockNodeShowsTabBar(ds.hoveredNode))
			{
				ds.isHitBoxTabsHovered = false;
				ds.isHitBoxTabsBarHovered = false;
			}

			// when docking is restricted to another window's dock area, windows cannot
			// be docked around this root
			if (!edgesEnabled)
			{
				ds.hitBoxRootLeft = {};
				ds.hitBoxRootRight = {};
				ds.hitBoxRootTop = {};
				ds.hitBoxRootBottom = {};
			}

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
				ds.dockToNode = edgeTarget;
				ds.dockType = DockType::Left;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootRightHovered)
			{
				ds.dockToNode = edgeTarget;
				ds.dockType = DockType::Right;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.x += ds.dragRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio);
				ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootTopHovered)
			{
				ds.dockToNode = edgeTarget;
				ds.dockType = DockType::Top;
				ds.dragRect = ds.dockToNode->rect;
				ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
			}

			if (ds.isHitBoxRootBottomHovered)
			{
				ds.dockToNode = edgeTarget;
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
					ds.dragRect.height = tabGroupElem.normalState().height * ctx->scale;
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

		// dropping on a chrome window docks next to the dock area, in the direction of
		// the chrome, so a full width row right below the menu can be created
		if (chromeNode && areaNode && edgesEnabled)
		{
			const Rect& areaRect = areaNode->rect;
			Point chromeCenter = chromeNode->rect.center();
			DockType chromeDockType = DockType::None;

			if (chromeCenter.y <= areaRect.y)
				chromeDockType = DockType::Top;
			else if (chromeCenter.y >= areaRect.bottom())
				chromeDockType = DockType::Bottom;
			else if (chromeCenter.x <= areaRect.x)
				chromeDockType = DockType::Left;
			else if (chromeCenter.x >= areaRect.right())
				chromeDockType = DockType::Right;

			if (chromeDockType != DockType::None)
			{
				ds.dockToNode = areaNode;
				ds.dockType = chromeDockType;
				ds.dragRect = areaRect;

				switch (chromeDockType)
				{
				case DockType::Left:
					ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
					break;

				case DockType::Right:
					ds.dragRect.x += ds.dragRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio);
					ds.dragRect.width *= ctx->settings.dockNodeDockingSizeRatio;
					break;

				case DockType::Top:
					ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
					break;

				case DockType::Bottom:
					ds.dragRect.y += floorf(ds.dragRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio));
					ds.dragRect.height *= ctx->settings.dockNodeDockingSizeRatio;
					break;

				default:
					break;
				}
			}
		}
	}

	handleDockNodeResize(node);
}

void handleDockNodeEvents(DockNode* node)
{
	HUI_ASSERT(node);
	auto& rect = node->rect;
	auto& event = hui::inputEventGet();

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

		handleDockingMouseMove(event, node);
	};
}

void drawDockGuides()
{
	auto& ds = ctx->docking;

	auto& dockGuideVerticalSplitNormalElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideVerticalSplit).normalState();
	auto& dockGuideHorizontalSplitNormalElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideHorizontalSplit).normalState();
	auto& dockGuideAsTabNormalElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideAsTab).normalState();

	auto& dockGuideVerticalSplitHoveredElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideVerticalSplit).hoveredState();
	auto& dockGuideHorizontalSplitHoveredElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideHorizontalSplit).hoveredState();
	auto& dockGuideAsTabHoveredElem = ctx->theme->getElement(WidgetElementId::WindowDockGuideAsTab).hoveredState();

	// draw the guides
	ctx->renderer.cmdSetColor(ds.isHitBoxLeftHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxLeftHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.hitBoxLeft);

	ctx->renderer.cmdSetColor(ds.isHitBoxRightHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxRightHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.hitBoxRight);

	ctx->renderer.cmdSetColor(ds.isHitBoxTopHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxTopHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxTop);

	ctx->renderer.cmdSetColor(ds.isHitBoxBottomHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxBottomHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxBottom);

	// root node guides
	ctx->renderer.cmdSetColor(ds.isHitBoxRootLeftHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer.cmdDrawImageBordered(
		ds.isHitBoxRootLeftHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.isHitBoxRootLeftHovered ?
		dockGuideVerticalSplitHoveredElem.border : dockGuideVerticalSplitNormalElem.border,
		ds.hitBoxRootLeft, ctx->scale);

	ctx->renderer.cmdSetColor(ds.isHitBoxRootRightHovered ? dockGuideVerticalSplitHoveredElem.color : dockGuideVerticalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxRootRightHovered ?
		dockGuideVerticalSplitHoveredElem.image : dockGuideVerticalSplitNormalElem.image,
		ds.hitBoxRootRight);

	ctx->renderer.cmdSetColor(ds.isHitBoxRootTopHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxRootTopHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxRootTop);

	ctx->renderer.cmdSetColor(ds.isHitBoxRootBottomHovered ? dockGuideHorizontalSplitHoveredElem.color : dockGuideHorizontalSplitNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxRootBottomHovered ?
		dockGuideHorizontalSplitHoveredElem.image : dockGuideHorizontalSplitNormalElem.image,
		ds.hitBoxRootBottom);

	ctx->renderer.cmdSetColor(ds.isHitBoxTabsHovered ? dockGuideAsTabHoveredElem.color : dockGuideAsTabNormalElem.color);
	ctx->renderer.cmdDrawImage(
		ds.isHitBoxTabsHovered ?
		dockGuideAsTabHoveredElem.image : dockGuideAsTabNormalElem.image,
		ds.hitBoxTabs);
}

void drawDockPreview(Window* window, const Rect& windowRect)
{
	auto& ds = ctx->docking;

	ctx->renderer.pushClipRect(windowRect, false);

	auto& windowElem = ctx->theme->getElement(WidgetElementId::WindowBody).normalState();

	Color tintColor;

	if (ctx->settings.dockingStyle == DockingGuidesStyle::NativeWindows)
	{
		auto tintColorStr = hui::themeGetUserSetting(ctx->theme, "dockPreviewNativeWindowsColorTint");

		if (tintColorStr && strcmp(tintColorStr, ""))
			tintColor = colorFromText(tintColorStr);
	}
	else
	{
		auto tintColorStr = hui::themeGetUserSetting(ctx->theme, "dockPreviewInsideWindowsColorTint");

		if (tintColorStr && strcmp(tintColorStr, ""))
			tintColor = colorFromText(tintColorStr);
	}

	ctx->renderer.cmdSetColor(windowElem.color * tintColor);
	ctx->renderer.cmdDrawImageBordered(windowElem.image, windowElem.border, windowRect, ctx->scale);
	paddingPush(PaddingType::Layout, Point());
	layoutBegin(windowRect);
	paddingPop(PaddingType::Layout);
	tabGroupBegin(0);
	hui::tab(window->title.c_str(), window->image);
	tabGroupEnd();
	layoutEnd();
	ctx->renderer.popClipRect();
}

void updateDockingSystem()
{
	if (ctx->docking.rootNativeWindowDockNodes.empty())
		return;

	auto copyOfRootNativeWindowDockNodes = ctx->docking.rootNativeWindowDockNodes;
	auto& ds = ctx->docking;
	const auto& mousePos = ctx->mousePosition;

	if (ds.dragWindow)
	{
		ds.dragStarted = (ds.lastMousePosSinceMouseDown - mousePos).getLength() > ctx->settings.dragStartDistance;
	}

	ds.mouseDragDelta = mousePos - ds.lastMousePos;

	auto screenMousePos = ctx->settings.services.getAbsoluteMousePosition();
	Rect screenRect;

	// remember last valid hovered node to remove insertion space when changed
	ds.lastHoveredNode = ds.hoveredNode ? ds.hoveredNode : ds.lastHoveredNode;
	ds.dockToNode = nullptr;
	ds.hoveredNode = nullptr;
	ds.dockType = DockType::Floating;

	for (auto& wnd : copyOfRootNativeWindowDockNodes)
	{
		if (wnd.second)
			handleDockNodeEvents(wnd.second);
	}

	if (ctx->event.type == InputEvent::Type::WindowResized || ctx->event.type == InputEvent::Type::WindowMoved)
	{
		for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
		{
			// we only check dock nodes that are not scheduled for deletion (those have nativeWindow null)
			if (pair.second && pair.second->nativeWindow)
				pair.second->computeRect();
		}
	}

	if (ds.dragWindow 
		&& ds.dragStarted
		&& ctx->settings.dockingStyle == DockingGuidesStyle::NativeWindows
		&& !ds.dragIndicatorNativeWindow
		&& ctx->lastHoveredNativeWindow)
	{
		Point wndPos = ctx->settings.services.getWindowPosition(ctx->lastHoveredNativeWindow);

		screenRect = ds.dragRect + wndPos;

		ds.dragIndicatorNativeWindow = ctx->settings.services.createWindow(ds.dragWindow->title.c_str(), NativeWindowFlags::NoInput | NativeWindowFlags::NoDecoration | NativeWindowFlags::Resizable, NativeWindowState::Normal, screenRect);

		ds.dragWindow->dockingNow = true;
	}

	// if we release the mouse button, wherever it is, over a window or not
	// then force a mouse up button, because we dont do capturing so we cant detect it when not in a window
	if (!ctx->settings.services.isMouseButtonDownNow(MouseButton::Left)
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
		// the dock target can be the dock area even when the mouse is over the chrome
		DockNode* previewTarget = ds.dockToNode ? ds.dockToNode : ds.hoveredNode;

		// if we try to dock on dock nodes sides or tabs area
		if (ctx->lastHoveredNativeWindow
			&& ds.dockType != DockType::None
			&& ds.dockType != DockType::Floating
			&& previewTarget)
		{
			auto pos = ctx->settings.services.getWindowPosition(previewTarget->nativeWindow);

			screenRect = ds.dragRect;
			screenRect += pos;
		}
		else
		{
			// resize window as floating window
			auto mousePosAbs = ctx->settings.services.getAbsoluteMousePosition();
			screenRect = ds.dragWindow->dockNode->rect;
			screenRect *= 0.6f; // scale back a bit from original size
			screenRect.x = mousePosAbs.x - screenRect.width / 2;
			screenRect.y = mousePosAbs.y - screenRect.height / 2;
		}

		ctx->settings.services.setWindowPosition(ds.dragIndicatorNativeWindow, screenRect.topLeft());
		ctx->settings.services.setWindowSize(ds.dragIndicatorNativeWindow, screenRect.getSize());
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
			if (ctx->docking.dragIndicatorNativeWindow)
			{
				ctx->settings.services.setCurrentWindow(ds.dragIndicatorNativeWindow);
				ctx->renderer.disableRendering = false;
				ctx->renderer.nativeWindowSetCurrent(ds.dragIndicatorNativeWindow);
				ctx->renderer.setWindowSize(screenRect.getSize());
				ctx->renderer.begin();
				// the rect is in screen coords, just make it relative to our dragged indicator window
				screenRect.x = screenRect.y = 0;
				drawDockPreview(ds.dragWindow, screenRect);
				ctx->renderer.end();
				ctx->renderer.executeDrawCommands(ds.dragIndicatorNativeWindow);
				ctx->settings.services.presentWindow(ds.dragIndicatorNativeWindow);
			}
		}
		else
		{
			DockNode* previewTarget = ds.dockToNode ? ds.dockToNode : ds.hoveredNode;

			if (previewTarget)
			{
				auto rcIt = ds.rootNativeWindowDockNodes.find(previewTarget->nativeWindow);

				if (rcIt != ds.rootNativeWindowDockNodes.end() && rcIt->second)
				{
					auto& rc = rcIt->second->rect;

					ctx->settings.services.setCurrentWindow(previewTarget->nativeWindow);
					ctx->renderer.disableRendering = false;
					ctx->renderer.nativeWindowSetCurrent(previewTarget->nativeWindow);
					ctx->renderer.setWindowSize(rc.getSize());
					ctx->renderer.begin();
					ctx->renderer.pushWindowDrawCmdLayer(DrawCmdLayerType::Overlay);
					drawDockPreview(ds.dragWindow, ds.dragRect);
					drawDockGuides();
					ctx->renderer.popWindowDrawCmdLayer();
					ctx->renderer.executeDrawCommands(previewTarget->nativeWindow);
				}
			}
		}
	}

	if (ctx->event.type == InputEvent::Type::WindowClose)
	{
		auto nodeIt = ctx->docking.rootNativeWindowDockNodes.find(ctx->event.window);

		if (nodeIt != ctx->docking.rootNativeWindowDockNodes.end() && nodeIt->second)
		{
			// remember all the windows of the closed native window so they stay
			// closed and reopen at the same place when shown again
			windowHideTree(nodeIt->second);
			nodeIt->second->removeWindowsAndDeleteChildrenRecursive();
		}

		ctx->docking.nativeWindowsToDelete.insert(ctx->event.window);
	}

	ds.lastMousePos = mousePos;
	ds.lastHoveredNode = ds.hoveredNode;
}

}
