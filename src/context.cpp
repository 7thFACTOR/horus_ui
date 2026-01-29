#include <string.h>
#include "context.h"
#include "font.h"
#include "renderer.h"
#include "util.h"
#include "unicode_text_cache.h"

namespace hui
{
Context* ctx = nullptr;

Context::~Context()
{
}

void Context::setLabelAndId(const char* text)
{
	const char* textPtr = text ? text : "";
	auto idStart = strstr(textPtr, "##");

	id = 0;

	// we have ###, forced id specified
	if (idStart && *(idStart + 2) == '#')
	{
		id = genId(idStart);
	}
	else if (idStart) // we have ##, hash the whole text
	{
		id = genId(textPtr);
	}

	widgetLabel.assign(textPtr, idStart ? idStart : textPtr + strlen(textPtr));

	if (widgetLabel == "" || id == 0)
	{
		id = genIdFromPosition("fromPos");
	}
}

void Context::setSkipRenderAndInput(bool skip)
{
	skipRenderAndInput = skip;
	renderer->skipRender = skip;
}

void Context::initializeRenderer()
{
	if (!renderer)
	{
		renderer = new Renderer();
		textCache = new UnicodeTextCache();
	}
}

}
