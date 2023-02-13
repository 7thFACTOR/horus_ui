#include "view.h"
#include <string.h>
#include <algorithm>

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
	if (parent)
	{

	}
	else
	{
		auto rc = getWindowClientRect(window);
		rect.x = unitRect.x * rc.width;
		rect.y = unitRect.y * rc.height;
		rect.width = unitRect.width * rc.width;
		rect.height = unitRect.height * rc.height;
	}
}

}
