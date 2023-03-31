#include "view.h"
#include <string.h>
#include <algorithm>
#include "context.h"

namespace hui
{
void DockNode::removeFromParent()
{
	if (parent)
	{
		auto iter = std::find(parent->children.begin(), parent->children.end(), this);
		
		if (iter != parent->children.end())
		{
			parent->children.erase(iter);
			parent->checkRedundancy();
			parent = nullptr;
		}
	}
	else
	{
		// this is a root node and removing it we must destroy the window too
		ctx->dockingState.rootWindowDockNodes.erase(window);
		destroyWindow(window);
		window = 0;
	}
}

void DockNode::removeView(View* view)
{
	auto iter = std::find(views.begin(), views.end(), view);

	if (iter != views.end())
	{
		views.erase(iter);
		(*iter)->dockNode = nullptr;
	}

	checkRedundancy();
}

void DockNode::gatherViewTabsNodes(std::vector<DockNode*>& outNodes)
{
	if (type == Type::ViewTabs)
	{
		outNodes.push_back(this);
		return;
	}

	for (auto& c : children)
	{
		if (c->type == DockNode::Type::ViewTabs)
		{
			outNodes.push_back(c);
		}
		else
		{
			c->gatherViewTabsNodes(outNodes);
		}
	}
}

void DockNode::computeRect()
{
	if (!parent)
	{
		rect = getWindowClientRect(window);
	}

	switch (type)
	{
	case hui::DockNode::Type::None:
	case hui::DockNode::Type::ViewTabs:
		break;
	case hui::DockNode::Type::Vertical:
	{
		f32 availableSpace = rect.height + ctx->settings.dockNodeSpacing * (f32)(children.size() - 1);
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

			// recursive, could be done with a vector and a while loop though
			child->computeRect();

			currentY += height;
			currentY += ctx->settings.dockNodeSpacing;
		}
	}
		break;
	case hui::DockNode::Type::Horizontal:
	{
		f32 availableSpace = rect.width + ctx->settings.dockNodeSpacing * (f32)(children.size() - 1);
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

bool DockNode::checkRedundancy()
{
	// if we have just 1 child, delete it, move its contents to us
	if (children.size() == 1)
	{
		auto& child = children[0];
		children = child->children;
		for (auto& c : children) c->parent = this;
		views = child->views;
		for (auto& v : views) v->dockNode = this;
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
			deleteThis = true;
		}

		parent->checkRedundancy();
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
	case Type::ViewTabs: name = "ViewTabs"; break;
	default: break;
	}

	printf("%s%s rect(%d,%d,%d,%d) tabIdx:%d wnd:%p\n", tabs.c_str(), name.c_str(), (i32)rect.x, (i32)rect.y, (i32)rect.width, (i32)rect.height, selectedTabIndex, window);

	if (!views.empty())
	{
		printf("%s\tViews:\n", tabs.c_str());
		for (auto& v : views)
			printf("%s\t\t%s userdata:%p type:%d icon:%p\n", tabs.c_str(), v->title.c_str(), (void*)v->userData, v->viewType, (void*)v->icon);
	}

	for (auto& c : children) c->debug(level + 1);
}

DockNode* DockNode::findResizeNode(const Point& pt)
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
					if (pt.x >= child->rect.right() - ctx->settings.dockNodeSpacing / 2
						&& pt.x <= child->rect.right() + ctx->settings.dockNodeSpacing / 2)
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
					if (pt.y >= child->rect.bottom() - ctx->settings.dockNodeSpacing / 2
						&& pt.y <= child->rect.bottom() + ctx->settings.dockNodeSpacing / 2)
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
			auto foundCell = cell->findResizeNode(pt);

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
	if (type == Type::None || type == Type::ViewTabs)
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

size_t DockNode::getViewIndex(View* view)
{
	auto iter = std::find(views.begin(), views.end(), view);
	
	if (iter == views.end()) return -1;

	return std::distance(views.begin(), iter);
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

bool saveDockingState(const char* filename)
{
	return true;
}

u8* saveDockingStateToMemory(size_t& outStateInfoSize)
{
	return 0;
}

bool loadDockingState(const char* filename)
{
	return true;
}

bool loadDockingStateFromMemory(const u8* stateInfo, size_t stateInfoSize)
{
	return true;
}

}
