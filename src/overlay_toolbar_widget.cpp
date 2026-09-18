#include <algorithm>
#include <string>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "renderer.h"
#include "util.h"

namespace hui
{
static OverlayToolbarManager s_manager;
static Rect s_viewportRect;
static bool s_overlayActive = false;
static WidgetId s_dragCaptureId = 0;
static OverlayToolbar* s_pendingDragToolbar = nullptr;
static Point s_pendingDragStartMouse;
static bool s_pendingDragPressed = false;

OverlayToolbarManager& overlayToolbarManagerGet()
{
	return s_manager;
}

OverlayToolbar* overlayToolbarFind(const char* id)
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

OverlayToolbar* overlayToolbarCreate(const char* id, const char* title, OverlayDockZone initialDockZone)
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

OverlayToolbarElement* overlayToolbarAddButton(OverlayToolbar* toolbar, const char* elementId, const char* label, HImage icon, void (*onClick)(), const char* tooltip)
{
	if (!toolbar)
		return nullptr;

	toolbar->elements.push_back({});
	auto& e = toolbar->elements.back();

	e.type = OverlayToolbarElementType::Button;
	e.id = elementId ? elementId : "";
	e.label = label ? label : "";
	e.icon = icon;
	e.onClick = onClick;

	if (tooltip)
		e.tooltip = tooltip;

	return &e;
}

OverlayToolbarElement* overlayToolbarAddToggle(OverlayToolbar* toolbar, const char* elementId, const char* label, HImage iconOff, HImage iconOn, bool* toggleValue, const char* tooltip)
{
	if (!toolbar)
		return nullptr;

	toolbar->elements.push_back({});
	auto& e = toolbar->elements.back();

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

OverlayToolbarElement* overlayToolbarAddSeparator(OverlayToolbar* toolbar)
{
	if (!toolbar)
		return nullptr;

	toolbar->elements.push_back({});
	auto& e = toolbar->elements.back();

	e.type = OverlayToolbarElementType::Separator;
	e.id = "separator";

	return &e;
}

OverlayToolbarElement* overlayToolbarAddSpace(OverlayToolbar* toolbar)
{
	if (!toolbar)
		return nullptr;

	toolbar->elements.push_back({});
	auto& e = toolbar->elements.back();

	e.type = OverlayToolbarElementType::Space;
	e.id = "space";

	return &e;
}

void overlayToolbarRemoveElement(OverlayToolbar* toolbar, const char* elementId)
{
	if (!toolbar || !elementId)
		return;

	for (auto iter = toolbar->elements.begin(); iter != toolbar->elements.end(); ++iter)
	{
		if (iter->id == elementId)
		{
			toolbar->elements.erase(iter);
			break;
		}
	}
}

void overlayToolbarSetDockZone(OverlayToolbar* toolbar, OverlayDockZone zone)
{
	if (toolbar)
		toolbar->dockZone = zone;
}

void overlayToolbarSetFloatingPosition(OverlayToolbar* toolbar, const Point& pos)
{
	if (toolbar)
		toolbar->floatingPosition = pos;
}

void overlayToolbarSetLayout(OverlayToolbar* toolbar, OverlayToolbarLayout layout)
{
	if (toolbar)
		toolbar->layout = layout;
}

void overlayToolbarSetCollapsed(OverlayToolbar* toolbar, bool collapsed)
{
	if (toolbar)
		toolbar->collapsed = collapsed;
}

void overlayToolbarSetVisible(OverlayToolbar* toolbar, bool visible)
{
	if (toolbar)
		toolbar->visible = visible;
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

static Rect dockedToolbarRect(OverlayToolbar* toolbar, f32 thickness)
{
	const Rect& vp = s_viewportRect;
	u32 stackIndex = 0;

	for (auto& other : s_manager.toolbars)
	{
		if (&other == toolbar)
			break;

		if (other.visible && !other.dragging && other.dockZone == toolbar->dockZone)
			stackIndex++;
	}

	switch (toolbar->dockZone)
	{
	case OverlayDockZone::TopToolbar:
		return { vp.x, vp.y + (f32)stackIndex * thickness, vp.width, thickness };

	case OverlayDockZone::BottomToolbar:
		return { vp.x, vp.bottom() - (f32)(stackIndex + 1) * thickness, vp.width, thickness };

	case OverlayDockZone::LeftToolbar:
		return { vp.x + (f32)stackIndex * thickness, vp.y, thickness, vp.height };

	case OverlayDockZone::RightToolbar:
		return { vp.right() - (f32)(stackIndex + 1) * thickness, vp.y, thickness, vp.height };

	default:
		return Rect();
	}
}

void overlayToolbarRender(OverlayToolbar* toolbar)
{
	if (!ctx || !s_overlayActive)
		return;

	if (!toolbar || !toolbar->visible)
		return;

	const auto& st = ctx->settings.overlayToolbars;
	f32 thickness = st.defaultThickness * ctx->scale;
	auto& mgr = s_manager;

	// dragged toolbars follow the mouse and highlight the dock zone under the cursor
	if (toolbar->dragging)
	{
		Point pos = toolbar->dragStartToolbarPos + (ctx->mousePosition - toolbar->dragStartMousePos);
		f32 dragW = std::max(toolbar->rect.width, 1.0f);
		f32 dragH = std::max(toolbar->rect.height, 1.0f);

		pos.x = std::max(s_viewportRect.x, std::min(pos.x, s_viewportRect.right() - dragW));
		pos.y = std::max(s_viewportRect.y, std::min(pos.y, s_viewportRect.bottom() - dragH));

		toolbar->floatingPosition = pos;
		toolbar->rect = { pos.x, pos.y, dragW, dragH };

		OverlayDockZone zone = OverlayDockZone::Floating;

		if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::RightToolbar, thickness))
			zone = OverlayDockZone::RightToolbar;
		else if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::LeftToolbar, thickness))
			zone = OverlayDockZone::LeftToolbar;
		else if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::BottomToolbar, thickness))
			zone = OverlayDockZone::BottomToolbar;
		else if (overlayToolbarPointInDockZone(s_viewportRect, ctx->mousePosition, OverlayDockZone::TopToolbar, thickness))
			zone = OverlayDockZone::TopToolbar;

		toolbar->showDockPreview = (zone != OverlayDockZone::Floating);
		toolbar->previewDockZone = zone;

		mgr.draggedToolbar = toolbar;
		mgr.showGlobalDockPreview = toolbar->showDockPreview;
		mgr.globalPreviewZone = zone;
		mgr.globalPreviewRect = toolbar->showDockPreview
			? overlayToolbarGetDockZoneRect(s_viewportRect, zone, thickness)
			: Rect();

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& ctx->event.mouse.button == MouseButton::Left)
		{
			toolbar->dockZone = toolbar->showDockPreview ? zone : OverlayDockZone::Floating;
			toolbar->dragging = false;
			toolbar->showDockPreview = false;

			mgr.draggedToolbar = nullptr;
			mgr.showGlobalDockPreview = false;

			ctx->widget.captureId = 0;
			ctx->widget.hoveredId = 0;
			ctx->event = InputEvent();
			forceRepaint();
		}
	}
	else if (toolbar->dockZone != OverlayDockZone::Floating)
	{
		toolbar->rect = dockedToolbarRect(toolbar, thickness);
	}
	else
	{
		toolbar->rect = { toolbar->floatingPosition.x, toolbar->floatingPosition.y, thickness, thickness };
	}

	OverlayToolbarLayout layout = toolbarEffectiveLayout(toolbar);
	f32 padding = st.elementPadding * ctx->scale;

	if (toolbar->collapsed)
	{
		auto& state = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
		f32 titleW = (state.font ? state.font->computeTextSize(toolbar->title.c_str()).width : 0.0f) + padding * 4.0f;

		if (toolbar->dockZone == OverlayDockZone::Floating)
		{
			if (layout == OverlayToolbarLayout::Vertical)
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
	else
	{
		Point content = layoutToolbarElements(toolbar, layout);

		if (toolbar->dockZone == OverlayDockZone::Floating)
		{
			toolbar->rect.width = std::max(content.x, thickness);
			toolbar->rect.height = std::max(content.y, thickness);
		}
	}

	drawToolbarBackground(toolbar);

	if (toolbar->dragging)
	{
		// keep the drag while it lasts, so later widgets cannot steal it
		ctx->widget.captureId = s_dragCaptureId;
		ctx->widget.hoveredId = s_dragCaptureId;

		if (toolbar->collapsed)
			drawToolbarTitle(toolbar);

		return;
	}

	if (toolbar->collapsed)
	{
		drawToolbarTitle(toolbar);
	}
	else
	{
		mgr.hoveredElement = nullptr;

		for (auto& e : toolbar->elements)
		{
			if (!e.visible)
				continue;

			bool clicked = false;

			if (e.type == OverlayToolbarElementType::Button || e.type == OverlayToolbarElementType::Toggle)
				clicked = processToolbarElement(toolbar, &e);

			drawToolbarElement(toolbar, &e);

			if (clicked && e.onClick)
				e.onClick();

			if (ctx->widget.hovered)
				mgr.hoveredElement = &e;

			if (!e.tooltip.empty())
				tooltip(e.tooltip.c_str());
		}
	}

	if (toolbarMouseOverToolbar(toolbar))
		mgr.hoveredToolbar = toolbar;

	// start dragging from the empty toolbar area, separators and spaces act as grab handles;
	// the drag only begins after the mouse moves past dragStartDistance
	if (ctx->event.type == InputEvent::Type::MouseDown
		&& ctx->event.mouse.button == MouseButton::Left
		&& toolbarMouseOverToolbar(toolbar)
		&& (toolbar->collapsed || !toolbarMouseOverInteractiveElement(toolbar)))
	{
		s_pendingDragToolbar = toolbar;
		s_pendingDragStartMouse = ctx->mousePosition;
		s_pendingDragPressed = true;

		ctx->widget.captureId = s_dragCaptureId;
		ctx->widget.hoveredId = s_dragCaptureId;
		ctx->alreadyClickedOnSomething = true;
		ctx->event = InputEvent();
		forceRepaint();
	}
	else if (s_pendingDragToolbar == toolbar && s_pendingDragPressed && !toolbar->dragging)
	{
		if (s_pendingDragStartMouse.getDistance(ctx->mousePosition) >= st.dragStartDistance * ctx->scale)
		{
			toolbar->dragging = true;
			toolbar->dockZone = OverlayDockZone::Floating;
			toolbar->dragStartMousePos = s_pendingDragStartMouse;

			// convert the docked extents into a floating content-sized rect
			OverlayToolbarLayout dragLayout = toolbarEffectiveLayout(toolbar);
			toolbar->rect = { toolbar->rect.x, toolbar->rect.y, thickness, thickness };

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
				toolbar->rect.width = std::max(content.x, thickness);
				toolbar->rect.height = std::max(content.y, thickness);
			}

			toolbar->dragStartToolbarPos = { toolbar->rect.x, toolbar->rect.y };

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