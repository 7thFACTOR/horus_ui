#include "osiris.h"
#include "types.h"

namespace hui
{
Element::Element()
{

}

Element::~Element()
{
	deleteChildren();
}

bool Element::castRay(const Vec3& start, const Vec3& dir, Vec3& outIntersection, f32& outTimeOnRay)
{

	return false;
}

Element* Element::findElement(const std::string& elementName)
{
	if (elementName == name)
	{
		return this;
	}

	for (auto& iter : children)
	{
		auto el = iter->findElement(elementName);
	
		if (el)
			return el;
	}

	return nullptr;
}

void Element::addChild(Element* element)
{
	auto iter = std::find(children.begin(), children.end(), element);

	if (iter != children.end())
		return;

	element->parent = this;
	children.push_back(element);
}

void Element::deleteChild(Element* element)
{
	auto iter = std::find(children.begin(), children.end(), element);

	if (iter == children.end())
		return;

	delete (*iter);
	children.erase(iter);
}

void Element::deleteChildren()
{
	for (auto& el : children)
	{
		delete el;
	}

	children.clear();
}

void Element::draw()
{

}

void Element::update()
{
	worldTransform.reset();
}

ButtonElement::ButtonElement()
{}

void ButtonElement::update()
{}

void ButtonElement::draw()
{
	//TODO: ctx->renderer->cmdDrawImageBordered(uiBodyImage,uiBodyImage-> )
}

bool ButtonElement::onMouseDown(const InputEvent3D& ev)
{
	return true;
}

bool ButtonElement::onMouseUp(const InputEvent3D& ev)
{
	return true;
}

bool ButtonElement::onMouseMove(const InputEvent3D& ev)
{
	return true;
}

bool ButtonElement::onKeyDown(const InputEvent3D& ev)
{
	return true;
}

bool ButtonElement::onKeyUp(const InputEvent3D& ev)
{
	return true;
}

}