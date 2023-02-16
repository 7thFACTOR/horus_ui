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
			parent->children.erase(iter);
	}
}

void DockNode::removeView(View* view)
{
	auto iter = std::find(views.begin(), views.end(), view);

	if (iter != views.end())
		views.erase(iter);
}

void DockNode::computeRect()
{
	Rect parentRect;

	if (!parent)
	{
		parentRect = getWindowClientRect(window);
	}
	else
	{
		parentRect = parent->rect;
	}

	switch (type)
	{
	case hui::DockNode::Type::None:
		break;
	case hui::DockNode::Type::ViewTabs:
		break;
	case hui::DockNode::Type::Vertical:
	{
		f32 availableSpace = parentRect.height + ctx->dockNodeSpacing * (f32)(children.size() - 1);
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

		f32 currentY = parentRect.y;

		for (auto& child : children)
		{
			f32 height = child->rect.height / totalSpace * availableSpace;

			child->rect.y = currentY;
			child->rect.x = parentRect.x;
			child->rect.width = parentRect.width;

			// recursive, could be done with a vector and a while loop though
			child->computeRect();

			currentY += height;
			currentY += ctx->dockNodeSpacing;
		}
	}
		break;
	case hui::DockNode::Type::Horizontal:
	{
		f32 availableSpace = parentRect.width + ctx->dockNodeSpacing * (f32)(children.size() - 1); 
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

		f32 currentX = parentRect.x;

		for (auto& child : children)
		{
			f32 width = child->rect.width / totalSpace * availableSpace;

			child->rect.x = currentX;
			child->rect.y = parentRect.y;
			child->rect.height = parentRect.height;

			// recursive, could be done with a vector and a while loop though
			child->computeRect();

			currentX += width;
			currentX += ctx->dockNodeSpacing;
		}
	}
		break;
	default:
		break;
	}
}

}
