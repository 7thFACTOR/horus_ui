#include "dock_node.h"
#include <string.h>
#include <algorithm>
#include "context.h"

namespace hui
{
void DockNode::deleteWindowsAndChildrenRecursive()
{
	for (auto& wnd : windows)
	{
		auto iter = ctx->dockingState.windows.find(wnd->id);
		if (iter != ctx->dockingState.windows.end())
			ctx->dockingState.windows.erase(iter);
		delete wnd;
	}

	windows.clear();

	for (auto& child : children)
	{
		child->deleteWindowsAndChildrenRecursive();
		delete child;
	}

	children.clear();
}

void DockNode::removeFromParent()
{
	if (parent)
	{
		auto iter = std::find(parent->children.begin(), parent->children.end(), this);
		
		if (iter != parent->children.end())
		{
			parent->children.erase(iter);
			parent->computeRect();
			parent = nullptr;
		}
	}
	else
	{
		// this is a root node and removing it we must destroy the window too
		ctx->dockingState.rootOsWindowDockNodes.erase(osWindow);
		auto iter = std::find(ctx->osWindows.begin(), ctx->osWindows.end(), osWindow);
		if (iter != ctx->osWindows.end())
			ctx->osWindows.erase(iter);
		HORUS_INPUT->destroyWindow(osWindow);
		osWindow = 0;
	}
}

void DockNode::removeWindow(Window* window)
{
	auto iter = std::find(windows.begin(), windows.end(), window);

	if (iter != windows.end())
	{
		(*iter)->dockNode = nullptr;
		windows.erase(iter);
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
		auto size = HORUS_INPUT->getWindowClientSize(osWindow);
		rect = { 0, 0, size.x, size.y };
	}

	switch (type)
	{
	case hui::DockNode::Type::None:
	case hui::DockNode::Type::Tabs:
		for (auto& wnd : windows)
		{
			wnd->clientRect = {
				rect.x, rect.y, rect.width, rect.height
			};
		}
		break;
	case hui::DockNode::Type::Vertical:
	{
		f32 availableSpace = rect.height - ctx->settings.dockNodeSpacing * (f32)(children.size() - 1);
		f32 averageSpace = availableSpace / (f32)children.size();
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
	case hui::DockNode::Type::Horizontal:
	{
		f32 availableSpace = rect.width - ctx->settings.dockNodeSpacing * (f32)(children.size() - 1);
		f32 averageSpace = availableSpace / (f32)children.size();
		f32 totalSpace = 0;

		for (auto& child : children)
		{
			if (child->rect.width <= 0.0f)
			{
				child->rect.width = averageSpace;
			}

			totalSpace += child->rect.width;
		}

		f32 currentX = rect.x;

		for (auto& child : children)
		{
			f32 width = child->rect.width / totalSpace * availableSpace;

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

	for (auto child : children)
	{
		child->computeMinSize();
	};

	switch (type)
	{
	case hui::DockNode::Type::None:
		break;
	case hui::DockNode::Type::Tabs:
		break;
	case hui::DockNode::Type::Vertical:
	{
		f32 total = 0;
		for (auto child : children)
		{
			total += child->minSize.y;
		};

		if (total > minSize.y) minSize.y = total;

		break;
	}
	case hui::DockNode::Type::Horizontal:
	{
		f32 total = 0;
		for (auto child : children)
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
	// first do leaves for redundant nesting of dock nodes
	// collapse starting at the leaf nodes
	for (auto& c : children)
	{
		c->checkRedundancy();
	}

	// if we have just 1 child, delete it, move its contents to us
	if (children.size() == 1)
	{
		auto child = children[0];
		children = child->children;
		for (auto& c : children) c->parent = this;
		windows = child->windows;
		for (auto& w : windows) w->dockNode = this;
		selectedTabIndex = child->selectedTabIndex;
		type = child->type;
		delete child;
		computeRect();
	}

	if (parent)
	{
		bool deleteThis = false;

		if (parent->type == type)
		{
			auto iterPosThis = std::find(parent->children.begin(), parent->children.end(), this);
			parent->children.insert(iterPosThis, children.begin(), children.end());
			// find it again, remove it
			iterPosThis = std::find(parent->children.begin(), parent->children.end(), this);
			parent->children.erase(iterPosThis);
			for (auto& c : children) c->parent = parent;
			for (auto& w : windows) w->dockNode = parent;
			deleteThis = true;
		}

		if (deleteThis) delete this;
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

	printf("%s%s rect(%d,%d,%d,%d) tabIdx:%d osWnd:%p\n", tabs.c_str(), name.c_str(), (i32)rect.x, (i32)rect.y, (i32)rect.width, (i32)rect.height, selectedTabIndex, osWindow);

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

		for (auto cell : children)
		{
			auto foundCell = cell->findResizeDockNode(pt);

			if (foundCell)
			{
				return foundCell;
			}
		}
	}

	return nullptr;
}

DockNode* DockNode::findTargetDockNode(const Point& pt)
{
	if (type == Type::None || type == Type::Tabs)
	{
		if (rect.contains(pt))
			return this;
	}
	else
	{
		for (auto child : children)
		{
			auto foundCell = child->findTargetDockNode(pt);

			if (foundCell)
			{
				return foundCell;
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

	for (auto& c : children)
	{
		auto node = c->findDockNode(pt);
		if (node) return node;
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

}
