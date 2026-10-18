#include <algorithm>
#include <deque>
#include <string>
#include <vector>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "renderer.h"
#include "util.h"

namespace hui
{

OverlayToolbar* findToolbar(const char* id)
{
	if (!id)
		return nullptr;

	for (auto& toolbar : ctx->overlayToolbars.toolbars)
	{
		if (toolbar.id == id)
			return &toolbar;
	}
	return nullptr;
}

OverlayToolbar* getOrCreateToolbar(const char* id, const char* title, OverlayDockZone dockZone, OverlayToolbarLayout layout)
{
	OverlayToolbar* toolbar = findToolbar(id);
	if (toolbar)
	{
		if (toolbar->lastRequestedDockZone != dockZone && !toolbar->dragging)
		{
			toolbar->dockZone = dockZone;
			toolbar->lastRequestedDockZone = dockZone;
		}
		toolbar->title = title ? title : "";
		toolbar->layout = layout;
		return toolbar;
	}

	ctx->overlayToolbars.toolbars.push_back({});
	auto& t = ctx->overlayToolbars.toolbars.back();
	t.id = id ? id : "";
	t.title = title ? title : "";
	t.dockZone = dockZone;
	t.lastRequestedDockZone = dockZone;
	t.layout = layout;
	t.stripOrder = (u32)ctx->overlayToolbars.toolbars.size() - 1;
	return &t;
}

static OverlayToolbarLayout toolbarEffectiveLayout(OverlayToolbar* toolbar)
{
	switch (toolbar->dockZone)
	{
	case OverlayDockZone::LeftToolbar:
	case OverlayDockZone::RightToolbar:
		return OverlayToolbarLayout::Vertical;
	case OverlayDockZone::TopToolbar:
	case OverlayDockZone::BottomToolbar:
		return OverlayToolbarLayout::Horizontal;
	default:
		return toolbar->layout;
	}
}

static Point elementContentSize(OverlayToolbarElement* e, bool vertical)
{
	const auto& st = ctx->settings.overlayToolbars;
	f32 padding = st.elementPadding * ctx->scale;
	f32 iconSize = st.iconButtonSize * ctx->scale;
	auto& state = ctx->theme->getElement(WidgetElementId::ButtonBody).normalState();

	switch (e->type)
	{
	case OverlayToolbarElementType::Separator:
		return vertical ? Point(0, 3.0f * ctx->scale) : Point(3.0f * ctx->scale, 0);
	case OverlayToolbarElementType::Space:
		return vertical ? Point(0, st.elementSpacing * 4.0f * ctx->scale) : Point(st.elementSpacing * 4.0f * ctx->scale, 0);
	case OverlayToolbarElementType::Grip:
	{
		f32 grip = st.gripSize * ctx->scale;
		return vertical ? Point(0, grip) : Point(grip, 0);
	}
	case OverlayToolbarElementType::Dropdown:
	{
		f32 textW = 0, textH = 0;

		if (state.font)
		{
			for (u32 i = 0; i < e->itemCount; i++)
			{
				if (!e->items || !e->items[i])
					continue;

				FontTextSize tsize = state.font->computeTextSize(e->items[i]);
				textW = std::max(textW, tsize.width);
				textH = std::max(textH, tsize.height);
			}
		}

		f32 arrowSize = st.iconButtonSize * 0.5f * ctx->scale;

		if (vertical)
			return Point(0, std::max(iconSize, textH + padding * 2.0f));

		f32 w = textW + arrowSize + padding * 2.0f + st.elementSpacing * ctx->scale;

		if (e->icon)
			w += iconSize + st.elementSpacing * ctx->scale;

		return Point(w, 0);
	}
	case OverlayToolbarElementType::Toggle:
	case OverlayToolbarElementType::Button:
	default:
	{
		bool hasIcon = e->icon || (e->type == OverlayToolbarElementType::Toggle && e->iconOn);
		bool hasLabel = !e->label.empty();
		bool hasIconOnly = hasIcon && !hasLabel;
		f32 textW = 0, textH = 0;

		if (hasLabel && state.font)
		{
			FontTextSize tsize = state.font->computeTextSize(e->label.c_str());
			textW = tsize.width;
			textH = tsize.height;
		}

		if (vertical)
			return Point(0, hasIconOnly ? iconSize : std::max(iconSize, textH + padding * 2.0f));

		if (hasIconOnly)
			return Point(iconSize, 0);

		f32 w = hasLabel
			? std::max(textW, iconSize - padding * 2.0f) + padding * 2.0f
			: padding * 2.0f;

		if (hasIcon && hasLabel)
			w += iconSize + st.elementSpacing * ctx->scale;

		return Point(w, 0);
	}
	}
}

static Point layoutToolbarElements(OverlayToolbar* toolbar, OverlayToolbarLayout layout)
{
	const auto& st = ctx->settings.overlayToolbars;
	f32 padding = st.elementPadding * ctx->scale;
	f32 spacing = st.elementSpacing * ctx->scale;
	Rect bar = toolbar->rect;

	bool vertical = (layout == OverlayToolbarLayout::Vertical);
	f32 crossSize = std::max((vertical ? bar.width : bar.height) - padding * 2.0f, 1.0f);

	bool wrap = (layout == OverlayToolbarLayout::Panel);
	f32 wrapWidth = wrap ? (toolbar->useCustomSize ? toolbar->customSize.x : std::max(ctx->overlayToolbars.viewportRect.width, 1.0f)) : 0.0f;

	f32 cursor = padding;
	f32 lineStart = 0;
	f32 lineSize = crossSize;
	f32 maxLineWidth = padding * 2.0f;

	for (auto& e : toolbar->elements)
	{
		if (!e.visible)
			continue;

		Point span = elementContentSize(&e, vertical);
		f32 mainSpan = vertical ? span.y : span.x;

		if (wrap && cursor + mainSpan > wrapWidth - padding && cursor > padding + 0.5f)
		{
			maxLineWidth = std::max(maxLineWidth, cursor - spacing + padding);
			cursor = padding;
			lineStart += lineSize + spacing;
			lineSize = crossSize;
		}

		if (vertical)
		{
			e.rect = { bar.x + padding, bar.y + cursor, crossSize, mainSpan };
			cursor += mainSpan + spacing;
		}
		else
		{
			e.rect = { bar.x + cursor, bar.y + lineStart + padding, mainSpan, crossSize };
			cursor += mainSpan + spacing;
		}

		lineSize = std::max(lineSize, crossSize);
	}

	maxLineWidth = std::max(maxLineWidth, cursor - spacing + padding);
	f32 usedMain = std::max(maxLineWidth, padding * 2.0f);
	f32 usedCross = std::max(lineStart + lineSize + padding, padding * 2.0f);

	return vertical ? Point(bar.width, usedMain) : Point(usedMain, usedCross);
}

static Image* toolbarRoundRectImage()
{
	Image* img = ctx->theme->getImage(hashString("flat/round_rect"));
	return img ? img : ctx->theme->whiteImage;
}

static Image* toolbarGripImage()
{
	Image* img = ctx->theme->getImage(hashString("flat/grip"));
	return img ? img : ctx->theme->whiteImage;
}

static void drawToolbarBackground(OverlayToolbar* toolbar)
{
	const auto& st = ctx->settings.overlayToolbars;
	ctx->renderer.cmdSetColor(st.toolbarBackgroundColor);
	ctx->renderer.cmdDrawImageBordered(toolbarRoundRectImage(), st.roundRectBorder, toolbar->rect, ctx->scale);

	if (toolbar->dragging)
	{
		ctx->renderer.cmdSetLineStyle(LineStyle(st.toolbarBorderColor, st.toolbarBorderThickness * ctx->scale));
		ctx->renderer.cmdDrawRectangle(toolbar->rect);
	}
}

static void drawToolbarTitle(OverlayToolbar* toolbar)
{
	auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
	ctx->renderer.cmdSetFont(state.font);
	ctx->renderer.cmdSetColor(state.textColor);
	ctx->renderer.cmdDrawTextInBox(toolbar->title.c_str(), toolbar->rect, HAlignType::Center, VAlignType::Center, true);
}

static void drawToolbarIcon(ThemeElement::State* state, Image* icon, const Rect& rect)
{
	f32 imgW = icon->rect.width * ctx->scale;
	f32 imgH = icon->rect.height * ctx->scale;
	viewportImageSizeFit(imgW, imgH, std::max(rect.width, 1.0f), std::max(rect.height, 1.0f), imgW, imgH, false, false);

	Rect imgRect = {
		rect.x + (rect.width - imgW) / 2,
		rect.y + (rect.height - imgH) / 2,
		imgW,
		imgH
	};

	ctx->renderer.cmdSetColor(state->textColor);
	ctx->renderer.cmdDrawImage(icon, imgRect);
}

static void drawToolbarLabel(ThemeElement::State* state, OverlayToolbarElement* e, const Rect& rect)
{
	ctx->renderer.cmdSetFont(state->font);
	ctx->renderer.cmdSetColor(state->textColor);
	ctx->renderer.cmdDrawTextInBox(e->label.c_str(), rect, HAlignType::Center, VAlignType::Center);
}

static void drawToolbarGrip(OverlayToolbarElement* e)
{
	const auto& st = ctx->settings.overlayToolbars;
	auto& btnBody = ctx->theme->getElement(WidgetElementId::ButtonBody);
	bool hovered = e->rect.contains(ctx->mousePosition) && ctx->hoveringThisWindow;

	if (hovered)
	{
		ThemeElement::State* state = &btnBody.getState(WidgetStateType::Hovered);
		ctx->renderer.cmdSetColor(state->color);
		ctx->renderer.cmdDrawImageBordered(toolbarRoundRectImage(), st.roundRectBorder, e->rect, ctx->scale);
		ctx->mouseCursor = MouseCursorType::SizeAll;
	}

	ThemeElement::State* gripState = &ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
	if (hovered)
		gripState = &btnBody.getState(WidgetStateType::Hovered);

	Rect inner = e->rect.contract(st.elementPadding * ctx->scale);
	drawToolbarIcon(gripState, toolbarGripImage(), inner);
}

static void drawToolbarElement(OverlayToolbar* toolbar, OverlayToolbarElement* e)
{
	const auto& st = ctx->settings.overlayToolbars;
	auto& btnBody = ctx->theme->getElement(WidgetElementId::ButtonBody);

	if (e->type == OverlayToolbarElementType::Separator)
	{
		auto& barState = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
		Color sepColor = barState.textColor;
		sepColor.a = 0.5f;

		Rect rc = e->rect;
		if (rc.width >= rc.height)
			rc = { rc.x, rc.y + rc.height / 2 - 1, rc.width, 2 };
		else
			rc = { rc.x + rc.width / 2 - 1, rc.y, 2, rc.height };

		ctx->renderer.cmdSetColor(sepColor);
		ctx->renderer.cmdDrawFilledRectangle(rc);
		return;
	}

	if (e->type == OverlayToolbarElementType::Space)
		return;

	if (e->type == OverlayToolbarElementType::Grip)
	{
		drawToolbarGrip(e);
		return;
	}

	bool toggleOn = e->toggleValue && *e->toggleValue;
	bool pressed = toggleOn || ctx->widget.pressed;

	WidgetStateType stateType = WidgetStateType::Normal;
	if (!e->enabled)
		stateType = WidgetStateType::Disabled;
	else if (pressed)
		stateType = WidgetStateType::Pressed;
	else if (ctx->widget.hovered)
		stateType = WidgetStateType::Hovered;
	else if (ctx->widget.focused)
		stateType = WidgetStateType::Focused;

	ThemeElement::State* state = &btnBody.getState(stateType);

	f32 padding = st.elementPadding * ctx->scale;
	f32 spacing = st.elementSpacing * ctx->scale;

	Color bgColor = state->color;
	if (pressed && e->enabled)
		bgColor = st.buttonPressedColor;

	ctx->renderer.cmdSetColor(bgColor);
	ctx->renderer.cmdDrawImageBordered(toolbarRoundRectImage(), st.roundRectBorder, e->rect, ctx->scale);

	if (e->type == OverlayToolbarElementType::Dropdown)
	{
		const char* selectedText = nullptr;

		if (e->items && e->selectedIndex && *e->selectedIndex >= 0 && (u32)*e->selectedIndex < e->itemCount)
			selectedText = e->items[*e->selectedIndex];

		if (!selectedText)
			selectedText = e->label.empty() ? "" : e->label.c_str();

		auto& arrowElem = ctx->theme->getElement(WidgetElementId::DropdownArrow);
		ThemeElement::State* arrowState = &arrowElem.getState(stateType);
		Image* arrowImg = arrowState->image ? arrowState->image : ctx->theme->whiteImage;

		f32 iconSide = st.iconButtonSize * ctx->scale;
		f32 arrowW = (arrowState->width > 0.0f ? arrowState->width : arrowImg->rect.width) * ctx->scale;
		f32 arrowH = (arrowState->height > 0.0f ? arrowState->height : arrowImg->rect.height) * ctx->scale;
		f32 iconOffset = e->icon ? (iconSide + spacing) : 0.0f;
		Rect inner = e->rect.contract(padding);
		Rect textRect = { inner.x + iconOffset, inner.y, std::max(inner.width - iconOffset - arrowW - spacing, 0.0f), inner.height };

		if (e->icon)
		{
			f32 side = std::min(inner.height, iconSide);
			Rect iconBox = { inner.x, inner.y + (inner.height - side) / 2, side, side };
			drawToolbarIcon(state, (Image*)e->icon, iconBox);
		}

		ctx->renderer.cmdSetFont(state->font);
		ctx->renderer.cmdSetColor(state->textColor);
		ctx->renderer.cmdDrawTextInBox(selectedText, textRect, HAlignType::Left, VAlignType::Center, true);

		f32 imgW = arrowImg->rect.width * ctx->scale;
		f32 imgH = arrowImg->rect.height * ctx->scale;
		viewportImageSizeFit(imgW, imgH, std::max(arrowW, 1.0f), std::max(arrowH, 1.0f), imgW, imgH, false, false);

		Rect arrowRect = {
			inner.right() - arrowW + (arrowW - imgW) / 2,
			inner.y + (inner.height - imgH) / 2,
			imgW,
			imgH
		};

		ctx->renderer.cmdSetColor(tintApply(arrowState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImage(arrowImg, arrowRect);

		return;
	}

	bool hasIcon = e->icon || (e->type == OverlayToolbarElementType::Toggle && e->iconOn);
	Image* icon = nullptr;

	if (e->type == OverlayToolbarElementType::Toggle)
		icon = (Image*)(toggleOn && e->iconOn ? e->iconOn : e->icon ? e->icon : e->iconOn);
	else
		icon = (Image*)e->icon;

	Rect inner = e->rect.contract(padding);
	bool hasLabel = !e->label.empty();

	if (hasIcon && icon && hasLabel)
	{
		f32 iconSide = std::min(inner.width * 0.5f, inner.height);
		Rect iconBox = { inner.x, inner.y + (inner.height - iconSide) / 2, iconSide, iconSide };
		Rect labelRect = { iconBox.right() + spacing, inner.y, std::max(inner.width - iconSide - spacing, 0.0f), inner.height };

		drawToolbarIcon(state, icon, iconBox);
		drawToolbarLabel(state, e, labelRect);
	}
	else if (hasIcon && icon)
	{
		drawToolbarIcon(state, icon, inner);
	}
	else
	{
		drawToolbarLabel(state, e, inner);
	}
}

static void drawToolbarDropdownPopup(OverlayToolbar* toolbar, OverlayToolbarElement* e)
{
	if (!e->items || e->itemCount == 0)
	{
		toolbar->openDropdownRect = Rect();
		return;
	}

	const auto& st = ctx->settings.overlayToolbars;
	f32 padding = st.elementPadding * ctx->scale;
	auto& bodyState = ctx->theme->getElement(WidgetElementId::DropdownBody).normalState();
	f32 textW = 0;

	if (bodyState.font)
	{
		for (u32 i = 0; i < e->itemCount; i++)
		{
			if (e->items[i])
				textW = std::max(textW, bodyState.font->computeTextSize(e->items[i]).width);
		}
	}

	f32 popupWidth = std::max(e->rect.width, textW + padding * 4.0f);
	Point popupPos = (toolbarEffectiveLayout(toolbar) == OverlayToolbarLayout::Vertical)
		? e->rect.topRight()
		: e->rect.bottomLeft();

	// we need the exact width, so the popup's width won't be scaled by the global scale
	ctx->popupUseGlobalScale = false;

	std::string popupId = "toolbarDropdown##" + e->id;

	popupBegin(popupId.c_str(), popupWidth, PopupFlags::CustomPosition, popupPos, WidgetElementId::DropdownListBody);

	spacingPush(0);
	paddingPush(PaddingType::ScrollView, Point());
	tintPush(Color::white);

	bool selectedNewItem = false;
	i32 selected = e->selectedIndex ? *e->selectedIndex : -1;

	for (u32 i = 0; i < e->itemCount; i++)
	{
		auto selectedFlag = ((i32)i == selected) ? SelectableFlags::Selected : SelectableFlags::Normal;

		if (selectable(e->items[i] ? e->items[i] : "", selectedFlag))
		{
			if (e->selectedIndex)
				*e->selectedIndex = (i32)i;

			if (e->onSelect)
				e->onSelect((i32)i);

			selectedNewItem = true;
		}
	}

	tintPop();
	spacingPop();
	paddingPop(PaddingType::ScrollView);

	bool closed = selectedNewItem || popupMustClose();

	if (closed)
	{
		popupClose();
		toolbar->openDropdownId.clear();
	}

	popupEnd();
	ctx->popupUseGlobalScale = true;

	// remember the popup rect so the viewport underneath can ignore input over it
	if (!closed && ctx->popupStack[ctx->popupIndex].active)
	{
		auto& popup = ctx->popupStack[ctx->popupIndex];
		toolbar->openDropdownRect = { popup.position.x, popup.position.y, popup.width, popup.height };
	}
	else
		toolbar->openDropdownRect = Rect();
}

static bool toolbarMouseOverToolbar(OverlayToolbar* toolbar)
{
	return toolbar->rect.contains(ctx->mousePosition) && ctx->hoveringThisWindow;
}

static bool toolbarMouseOverInteractiveElement(OverlayToolbar* toolbar)
{
	for (auto& e : toolbar->elements)
	{
		if (e.visible && e.rect.contains(ctx->mousePosition)
			&& (e.type == OverlayToolbarElementType::Button || e.type == OverlayToolbarElementType::Toggle || e.type == OverlayToolbarElementType::Dropdown))
			return true;
	}
	return false;
}

static bool processToolbarElement(OverlayToolbar* toolbar, OverlayToolbarElement* e)
{
	std::string idStr = toolbar->id + "##" + e->id;
	ctx->id = genId(idStr.c_str());
	ctx->widget.nextDisabled = false;
	ctx->widget.disabled = !e->enabled;
	ctx->widget.rect = e->rect;
	ctx->widget.hoveredWidgetRect = e->rect;
	ctx->widget.hoveredType = WidgetType::None;
	ctx->widget.doubleClicked = false;
	ctx->widget.changeEnded = false;

	widgetSetFocusable();
	buttonBehavior();

	// when the dropdown popup is open, an outside press closes it, so release the
	// capture here to avoid the mouse release re-toggling the dropdown
	if (e->type == OverlayToolbarElementType::Dropdown && toolbar->openDropdownId == e->id && ctx->widget.captureId == ctx->id)
		ctx->widget.captureId = 0;

	if (e->type == OverlayToolbarElementType::Toggle && ctx->widget.clicked && e->toggleValue)
	{
		*e->toggleValue = !*e->toggleValue;
		if (e->onToggle)
			e->onToggle(*e->toggleValue);
		forceRepaint();
	}

	if (e->type == OverlayToolbarElementType::Dropdown && ctx->widget.clicked)
	{
		if (toolbar->openDropdownId == e->id)
		{
			toolbar->openDropdownId.clear();
		}
		else
		{
			for (auto& other : ctx->overlayToolbars.toolbars)
			{
				if (&other != toolbar)
					other.openDropdownId.clear();
			}

			toolbar->openDropdownId = e->id;
		}

		forceRepaint();
	}

	return ctx->widget.clicked;
}

static std::vector<OverlayToolbar*> overlayToolbarsInZone(OverlayDockZone zone, OverlayToolbar* exclude)
{
	std::vector<OverlayToolbar*> result;
	for (auto& toolbar : ctx->overlayToolbars.toolbars)
	{
		if (&toolbar == exclude)
			continue;
		if (toolbar.visible && !toolbar.dragging && toolbar.dockZone == zone)
			result.push_back(&toolbar);
	}
	std::stable_sort(result.begin(), result.end(), [](OverlayToolbar* a, OverlayToolbar* b) { return a->stripOrder < b->stripOrder; });
	return result;
}

static f32 measureToolbarMainSize(OverlayToolbar* toolbar, OverlayToolbarLayout layout, f32 thickness)
{
	const auto& st = ctx->settings.overlayToolbars;
	f32 padding = st.elementPadding * ctx->scale;
	f32 spacing = st.elementSpacing * ctx->scale;
	bool vertical = (layout == OverlayToolbarLayout::Vertical);
	f32 cursor = padding;
	bool wrap = (layout == OverlayToolbarLayout::Panel);
	f32 wrapWidth = wrap ? std::max(ctx->overlayToolbars.viewportRect.width, 1.0f) : 0.0f;

	for (auto& e : toolbar->elements)
	{
		if (!e.visible)
			continue;
		Point span = elementContentSize(&e, vertical);
		f32 mainSpan = vertical ? span.y : span.x;
		if (wrap && cursor + mainSpan > wrapWidth - padding && cursor > padding + 0.5f)
			cursor = padding;
		cursor += mainSpan + spacing;
	}
	return std::max(cursor - spacing + padding, padding * 2.0f);
}

static f32 dockedToolbarMainSize(OverlayToolbar* toolbar, f32 thickness, f32 padding)
{
	if (toolbar->collapsed)
	{
		auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
		f32 titleW = (state.font ? state.font->computeTextSize(toolbar->title.c_str()).width : 0.0f) + padding * 4.0f;
		return std::max(titleW, thickness);
	}
	return std::max(measureToolbarMainSize(toolbar, toolbarEffectiveLayout(toolbar), thickness), thickness);
}

struct ToolbarDockResult
{
	bool showHint = false;
	Rect hintRect;
	Rect toolbarRect;
	bool snapToEnd = false;
	u32 insertionIndex = 0;
};

static ToolbarDockResult computeToolbarDock(OverlayToolbar* toolbar, OverlayDockZone zone, f32 thickness, f32 padding)
{
	ToolbarDockResult result;
	Rect strip = overlayToolbarGetDockZoneRect(ctx->overlayToolbars.viewportRect, zone, thickness);

	bool vertical = (zone == OverlayDockZone::LeftToolbar || zone == OverlayDockZone::RightToolbar);
	f32 stripMain = vertical ? strip.height : strip.width;
	f32 mouseMain = vertical ? (ctx->mousePosition.y - strip.y) : (ctx->mousePosition.x - strip.x);
	result.snapToEnd = (mouseMain >= stripMain * 0.5f);

	std::vector<OverlayToolbar*> cluster;
	for (OverlayToolbar* other : overlayToolbarsInZone(zone, toolbar))
	{
		if (other->snapToEnd == result.snapToEnd)
			cluster.push_back(other);
	}

	f32 total = 0, passed = 0;
	u32 index = 0;
	for (OverlayToolbar* other : cluster)
		total += dockedToolbarMainSize(other, thickness, padding);

	for (u32 r = 0; r < cluster.size(); r++)
	{
		f32 size = dockedToolbarMainSize(cluster[r], thickness, padding);
		f32 start = result.snapToEnd ? (stripMain - (total - passed)) : passed;
		f32 mid = start + size * 0.5f;
		if (mouseMain > mid)
		{
			index = r + 1;
			passed += size;
		}
		else break;
	}

	f32 mySize = dockedToolbarMainSize(toolbar, thickness, padding);
	f32 newStart = result.snapToEnd ? (stripMain - (total - passed) - mySize) : passed;
	f32 hintThickness = ctx->settings.overlayToolbars.insertionHintThickness * ctx->scale;
	f32 barPos = std::max(newStart - hintThickness / 2.0f, 0.0f);

	if (vertical)
	{
		result.hintRect = { strip.x, strip.y + barPos, thickness, hintThickness };
		result.toolbarRect = { strip.x, strip.y + newStart, thickness, mySize };
	}
	else
	{
		result.hintRect = { strip.x + barPos, strip.y, hintThickness, thickness };
		result.toolbarRect = { strip.x + newStart, strip.y, mySize, thickness };
	}
	result.showHint = true;
	result.insertionIndex = index;
	return result;
}

void overlayToolbarsBegin(const Rect& viewportRect)
{
	if (!ctx)
		return;

	ctx->overlayToolbars.viewportRect = viewportRect;
	ctx->overlayToolbars.overlayActive = true;
	ctx->overlayToolbars.dragCaptureId = genId("##overlayToolbarDrag");
}

void overlayToolbarsEnd()
{
	if (!ctx || !ctx->overlayToolbars.overlayActive)
		return;

	if (ctx->overlayToolbars.activeToolbarId)
	{
		OverlayToolbar* active = nullptr;
		for (auto& t : ctx->overlayToolbars.toolbars)
		{
			if ((WidgetId)&t == ctx->overlayToolbars.activeToolbarId)
			{
				active = &t;
				break;
			}
		}
		if (active && active->showDockPreview && !active->rect.isZero())
		{
			const auto& st = ctx->settings.overlayToolbars;
			f32 thickness = st.defaultThickness * ctx->scale;
			f32 padding = st.elementPadding * ctx->scale;
			ToolbarDockResult dock = computeToolbarDock(active, active->previewDockZone, thickness, padding);
			ctx->renderer.cmdSetColor(st.dockPreviewColor);
			ctx->renderer.cmdDrawFilledRectangle(dock.toolbarRect);
		}
	}

	ctx->overlayToolbars.overlayActive = false;
}

bool overlayToolbarBegin(const char* id, const char* title, OverlayDockZone dockZone, OverlayToolbarLayout layout)
{
	if (!ctx || !ctx->overlayToolbars.overlayActive || !id)
		return false;

	OverlayToolbar* toolbar = getOrCreateToolbar(id, title, dockZone, layout);
	ctx->overlayToolbars.currentToolbar = toolbar;

	if (!toolbar->visible)
		return false;

	// Clear elements for immediate mode - rebuild every frame
	toolbar->elements.clear();
	toolbar->dragPositionUpdated = false;

	const auto& st = ctx->settings.overlayToolbars;
	f32 thickness = st.defaultThickness * ctx->scale;
	f32 padding = st.elementPadding * ctx->scale;

	if (toolbar->dragging)
	{
		Point pos = toolbar->dragStartToolbarPos + (ctx->mousePosition - toolbar->dragStartMousePos);
		f32 dragW = std::max(toolbar->rect.width, 1.0f);
		f32 dragH = std::max(toolbar->rect.height, 1.0f);

		if (toolbar->snapToGrid)
		{
			f32 grid = toolbar->gridSize > 0 ? toolbar->gridSize : ctx->settings.overlayToolbars.floatingGridSize;
			if (grid > 0)
			{
				pos.x = std::round(pos.x / grid) * grid;
				pos.y = std::round(pos.y / grid) * grid;
			}
		}

		pos.x = std::max(ctx->overlayToolbars.viewportRect.x, std::min(pos.x, ctx->overlayToolbars.viewportRect.right() - dragW));
		pos.y = std::max(ctx->overlayToolbars.viewportRect.y, std::min(pos.y, ctx->overlayToolbars.viewportRect.bottom() - dragH));

		toolbar->floatingPosition = { pos.x - ctx->overlayToolbars.viewportRect.x, pos.y - ctx->overlayToolbars.viewportRect.y };
		toolbar->dragPositionUpdated = true;
		toolbar->rect = { pos.x, pos.y, dragW, dragH };

		OverlayDockZone zone = OverlayDockZone::Floating;
		if (overlayToolbarPointInDockZone(ctx->overlayToolbars.viewportRect, ctx->mousePosition, OverlayDockZone::RightToolbar, thickness))
			zone = OverlayDockZone::RightToolbar;
		else if (overlayToolbarPointInDockZone(ctx->overlayToolbars.viewportRect, ctx->mousePosition, OverlayDockZone::LeftToolbar, thickness))
			zone = OverlayDockZone::LeftToolbar;
		else if (overlayToolbarPointInDockZone(ctx->overlayToolbars.viewportRect, ctx->mousePosition, OverlayDockZone::BottomToolbar, thickness))
			zone = OverlayDockZone::BottomToolbar;
		else if (overlayToolbarPointInDockZone(ctx->overlayToolbars.viewportRect, ctx->mousePosition, OverlayDockZone::TopToolbar, thickness))
			zone = OverlayDockZone::TopToolbar;

		toolbar->showDockPreview = (zone != OverlayDockZone::Floating);
		toolbar->previewDockZone = zone;

		ToolbarDockResult dock;
		if (toolbar->showDockPreview)
		{
			dock = computeToolbarDock(toolbar, zone, thickness, padding);
			ctx->overlayToolbars.activeToolbarId = (WidgetId)toolbar;
		}
		else
		{
			ctx->overlayToolbars.activeToolbarId = 0;
		}

		if (ctx->event.type == InputEvent::Type::MouseUp && ctx->event.mouse.button == MouseButton::Left)
		{
			if (toolbar->showDockPreview)
			{
				toolbar->dockZone = zone;
				toolbar->snapToEnd = dock.snapToEnd;
				std::vector<OverlayToolbar*> cluster;
				for (OverlayToolbar* other : overlayToolbarsInZone(zone, toolbar))
				{
					if (other->snapToEnd == toolbar->snapToEnd)
						cluster.push_back(other);
				}
				u32 index = std::min<u32>(cluster.size(), dock.insertionIndex);
				cluster.insert(cluster.begin() + index, toolbar);
				for (u32 i = 0; i < cluster.size(); i++)
					cluster[i]->stripOrder = i;
			}
			else
			{
				toolbar->dockZone = OverlayDockZone::Floating;
			}
			toolbar->dragging = false;
			toolbar->showDockPreview = false;
			ctx->widget.captureId = 0;
			ctx->widget.hoveredId = 0;
			ctx->event = InputEvent();
			forceRepaint();
		}
	}

	// For non-dragging toolbars, rect computation is deferred to overlayToolbarEnd
	// where elements have been populated and measurements are correct.
	// For dragging toolbars, the rect was computed above using the previous frame's dimensions.
	return true;
}

void overlayToolbarEnd()
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;

	OverlayToolbar* toolbar = ctx->overlayToolbars.currentToolbar;
	const auto& st = ctx->settings.overlayToolbars;
	f32 thickness = st.defaultThickness * ctx->scale;
	f32 padding = st.elementPadding * ctx->scale;
	OverlayToolbarLayout effLayout = toolbarEffectiveLayout(toolbar);

	if (toolbar->dragging)
	{
		// Dragging toolbar: position was already set in overlayToolbarBegin.
		// Layout elements and compute size with populated elements.
		if (toolbar->collapsed)
		{
			auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
			f32 titleW = (state.font ? state.font->computeTextSize(toolbar->title.c_str()).width : 0.0f) + padding * 4.0f;
			if (effLayout == OverlayToolbarLayout::Vertical)
			{
				toolbar->rect.width = thickness + padding * 2.0f;
				toolbar->rect.height = std::max(titleW, thickness);
			}
			else
			{
				toolbar->rect.width = std::max(titleW, thickness);
				toolbar->rect.height = thickness;
			}
		}
		else
		{
			Point content = layoutToolbarElements(toolbar, effLayout);
			if (toolbar->useCustomSize)
			{
				toolbar->rect.width = toolbar->customSize.x;
				toolbar->rect.height = toolbar->customSize.y;
			}
			else
			{
				toolbar->rect.width = std::max(content.x, thickness);
				toolbar->rect.height = std::max(content.y, thickness);
			}
		}
	}
	else
	{
		// Non-dragging toolbar: compute position & size now that elements are populated.
		if (toolbar->dockZone != OverlayDockZone::Floating)
		{
			bool vertical = (effLayout == OverlayToolbarLayout::Vertical);
			Rect strip = overlayToolbarGetDockZoneRect(ctx->overlayToolbars.viewportRect, toolbar->dockZone, thickness);
			f32 main = dockedToolbarMainSize(toolbar, thickness, padding);
			f32 stripOffset = 0;

			std::vector<OverlayToolbar*> startCluster, endCluster;
			for (OverlayToolbar* other : overlayToolbarsInZone(toolbar->dockZone, toolbar))
			{
				if (other->snapToEnd)
					endCluster.push_back(other);
				else
					startCluster.push_back(other);
			}

			if (toolbar->snapToEnd)
			{
				f32 endTotal = main;
				for (OverlayToolbar* other : endCluster)
					endTotal += dockedToolbarMainSize(other, thickness, padding);
				for (OverlayToolbar* other : endCluster)
				{
					if (other->stripOrder >= toolbar->stripOrder)
						break;
					stripOffset += dockedToolbarMainSize(other, thickness, padding);
				}
				f32 stripMain = vertical ? strip.height : strip.width;
				f32 startPos = std::max(stripMain - endTotal + stripOffset, 0.0f);
				if (vertical)
					toolbar->rect = { strip.x, strip.y + startPos, thickness, main };
				else
					toolbar->rect = { strip.x + startPos, strip.y, main, thickness };
			}
			else
			{
				for (OverlayToolbar* other : startCluster)
				{
					if (other->stripOrder >= toolbar->stripOrder)
						break;
					stripOffset += dockedToolbarMainSize(other, thickness, padding);
				}
				if (vertical)
					toolbar->rect = { strip.x, strip.y + stripOffset, thickness, main };
				else
					toolbar->rect = { strip.x + stripOffset, strip.y, main, thickness };
			}
		}
		else
		{
			Point pos = { ctx->overlayToolbars.viewportRect.x + toolbar->floatingPosition.x, ctx->overlayToolbars.viewportRect.y + toolbar->floatingPosition.y };
			if (toolbar->snapToGrid)
			{
				f32 grid = toolbar->gridSize > 0 ? toolbar->gridSize : ctx->settings.overlayToolbars.floatingGridSize;
				if (grid > 0)
				{
					pos.x = std::round(pos.x / grid) * grid;
					pos.y = std::round(pos.y / grid) * grid;
				}
			}
			f32 initW = toolbar->useCustomSize ? toolbar->customSize.x : thickness;
			f32 initH = toolbar->useCustomSize ? toolbar->customSize.y : thickness;
			toolbar->rect = { pos.x, pos.y, initW, initH };
		}

		if (toolbar->collapsed)
		{
			auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
			f32 titleW = (state.font ? state.font->computeTextSize(toolbar->title.c_str()).width : 0.0f) + padding * 4.0f;
			if (toolbar->dockZone == OverlayDockZone::Floating)
			{
				if (toolbar->useCustomSize)
				{
					toolbar->rect.width = toolbar->customSize.x;
					toolbar->rect.height = toolbar->customSize.y;
				}
				else
				{
					if (effLayout == OverlayToolbarLayout::Vertical)
					{
						toolbar->rect.width = thickness + padding * 2.0f;
						toolbar->rect.height = std::max(titleW, thickness);
					}
					else
					{
						toolbar->rect.width = std::max(titleW, thickness);
						toolbar->rect.height = thickness;
					}
				}
			}
		}
		else
		{
			Point content = layoutToolbarElements(toolbar, effLayout);
			if (toolbar->dockZone == OverlayDockZone::Floating)
			{
				if (toolbar->useCustomSize)
				{
					toolbar->rect.width = toolbar->customSize.x;
					toolbar->rect.height = toolbar->customSize.y;
				}
				else
				{
					toolbar->rect.width = std::max(content.x, thickness);
					toolbar->rect.height = std::max(content.y, thickness);
				}
			}
		}
	}

	drawToolbarBackground(toolbar);
	ctx->renderer.pushClipRect(toolbar->rect, true);

	OverlayToolbarElement* openDropdownElement = nullptr;

	if (toolbar->collapsed)
	{
		drawToolbarTitle(toolbar);
	}
	else
	{
		for (auto& e : toolbar->elements)
		{
			if (!e.visible)
				continue;

			bool clicked = false;
			if (!toolbar->dragging && (e.type == OverlayToolbarElementType::Button || e.type == OverlayToolbarElementType::Toggle || e.type == OverlayToolbarElementType::Dropdown))
				clicked = processToolbarElement(toolbar, &e);

			if (!toolbar->dragging && e.type == OverlayToolbarElementType::Dropdown && toolbar->openDropdownId == e.id)
				openDropdownElement = &e;

			drawToolbarElement(toolbar, &e);

			if (e.type == OverlayToolbarElementType::Grip
				&& ctx->event.type == InputEvent::Type::MouseDown
				&& ctx->event.mouse.button == MouseButton::Left
				&& ctx->event.mouse.clickCount >= 2
				&& e.rect.contains(ctx->mousePosition))
			{
				// double click on grip collapses the toolbar
				toolbar->collapsed = true;
				ctx->overlayToolbars.pendingDrag = false;
				ctx->widget.captureId = 0;
				ctx->event = InputEvent();
				forceRepaint();
			}

			if (clicked && e.onClick)
				e.onClick();
			if (!toolbar->dragging && !e.tooltip.empty())
				tooltip(e.tooltip.c_str());
		}
	}

	if (!openDropdownElement)
	{
		toolbar->openDropdownId.clear();
		toolbar->openDropdownRect = Rect();
	}

	if (toolbar->dragging)
	{
		ctx->widget.captureId = ctx->overlayToolbars.dragCaptureId;
		ctx->widget.hoveredId = ctx->overlayToolbars.dragCaptureId;
	}
	else
	{
		if (toolbarMouseOverToolbar(toolbar))
			ctx->overlayToolbars.activeToolbarId = (WidgetId)toolbar;

		if (toolbar->collapsed
			&& ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Left
			&& ctx->event.mouse.clickCount >= 2
			&& toolbarMouseOverToolbar(toolbar))
		{
			// double clicking a collapsed toolbar expands it again
			toolbar->collapsed = false;
			ctx->overlayToolbars.pendingDrag = false;
			ctx->widget.captureId = 0;
			ctx->event = InputEvent();
			forceRepaint();
		}
		else if (ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Left
			&& toolbarMouseOverToolbar(toolbar)
			&& (toolbar->collapsed || !toolbarMouseOverInteractiveElement(toolbar)))
		{
			ctx->overlayToolbars.pendingDrag = true;
			ctx->overlayToolbars.pendingDragStartMouse = ctx->mousePosition;
			ctx->overlayToolbars.activeToolbarId = (WidgetId)toolbar;

			ctx->widget.captureId = ctx->overlayToolbars.dragCaptureId;
			ctx->widget.hoveredId = ctx->overlayToolbars.dragCaptureId;
			ctx->alreadyClickedOnSomething = true;
			ctx->event = InputEvent();
			forceRepaint();
		}
		else if (ctx->overlayToolbars.pendingDrag && ctx->overlayToolbars.activeToolbarId == (WidgetId)toolbar && !toolbar->dragging)
		{
			if (ctx->overlayToolbars.pendingDragStartMouse.getDistance(ctx->mousePosition) >= st.dragStartDistance * ctx->scale)
			{
				toolbar->dragging = true;
				toolbar->dockZone = OverlayDockZone::Floating;
				toolbar->dragStartMousePos = ctx->overlayToolbars.pendingDragStartMouse;

				OverlayToolbarLayout dragLayout = toolbarEffectiveLayout(toolbar);
				f32 dragInitW = toolbar->useCustomSize ? toolbar->customSize.x : thickness;
				f32 dragInitH = toolbar->useCustomSize ? toolbar->customSize.y : thickness;
				toolbar->rect = { toolbar->rect.x, toolbar->rect.y, dragInitW, dragInitH };

				if (toolbar->collapsed)
				{
					auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
					f32 titleW = (state.font ? state.font->computeTextSize(toolbar->title.c_str()).width : 0.0f) + padding * 4.0f;
					if (dragLayout == OverlayToolbarLayout::Vertical)
					{
						toolbar->rect.width = thickness + padding * 2.0f;
						toolbar->rect.height = std::max(titleW, thickness);
					}
					else
					{
						toolbar->rect.width = std::max(titleW, thickness);
						toolbar->rect.height = thickness;
					}
				}
				else
				{
					Point content = layoutToolbarElements(toolbar, dragLayout);
					if (toolbar->useCustomSize)
					{
						toolbar->rect.width = toolbar->customSize.x;
						toolbar->rect.height = toolbar->customSize.y;
					}
					else
					{
						toolbar->rect.width = std::max(content.x, thickness);
						toolbar->rect.height = std::max(content.y, thickness);
					}
				}

				toolbar->dragStartToolbarPos = { toolbar->rect.x, toolbar->rect.y };
				ctx->widget.captureId = ctx->overlayToolbars.dragCaptureId;
				ctx->widget.hoveredId = ctx->overlayToolbars.dragCaptureId;
				ctx->overlayToolbars.pendingDrag = false;
				forceRepaint();
			}
			else if (ctx->event.type == InputEvent::Type::MouseUp)
			{
				ctx->overlayToolbars.pendingDrag = false;
				ctx->overlayToolbars.activeToolbarId = 0;
				ctx->widget.captureId = 0;
			}
		}
	}

	ctx->renderer.popClipRect();

	// the dropdown popup must be drawn after the toolbar clip is popped
	if (openDropdownElement)
		drawToolbarDropdownPopup(toolbar, openDropdownElement);

	ctx->overlayToolbars.currentToolbar = nullptr;
}

bool overlayToolbarButton(const char* elementId, const char* label, HImage icon, void (*onClick)(), const char* tooltip)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return false;

	ctx->overlayToolbars.currentToolbar->elements.push_back({});
	auto& e = ctx->overlayToolbars.currentToolbar->elements.back();
	e.type = OverlayToolbarElementType::Button;
	e.id = elementId ? elementId : "";
	e.label = label ? label : "";
	e.icon = icon;
	e.onClick = onClick;
	if (tooltip)
		e.tooltip = tooltip;

	return false;
}

bool overlayToolbarToggle(const char* elementId, const char* label, HImage iconOff, HImage iconOn, bool* toggleValue, void (*onToggle)(bool), const char* tooltip)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return false;

	ctx->overlayToolbars.currentToolbar->elements.push_back({});
	auto& e = ctx->overlayToolbars.currentToolbar->elements.back();
	e.type = OverlayToolbarElementType::Toggle;
	e.id = elementId ? elementId : "";
	e.label = label ? label : "";
	e.icon = iconOff;
	e.iconOn = iconOn;
	e.toggleValue = toggleValue;
	e.onToggle = onToggle;
	if (tooltip)
		e.tooltip = tooltip;

	return false;
}

void overlayToolbarDropdown(const char* elementId, const char** items, u32 itemCount, i32* selectedIndex, void (*onSelect)(i32), HImage icon, const char* tooltip)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;

	ctx->overlayToolbars.currentToolbar->elements.push_back({});
	auto& e = ctx->overlayToolbars.currentToolbar->elements.back();
	e.type = OverlayToolbarElementType::Dropdown;
	e.id = elementId ? elementId : "";
	e.items = items;
	e.itemCount = itemCount;
	e.selectedIndex = selectedIndex;
	e.onSelect = onSelect;
	e.icon = icon;
	if (tooltip)
		e.tooltip = tooltip;
}

void overlayToolbarSeparator()
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;

	ctx->overlayToolbars.currentToolbar->elements.push_back({});
	auto& e = ctx->overlayToolbars.currentToolbar->elements.back();
	e.type = OverlayToolbarElementType::Separator;
	e.id = "separator";
}

void overlayToolbarSpace()
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;

	ctx->overlayToolbars.currentToolbar->elements.push_back({});
	auto& e = ctx->overlayToolbars.currentToolbar->elements.back();
	e.type = OverlayToolbarElementType::Space;
	e.id = "space";
}

void overlayToolbarGrip()
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;

	ctx->overlayToolbars.currentToolbar->elements.push_back({});
	auto& e = ctx->overlayToolbars.currentToolbar->elements.back();
	e.type = OverlayToolbarElementType::Grip;
	e.id = "grip";
}

void overlayToolbarSetGridAlignment(bool enable, f32 gridSize)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;
	ctx->overlayToolbars.currentToolbar->snapToGrid = enable;
	ctx->overlayToolbars.currentToolbar->gridSize = gridSize;
}

void overlayToolbarSetCustomSize(const Point& size, bool useCustomSize)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;
	ctx->overlayToolbars.currentToolbar->useCustomSize = useCustomSize;
	ctx->overlayToolbars.currentToolbar->customSize = size;
}

Rect overlayToolbarGetDockZoneRect(const Rect& viewportRect, OverlayDockZone zone, f32 toolbarThickness)
{
	if (!ctx)
		return Rect();

	f32 topInset = 0, bottomInset = 0;
	if (zone == OverlayDockZone::LeftToolbar || zone == OverlayDockZone::RightToolbar)
	{
		for (auto& toolbar : ctx->overlayToolbars.toolbars)
		{
			if (!toolbar.visible || toolbar.dragging)
				continue;
			if (toolbar.dockZone == OverlayDockZone::TopToolbar)
				topInset = toolbarThickness;
			else if (toolbar.dockZone == OverlayDockZone::BottomToolbar)
				bottomInset = toolbarThickness;
		}
	}

	switch (zone)
	{
	case OverlayDockZone::TopToolbar:
		return { viewportRect.x, viewportRect.y, viewportRect.width, toolbarThickness };
	case OverlayDockZone::BottomToolbar:
		return { viewportRect.x, viewportRect.bottom() - toolbarThickness, viewportRect.width, toolbarThickness };
	case OverlayDockZone::LeftToolbar:
		return { viewportRect.x, viewportRect.y + topInset, toolbarThickness, std::max(viewportRect.height - topInset - bottomInset, 0.0f) };
	case OverlayDockZone::RightToolbar:
		return { viewportRect.right() - toolbarThickness, viewportRect.y + topInset, toolbarThickness, std::max(viewportRect.height - topInset - bottomInset, 0.0f) };
	default:
		return Rect();
	}
}

bool overlayToolbarPointInDockZone(const Rect& viewportRect, const Point& pt, OverlayDockZone zone, f32 toolbarThickness)
{
	if (zone == OverlayDockZone::Floating)
		return viewportRect.contains(pt);
	return overlayToolbarGetDockZoneRect(viewportRect, zone, toolbarThickness).contains(pt);
}

bool overlayToolbarContainsPoint(const Point& viewportPoint)
{
	if (!ctx)
		return false;

	for (auto& toolbar : ctx->overlayToolbars.toolbars)
	{
		if (toolbar.visible && (toolbar.dragging || ctx->overlayToolbars.pendingDrag))
			return true;
	}

	Point screenPos = { ctx->overlayToolbars.viewportRect.x + viewportPoint.x, ctx->overlayToolbars.viewportRect.y + viewportPoint.y };
	for (auto& toolbar : ctx->overlayToolbars.toolbars)
	{
		if (!toolbar.visible)
			continue;

		if (toolbar.rect.contains(screenPos))
			return true;

		if (toolbar.openDropdownRect.contains(screenPos))
			return true;
	}
	return false;
}

void overlayToolbarSetFloatingPosition(const Point& pos)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar || ctx->overlayToolbars.currentToolbar->dragPositionUpdated)
		return;
	ctx->overlayToolbars.currentToolbar->floatingPosition = pos;
}

Point overlayToolbarGetFloatingPosition()
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return Point();
	return ctx->overlayToolbars.currentToolbar->floatingPosition;
}

void overlayToolbarSetCollapsed(bool collapsed)
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return;
	ctx->overlayToolbars.currentToolbar->collapsed = collapsed;
}

bool overlayToolbarIsCollapsed()
{
	if (!ctx || !ctx->overlayToolbars.currentToolbar)
		return false;
	return ctx->overlayToolbars.currentToolbar->collapsed;
}

}