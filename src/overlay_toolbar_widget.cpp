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
// Overlay toolbars are private implementation details; the public C API in horus.h
// exposes them only as opaque handles (HOverlayToolbar / HOverlayToolbarWidget), so
// the concrete structs below are file-local.

enum class OverlayToolbarElementType
{
	Button,
	Toggle,
	Separator,
	Space,
	Grip,
};

struct OverlayToolbarElement
{
	OverlayToolbarElementType type = OverlayToolbarElementType::Button;
	std::string id;
	std::string label;
	std::string tooltip;
	HImage icon = nullptr; // icon shown when off
	HImage iconOn = nullptr;     // icon shown when toggled on
	void (*onClick)() = nullptr;
	bool* toggleValue = nullptr;
	bool visible = true;
	bool enabled = true;
	Rect rect;
};

struct OverlayToolbar
{
	std::string id;
	std::string title;
	OverlayDockZone dockZone = OverlayDockZone::Floating;
	OverlayToolbarLayout layout = OverlayToolbarLayout::Horizontal;
	bool collapsed = false;
	bool visible = true;
	bool dragging = false;
	std::vector<OverlayToolbarElement> elements;
	Rect rect;
	Point floatingPosition;
	Point dragStartToolbarPos;
	Point dragStartMousePos;
	bool showDockPreview = false;
	OverlayDockZone previewDockZone = OverlayDockZone::Floating;
};

struct OverlayToolbarManager
{
	// std::deque is used instead of std::vector so that push_back never
	// invalidates pointers to existing elements (HOverlayToolbar handles are
	// raw pointers into this container, so vector reallocation would dangling them).
	std::deque<OverlayToolbar> toolbars;
	OverlayToolbar* draggedToolbar = nullptr;
	OverlayToolbar* hoveredToolbar = nullptr;
	OverlayToolbarElement* hoveredElement = nullptr;
	bool showGlobalDockPreview = false;
	Rect globalPreviewRect;
	OverlayDockZone globalPreviewZone = OverlayDockZone::Floating;
};

static OverlayToolbarManager s_manager;
static Rect s_viewportRect;
static bool s_overlayActive = false;
static WidgetId s_dragCaptureId = 0;
static OverlayToolbar* s_pendingDragToolbar = nullptr;
static Point s_pendingDragStartMouse;
static bool s_pendingDragPressed = false;

HOverlayToolbar overlayToolbarFind(const char* id)
{
	if (!ctx || !id)
		return nullptr;

	for (auto& toolbar : s_manager.toolbars)
	{
		if (toolbar.id == id)
			return &toolbar;
	}

	return nullptr;
}

HOverlayToolbar overlayToolbarCreate(const char* id, const char* title, OverlayDockZone initialDockZone)
{
	if (!ctx)
		return nullptr;

	s_manager.toolbars.push_back({});
	auto& toolbar = s_manager.toolbars.back();

	toolbar.id = id ? id : "";
	toolbar.title = title ? title : "";
	toolbar.dockZone = initialDockZone;

	return &toolbar;
}

HOverlayToolbarWidget overlayToolbarAddButton(HOverlayToolbar toolbar, const char* elementId, const char* label, HImage icon, void (*onClick)(), const char* tooltip)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;
	if (!tbar)
		return nullptr;

	tbar->elements.push_back({});
	auto& e = tbar->elements.back();

	e.type = OverlayToolbarElementType::Button;
	e.id = elementId ? elementId : "";
	e.label = label ? label : "";
	e.icon = icon;
	e.onClick = onClick;

	if (tooltip)
		e.tooltip = tooltip;

	return &e;
}

HOverlayToolbarWidget overlayToolbarAddToggle(HOverlayToolbar toolbar, const char* elementId, const char* label, HImage iconOff, HImage iconOn, bool* toggleValue, const char* tooltip)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;
	
	if (!tbar)
		return nullptr;

	tbar->elements.push_back({});
	auto& e = tbar->elements.back();

	e.type = OverlayToolbarElementType::Toggle;
	e.id = elementId ? elementId : "";
	e.label = label ? label : "";
	e.icon = iconOff;
	e.iconOn = iconOn;
	e.toggleValue = toggleValue;

	if (tooltip)
		e.tooltip = tooltip;

	return &e;
}

HOverlayToolbarWidget overlayToolbarAddSeparator(HOverlayToolbar toolbar)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;
	
	if (!tbar)
		return nullptr;

	tbar->elements.push_back({});
	auto& e = tbar->elements.back();

	e.type = OverlayToolbarElementType::Separator;
	e.id = "separator";

	return &e;
}

HOverlayToolbarWidget overlayToolbarAddSpace(HOverlayToolbar toolbar)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return nullptr;

	tbar->elements.push_back({});
	auto& e = tbar->elements.back();

	e.type = OverlayToolbarElementType::Space;
	e.id = "space";

	return &e;
}

HOverlayToolbarWidget overlayToolbarAddGrip(HOverlayToolbar toolbar)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return nullptr;

	tbar->elements.push_back({});
	auto& e = tbar->elements.back();

	e.type = OverlayToolbarElementType::Grip;
	e.id = "grip";

	return &e;
}

void overlayToolbarRemoveElement(HOverlayToolbar toolbar, const char* elementId)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;
	
	if (!tbar || !elementId)
		return;

	for (auto iter = tbar->elements.begin(); iter != tbar->elements.end(); ++iter)
	{
		if (iter->id == elementId)
		{
			tbar->elements.erase(iter);
			break;
		}
	}
}

void overlayToolbarSetDockZone(HOverlayToolbar toolbar, OverlayDockZone zone)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return;

	tbar->dockZone = zone;
}

void overlayToolbarSetFloatingPosition(HOverlayToolbar toolbar, const Point& pos)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return;

	tbar->floatingPosition = pos;
}

void overlayToolbarSetLayout(HOverlayToolbar toolbar, OverlayToolbarLayout layout)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return;

	tbar->layout = layout;
}

void overlayToolbarSetCollapsed(HOverlayToolbar toolbar, bool collapsed)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return;

	tbar->collapsed = collapsed;
}

void overlayToolbarSetVisible(HOverlayToolbar toolbar, bool visible)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (tbar)
		tbar->visible = visible;
}

bool overlayToolbarIsVisible(HOverlayToolbar toolbar)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return false;

	return tbar->visible;
}

bool overlayToolbarIsDragging(HOverlayToolbar toolbar)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!tbar)
		return false;

	return tbar->dragging;
}

Rect overlayToolbarGetDockZoneRect(const Rect& viewportRect, OverlayDockZone zone, f32 toolbarThickness)
{
	switch (zone)
	{
	case OverlayDockZone::TopToolbar:
		return { viewportRect.x, viewportRect.y, viewportRect.width, toolbarThickness };

	case OverlayDockZone::BottomToolbar:
		return { viewportRect.x, viewportRect.bottom() - toolbarThickness, viewportRect.width, toolbarThickness };

	case OverlayDockZone::LeftToolbar:
		return { viewportRect.x, viewportRect.y, toolbarThickness, viewportRect.height };

	case OverlayDockZone::RightToolbar:
		return { viewportRect.right() - toolbarThickness, viewportRect.y, toolbarThickness, viewportRect.height };

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

// returns the main axis span and the cross axis span of a toolbar element content
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

	case OverlayToolbarElementType::Toggle:
	case OverlayToolbarElementType::Button:
	default:
	{
		bool hasIcon = e->icon || (e->type == OverlayToolbarElementType::Toggle && e->iconOn);
		bool hasLabel = !e->label.empty();
		bool hasIconOnly = hasIcon && !hasLabel;

		f32 textW = 0;
		f32 textH = 0;

		if (hasLabel && state.font)
			{
				FontTextSize tsize = state.font->computeTextSize(e->label.c_str());
				textW = tsize.width;
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
	f32 wrapWidth = wrap ? std::max(s_viewportRect.width, 1.0f) : 0.0f;

	f32 cursor = padding;
	f32 lineStart = 0;
	f32 lineSize = crossSize;

	for (auto& e : toolbar->elements)
	{
		if (!e.visible)
			continue;

		Point span = elementContentSize(&e, vertical);
		f32 mainSpan = vertical ? span.y : span.x;

		if (wrap && cursor + mainSpan > wrapWidth - padding && cursor > padding + 0.5f)
		{
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

	f32 usedMain = std::max(cursor - spacing + padding, padding * 2.0f);
	f32 usedCross = std::max(lineStart + lineSize + padding, padding * 2.0f);

	return vertical ? Point(bar.width, usedMain) : Point(usedMain, usedCross);
}

static void drawToolbarBackground(OverlayToolbar* toolbar)
{
	auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();

	ctx->renderer.cmdSetColor(state.color);
	ctx->renderer.cmdDrawFilledRectangle(toolbar->rect);

	if (toolbar->dragging)
	{
		ctx->renderer.cmdSetColor(ctx->settings.overlayToolbars.dockPreviewColor);
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
		ctx->renderer.cmdDrawFilledRectangle(e->rect);

		ctx->mouseCursor = MouseCursorType::SizeAll;
	}

	f32 dot = std::max(2.0f * ctx->scale, 1.5f);
	f32 gap = 4.0f * ctx->scale;
	bool horizontal = e->rect.width >= e->rect.height;
	u32 rows = horizontal ? 2 : 3;
	u32 columns = horizontal ? 3 : 2;

	Color dotColor = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState().textColor;

	if (hovered)
		dotColor = btnBody.getState(WidgetStateType::Hovered).textColor;

	Rect inner = e->rect.contract(st.elementPadding * ctx->scale);
	f32 totalW = columns * dot + (columns - 1) * gap;
	f32 totalH = rows * dot + (rows - 1) * gap;

	Point origin = {
		inner.x + (inner.width - totalW) / 2,
		inner.y + (inner.height - totalH) / 2,
	};

	for (u32 r = 0; r < rows; r++)
	{
		for (u32 c = 0; c < columns; c++)
		{
			Rect dotRect = {
				origin.x + c * (dot + gap),
				origin.y + r * (dot + gap),
				dot,
				dot,
			};

			ctx->renderer.cmdSetColor(dotColor);
			ctx->renderer.cmdDrawFilledRectangle(dotRect);
		}
	}
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

	ThemeElement::State* state = &btnBody.normalState();
	bool toggleOn = e->toggleValue && *e->toggleValue;

	if (!e->enabled)
		state = &btnBody.getState(WidgetStateType::Disabled);
	else if (toggleOn || ctx->widget.pressed)
		state = &btnBody.getState(WidgetStateType::Pressed);
	else if (ctx->widget.hovered)
		state = &btnBody.getState(WidgetStateType::Hovered);
	else if (ctx->widget.focused)
		state = &btnBody.getState(WidgetStateType::Focused);

	f32 padding = st.elementPadding * ctx->scale;
	f32 spacing = st.elementSpacing * ctx->scale;

	ctx->renderer.cmdSetColor(state->color);
	ctx->renderer.cmdDrawFilledRectangle(e->rect);

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

static bool toolbarMouseOverToolbar(OverlayToolbar* toolbar)
{
	return toolbar->rect.contains(ctx->mousePosition) && ctx->hoveringThisWindow;
}

static bool toolbarMouseOverInteractiveElement(OverlayToolbar* toolbar)
{
	for (auto& e : toolbar->elements)
	{
		if (e.visible
			&& e.rect.contains(ctx->mousePosition)
			&& (e.type == OverlayToolbarElementType::Button || e.type == OverlayToolbarElementType::Toggle))
		{
			return true;
		}
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

	if (e->type == OverlayToolbarElementType::Toggle && ctx->widget.clicked && e->toggleValue)
	{
		*e->toggleValue = !*e->toggleValue;
		forceRepaint();
	}

	return ctx->widget.clicked;
}

// returns the used main-axis size of a toolbar laid out on a thickness-sized strip,
// without modifying the toolbar element rects
static f32 measureToolbarMainSize(OverlayToolbar* toolbar, OverlayToolbarLayout layout, f32 thickness)
{
	const auto& st = ctx->settings.overlayToolbars;
	f32 padding = st.elementPadding * ctx->scale;
	f32 spacing = st.elementSpacing * ctx->scale;

	bool vertical = (layout == OverlayToolbarLayout::Vertical);
	f32 cursor = padding;

	bool wrap = (layout == OverlayToolbarLayout::Panel);
	f32 wrapWidth = wrap ? std::max(s_viewportRect.width, 1.0f) : 0.0f;

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

void overlayToolbarRender(HOverlayToolbar toolbar)
{
	OverlayToolbar* tbar = (OverlayToolbar*)toolbar;

	if (!ctx || !s_overlayActive)
		return;

	if (!tbar || !tbar->visible)
		return;

	const auto& st = ctx->settings.overlayToolbars;
	f32 thickness = st.defaultThickness * ctx->scale;
	f32 padding = st.elementPadding * ctx->scale;
	auto& mgr = s_manager;

	// dragged toolbars follow the mouse and highlight the dock zone under the cursor
	if (tbar->dragging)
	{
		Point pos = tbar->dragStartToolbarPos + (ctx->mousePosition - tbar->dragStartMousePos);
		f32 dragW = std::max(tbar->rect.width, 1.0f);
		f32 dragH = std::max(tbar->rect.height, 1.0f);

		pos.x = std::max(s_viewportRect.x, std::min(pos.x, s_viewportRect.right() - dragW));
		pos.y = std::max(s_viewportRect.y, std::min(pos.y, s_viewportRect.bottom() - dragH));

		tbar->floatingPosition = pos;
		tbar->rect = { pos.x, pos.y, dragW, dragH };

		OverlayDockZone zone = OverlayDockZone::Floating;

		if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::RightToolbar, thickness))
			zone = OverlayDockZone::RightToolbar;
		else if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::LeftToolbar, thickness))
			zone = OverlayDockZone::LeftToolbar;
		else if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::BottomToolbar, thickness))
			zone = OverlayDockZone::BottomToolbar;
		else if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::TopToolbar, thickness))
			zone = OverlayDockZone::TopToolbar;

		tbar->showDockPreview = (zone != OverlayDockZone::Floating);
		tbar->previewDockZone = zone;

		mgr.draggedToolbar = tbar;
		mgr.showGlobalDockPreview = tbar->showDockPreview;
		mgr.globalPreviewZone = zone;
		mgr.globalPreviewRect = tbar->showDockPreview
			? overlayToolbarGetDockZoneRect(s_viewportRect, zone, thickness)
			: Rect();

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& ctx->event.mouse.button == MouseButton::Left)
		{
			tbar->dockZone = tbar->showDockPreview ? zone : OverlayDockZone::Floating;
			tbar->dragging = false;
			tbar->showDockPreview = false;

			mgr.draggedToolbar = nullptr;
			mgr.showGlobalDockPreview = false;

			ctx->widget.captureId = 0;
			ctx->widget.hoveredId = 0;
			ctx->event = InputEvent();
			forceRepaint();
		}
	}
	else if (tbar->dockZone != OverlayDockZone::Floating)
	{
		// toolbars docked on the same edge share a single strip and are laid out
		// inline: side by side on top/bottom edges, stacked in a column on left/right edges
		OverlayToolbarLayout layout = toolbarEffectiveLayout(tbar);
		bool vertical = (layout == OverlayToolbarLayout::Vertical);
		Rect strip = overlayToolbarGetDockZoneRect(s_viewportRect, tbar->dockZone, thickness);
		f32 main = std::max(thickness, 1.0f);

		if (tbar->collapsed)
		{
			auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
			f32 titleW = (state.font ? state.font->computeTextSize(tbar->title.c_str()).width : 0.0f) + padding * 4.0f;
			main = std::max(titleW, thickness);
		}
		else
		{
			main = std::max(measureToolbarMainSize(tbar, layout, thickness), thickness);
		}

		f32 stripOffset = 0;

		// accumulate the sizes of the toolbars docked before this one on the same strip
		for (auto& other : s_manager.toolbars)
		{
			if (&other == tbar)
				break;

			if (!other.visible || other.dragging || other.dockZone != tbar->dockZone)
				continue;

			if (other.collapsed)
			{
				auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
				f32 titleW = (state.font ? state.font->computeTextSize(other.title.c_str()).width : 0.0f) + padding * 4.0f;
				stripOffset += std::max(titleW, thickness);
			}
			else
			{
				stripOffset += std::max(measureToolbarMainSize(&other, toolbarEffectiveLayout(&other), thickness), thickness);
			}
		}

		if (vertical)
			tbar->rect = { strip.x, strip.y + stripOffset, thickness, main };
		else
			tbar->rect = { strip.x + stripOffset, strip.y, main, thickness };
	}
	else
	{
		tbar->rect = { tbar->floatingPosition.x, tbar->floatingPosition.y, thickness, thickness };
	}

	OverlayToolbarLayout layout = toolbarEffectiveLayout(tbar);

	if (tbar->collapsed)
	{
		auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
		f32 titleW = (state.font ? state.font->computeTextSize(tbar->title.c_str()).width : 0.0f) + padding * 4.0f;

		if (tbar->dockZone == OverlayDockZone::Floating)
		{
			if (layout == OverlayToolbarLayout::Vertical)
			{
				tbar->rect.width = thickness + padding * 2.0f;
				tbar->rect.height = std::max(titleW, thickness);
			}
			else
			{
				tbar->rect.width = std::max(titleW, thickness);
				tbar->rect.height = thickness;
			}
		}
	}
	else
	{
		Point content = layoutToolbarElements(tbar, layout);

		if (tbar->dockZone == OverlayDockZone::Floating)
		{
			tbar->rect.width = std::max(content.x, thickness);
			tbar->rect.height = std::max(content.y, thickness);
		}
	}

	drawToolbarBackground(tbar);

	if (tbar->dragging)
	{
		// keep the drag while it lasts, so later widgets cannot steal it
		ctx->widget.captureId = s_dragCaptureId;
		ctx->widget.hoveredId = s_dragCaptureId;

		if (tbar->collapsed)
			drawToolbarTitle(tbar);

		return;
	}

	if (tbar->collapsed)
	{
		drawToolbarTitle(tbar);
	}
	else
	{
		mgr.hoveredElement = nullptr;

		for (auto& e : tbar->elements)
		{
			if (!e.visible)
				continue;

			bool clicked = false;

			if (e.type == OverlayToolbarElementType::Button || e.type == OverlayToolbarElementType::Toggle)
				clicked = processToolbarElement(tbar, &e);

			drawToolbarElement(tbar, &e);

			if (clicked && e.onClick)
				e.onClick();

			if (ctx->widget.hovered)
				mgr.hoveredElement = &e;

			if (!e.tooltip.empty())
				tooltip(e.tooltip.c_str());
		}
	}

	if (toolbarMouseOverToolbar(tbar))
		mgr.hoveredToolbar = tbar;

	// start dragging from the empty toolbar area, separators and spaces act as grab handles;
	// the drag only begins after the mouse moves past dragStartDistance
	if (ctx->event.type == InputEvent::Type::MouseDown
		&& ctx->event.mouse.button == MouseButton::Left
		&& toolbarMouseOverToolbar(tbar)
		&& (tbar->collapsed || !toolbarMouseOverInteractiveElement(tbar)))
	{
		s_pendingDragToolbar = tbar;
		s_pendingDragStartMouse = ctx->mousePosition;
		s_pendingDragPressed = true;

		ctx->widget.captureId = s_dragCaptureId;
		ctx->widget.hoveredId = s_dragCaptureId;
		ctx->alreadyClickedOnSomething = true;
		ctx->event = InputEvent();
		forceRepaint();
	}
	else if (s_pendingDragToolbar == toolbar && s_pendingDragPressed && !tbar->dragging)
	{
		if (s_pendingDragStartMouse.getDistance(ctx->mousePosition) >= st.dragStartDistance * ctx->scale)
		{
			tbar->dragging = true;
			tbar->dockZone = OverlayDockZone::Floating;
			tbar->dragStartMousePos = s_pendingDragStartMouse;

			// convert the docked extents into a floating content-sized rect
			OverlayToolbarLayout dragLayout = toolbarEffectiveLayout(tbar);
			tbar->rect = { tbar->rect.x, tbar->rect.y, thickness, thickness };

			if (tbar->collapsed)
			{
				auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
				f32 titleW = (state.font ? state.font->computeTextSize(tbar->title.c_str()).width : 0.0f) + padding * 4.0f;

				if (dragLayout == OverlayToolbarLayout::Vertical)
				{
					tbar->rect.width = thickness + padding * 2.0f;
					tbar->rect.height = std::max(titleW, thickness);
				}
				else
				{
					tbar->rect.width = std::max(titleW, thickness);
					tbar->rect.height = thickness;
				}
			}
			else
			{
				Point content = layoutToolbarElements(tbar, dragLayout);
				tbar->rect.width = std::max(content.x, thickness);
				tbar->rect.height = std::max(content.y, thickness);
			}

			tbar->dragStartToolbarPos = { tbar->rect.x, tbar->rect.y };

			ctx->widget.captureId = s_dragCaptureId;
			ctx->widget.hoveredId = s_dragCaptureId;
			s_pendingDragPressed = false;
			forceRepaint();
		}
		else if (ctx->event.type == InputEvent::Type::MouseUp)
		{
			s_pendingDragToolbar = nullptr;
			s_pendingDragPressed = false;
			ctx->widget.captureId = 0;
		}
	}
}

void overlayToolbarBegin(const Rect& viewportRect)
{
	if (!ctx)
		return;

	s_viewportRect = viewportRect;
	s_overlayActive = true;
	s_dragCaptureId = genId("##overlayToolbarDrag");

	s_manager.draggedToolbar = nullptr;
	s_manager.hoveredToolbar = nullptr;
	s_manager.hoveredElement = nullptr;
	s_manager.showGlobalDockPreview = false;

	if (ctx->event.type == InputEvent::Type::WindowLostFocus)
	{
		for (auto& toolbar : s_manager.toolbars)
		{
			if (toolbar.dragging)
			{
				toolbar.dragging = false;
				toolbar.showDockPreview = false;
			}
		}

		s_pendingDragToolbar = nullptr;
		s_pendingDragPressed = false;
	}

	ctx->renderer.pushWindowDrawCmdLayer(DrawCmdLayerType::Overlay);
	ctx->renderer.pushClipRect(s_viewportRect, false);
}

void overlayToolbarEnd()
{
	if (!ctx || !s_overlayActive)
		return;

	if (s_manager.showGlobalDockPreview && !s_manager.globalPreviewRect.isZero())
	{
		ctx->renderer.cmdSetColor(ctx->settings.overlayToolbars.dockPreviewColor);
		ctx->renderer.cmdDrawFilledRectangle(s_manager.globalPreviewRect);
	}

	ctx->renderer.popClipRect();
	ctx->renderer.popWindowDrawCmdLayer();

	s_overlayActive = false;
}

}