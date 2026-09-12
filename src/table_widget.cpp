#include "horus.h"
#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "util.h"
#include <algorithm>
#include <map>

namespace hui
{
static TableState& currentTable()
{
	if (ctx->tableStack.empty())
	{
		static TableState dummy;
		return dummy;
	}

	return ctx->tableStack.back();
}

TablePersistentState::TablePersistentState()
{
	splitter = new DrawCmdLayerSplitter();
}

TablePersistentState::~TablePersistentState()
{
	delete splitter;
}

static void finishRow(TableState& state)
{
	// resolve any pending sameLine state so position.y reflects the true bottom of content.
	// this happens when the last widget in the cell used sameLine() (e.g. vec3Editor).
	if (ctx->sameLine.wasEnabled)
	{
		ctx->position.x = ctx->sameLine.currentPosition.x;
		ctx->sameLine.wasEnabled = false;
		ctx->position.y += ctx->sameLine.maxHeight;
		ctx->sameLine.maxHeight = 0;
		ctx->sameLine.currentPosition.y = ctx->position.y;
	}

	// calculate height of the last cell in the row
	// note: ctx->position.y points to where the next widget would go, so it represents the bottom of content
	f32 lastCellHeight = ctx->position.y - state.cellStartY + ctx->cellPadding.y;
	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, lastCellHeight);

	// ensure row has at least the minimum height (e.g. from theme)
	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, state.rowHeight);

	// draw background for this row using deferred drawing
	// note: If we are in the header, startHeader already drew the background and borders.
	// we should only draw here for body rows.
	if (!state.isInHeader)
	{
		f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
		
		// draw custom row color if set
		if (state.currentRowColorSet)
		{
			Rect rowRect(
				baseX,
				state.rowStartY,
				state.innerWidth,
				state.currentMaxRowHeight
			);

			rowRect = rowRect.contract(1.0);

			ctx->renderer.cmdSetColor(state.currentRowColor);
			// skip splitter when inside scroll view to respect clip rect
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(0);
		
			ctx->renderer.cmdDrawFilledRectangle(rowRect);
			
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(1);
		}
		// draw alternating row background if enabled and no custom color
		else if (has(state.flags, TableFlags::AltRowBg))
		{
			Rect rowRect(
				baseX,
				state.rowStartY,
				state.innerWidth,
				state.currentMaxRowHeight
			);

			rowRect = rowRect.contract(1.0);
			auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);

			// switch to background layer (0), skip splitter when inside scroll view
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(0);

			if (state.currentRow % 2 == 1)
			{
				ctx->renderer.cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor0", Color::transparent));
			}
			else
			{
				ctx->renderer.cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor1", Color::transparent));
			}

			ctx->renderer.cmdDrawFilledRectangle(rowRect);

			// switch back to content layer (1)
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(1);
		}

		// draw pending cell backgrounds (on top of row background)
		if (!state.cellColorRequests.empty())
		{
			state.persistent->splitter->setLayer(0);
			
			for (const auto& req : state.cellColorRequests)
			{
				f32 cx = baseX;
				f32 cw = 0;
				
				// find column x and width
				for (u32 i = 0; i < state.persistent->columns.size(); i++)
				{
					if (state.persistent->columns[i].isHidden)
						continue;
				
					if (i == req.columnIndex)
					{
						cw = state.persistent->columns[i].width;
						break;
					}

					cx += state.persistent->columns[i].width;
				}

				if (cw > 0)
				{
					Rect cellRect(cx, state.rowStartY, cw, state.currentMaxRowHeight);
					
					cellRect = cellRect.contract(1.0f);
					ctx->renderer.cmdSetColor(req.color);
					ctx->renderer.cmdDrawFilledRectangle(cellRect);
				}
			}

			state.persistent->splitter->setLayer(1);
		}

		// clear requests for next row
		state.cellColorRequests.clear();
		state.rowSeparators.push_back(state.rowStartY + state.currentMaxRowHeight);

		// reset row color flag
		state.currentRowColorSet = false;

		// advance Y position ONLY if we actually finished a row (not header)
		state.currentRowY += state.currentMaxRowHeight;

		// respect external advances (virtual list). If ctx->position.y already moved past our computed row end,
		// adopt the external position instead of forcing ctx->position.y backwards.

		if (ctx->position.y < state.currentRowY)
			ctx->position.y = state.currentRowY;
		else
			state.currentRowY = ctx->position.y;
	}
	else
	{
		// header drawing
		f32 headerHeight = state.currentMaxRowHeight;

		state.headerRect = Rect(
			state.tableRect.x,
			state.tableRect.y,
			state.innerWidth,
			headerHeight
		);

		auto& bodyElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);
		auto& bodyElemState = bodyElem.normalState();
		auto& headerLeftElem = ctx->theme->getElement(WidgetElementId::TableHeaderBodyLeft);
		auto& headerRightElem = ctx->theme->getElement(WidgetElementId::TableHeaderBodyRight);
		auto& headerLeftState = headerLeftElem.normalState();
		auto& headerRightState = headerRightElem.normalState();

		// draw header background per column: first, middle and last columns use left, middle and right elements
		state.persistent->splitter->setLayer(0);
		{
			u32 visibleCount = 0;

			for (const auto& col : state.persistent->columns)
				if (!col.isHidden) visibleCount++;

			f32 x = state.tableRect.x;
			u32 visibleIndex = 0;

			for (u32 i = 0; i < state.persistent->columns.size(); i++)
			{
				if (state.persistent->columns[i].isHidden)
					continue;

				const auto& elemState = visibleIndex == 0 ? headerLeftState
					: visibleIndex == visibleCount - 1 ? headerRightState
					: bodyElemState;
				f32 columnWidth = state.persistent->columns[i].width;
				Rect cellRect(x, state.tableRect.y, columnWidth, headerHeight);

				ctx->renderer.cmdSetColor(tintApply(elemState.color, TintColorType::Body));
				ctx->renderer.cmdDrawImageBordered(elemState.image, elemState.border, cellRect, ctx->scale);

				x += columnWidth;
				visibleIndex++;
			}

			// fill remaining width after the last column with the right element
			if (x < state.headerRect.right())
			{
				Rect cellRect(x, state.tableRect.y, state.headerRect.right() - x, headerHeight);

				ctx->renderer.cmdSetColor(tintApply(headerRightState.color, TintColorType::Body));
				ctx->renderer.cmdDrawImageBordered(headerRightState.image, headerRightState.border, cellRect, ctx->scale);
			}
		}

		// header pending cell backgrounds? Usually not used, but supported just in case
		if (!state.cellColorRequests.empty())
		{
			for (const auto& req : state.cellColorRequests)
			{
				f32 cx = state.tableRect.x;
				f32 cw = 0;
			
				for (u32 i = 0; i < state.persistent->columns.size(); i++)
				{
					if (state.persistent->columns[i].isHidden)
						continue;
					
					if (i == req.columnIndex)
					{
						cw = state.persistent->columns[i].width;
						break;
					}
					
					cx += state.persistent->columns[i].width;
				}

				if (cw > 0)
				{
					Rect cellRect(cx, state.tableRect.y, cw, headerHeight);
					
					cellRect = cellRect.contract(1.0f);
					ctx->renderer.cmdSetColor(req.color);
					ctx->renderer.cmdDrawFilledRectangle(cellRect);
				}
			}

			state.cellColorRequests.clear();
		}

		state.persistent->splitter->setLayer(1);

		// draw vertical separators between columns (only if BordersV or compatible flags are set)
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) ||
			has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersV))
		{
			f32 currentX = state.headerRect.x;
			bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
			bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);
			auto& tableHeaderElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);

			ctx->renderer.cmdSetLineStyle(LineStyle(tableHeaderElem.currentStyle->getColorParameter("borderColorV", Color::white), 1.0f));

			for (u32 i = 0; i < state.persistent->columns.size(); i++)
			{
				if (!state.persistent->columns[i].isHidden)
				{
					// draw left line for this column
					// skip the first column's left line only if:
					// - we already drew it as the leftmost outer border (hasOuter and i==0)
					// - or we're in inner-only mode and it's the first column
					bool drawLeftLine = true;
					
					if (i == 0 && (hasOuter || innerOnly))
						drawLeftLine = false;

					// skip boundaries covered by a colspan cell in the header
					if (drawLeftLine
						&& std::find(state.headerColspanBoundaries.begin(), state.headerColspanBoundaries.end(), i)
							!= state.headerColspanBoundaries.end())
						drawLeftLine = false;

					if (drawLeftLine)
					{
						ctx->renderer.cmdDrawLine(
							Point(currentX, state.headerRect.y),
							Point(currentX, state.headerRect.y + state.headerRect.height)
						);
					}

					currentX += state.persistent->columns[i].width;
				}
			}

			// draw Rightmost line after all columns (skip if only inner borders)
			if (!innerOnly && !hasOuter)
			{
				// draw Right line for last column
				ctx->renderer.cmdDrawLine(
					Point(currentX, state.headerRect.y),
					Point(currentX, state.headerRect.y + state.headerRect.height)
				);
			}
		}

		// draw header bottom border AFTER vertical separators (only if BordersH or compatible flags are set)
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) ||
			has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersH))
		{
			auto& tableHeaderElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);
			
			ctx->renderer.cmdSetLineStyle(LineStyle(tableHeaderElem.currentStyle->getColorParameter("borderColorH", Color::white), 1.0f));
			ctx->renderer.cmdDrawLine(
				Point(state.headerRect.x, state.headerRect.y + state.headerRect.height),
				Point(state.headerRect.x + state.headerRect.width, state.headerRect.y + state.headerRect.height)
			);
		}

		state.currentRowY += headerHeight;
		state.bodyStartY = state.currentRowY;

		// respect external advances (virtual list) for header as well
		if (ctx->position.y < state.currentRowY)
			ctx->position.y = state.currentRowY;
		else
			state.currentRowY = ctx->position.y;

		// start scroll view for the body content
		// enable scroll view if height > 0 or ScrollY flag is set
		// height == 0 means auto-grow without scroll view
		f32 scrollViewHeight = state.innerHeight > 0 ? state.innerHeight : 200.0f;

		// scroll view doesn't scale height by default, so keep it single scaled
		if (ctx->settings.scaleScrollViewHeight)
			scrollViewHeight /= ctx->scale;

		if (state.innerHeight > 0)
		{
			// use NoPadding so the body content starts exactly at the table edge
			// and spans the full table width (clipped only by the vertical scrollbar)
			ctx->position.x = state.tableRect.x;
			ctx->layout.width = state.innerWidth;
			
			scrollViewBegin("##tableScrollView", scrollViewHeight, state.persistent->scrollViewScrollPos.y, 0.0f,
				ScrollViewFlags::NoBorder | ScrollViewFlags::NoHorizontalScroll | ScrollViewFlags::NoPadding);
			state.needsScrollViewStart = true;
			
			// after scroll view starts, ctx->position.x should now align with table edge
			state.scrollViewBaseX = ctx->position.x;
			state.currentRowY = ctx->position.y;
			state.rowStartY = ctx->position.y;
			state.bodyStartY = ctx->position.y;
		}
	}
}

bool tableBegin(const char* id, u32 columnCount, f32 height, TableFlags flags)
{
	if (columnCount == 0)
		return false;

	widgetPushDisabled(widgetGetDisabled());
	addWidget(0);

	WidgetId tableId = genId(id);
	auto& persistent = ctx->tablePersistentStates[tableId];

	// initialize persistent state if needed
	if (!persistent.initialized || persistent.columns.size() != columnCount)
	{
		persistent.columns.resize(columnCount);
		
		for (u32 i = 0; i < columnCount; i++)
		{
			persistent.columns[i].width = 100.0f;
			persistent.columns[i].isHidden = false;
			persistent.columns[i].isStretchable = has(flags, TableFlags::Stretch);
		}
		
		persistent.initialized = true;
	}

	// apply active resize BEFORE calculating widths
	if (persistent.resizingColumn && persistent.resizingColumnIndex < columnCount)
	{
		u32 i = persistent.resizingColumnIndex;
		f32 idealDelta = ctx->mousePosition.x - persistent.resizeStartX;
		f32 leftStart = persistent.resizeStartWidth;

		// clamp delta against min width (10px); growing is unconstrained,
		// only the dragged column's width changes
		f32 maxNegativeDelta = -(leftStart - 10.0f);

		if (idealDelta < maxNegativeDelta) idealDelta = maxNegativeDelta;

		persistent.columns[i].specifiedSize = leftStart + idealDelta;
	}

	// create new transient table state
	TableState state;
	
	state.id = tableId;
	state.persistent = &persistent;
	state.currentColumn = 0;
	// state.columns removed
	state.isInHeader = false;
	state.currentRow = 0;
	state.flags = flags;
	state.headerRect = Rect(0,0,0,0);
	state.rowSeparators.clear(); // clear separate list
	state.savedLayoutWidth = ctx->layout.width; // save layout width to restore later


	// calculate total used width from columns
	f32 totalColumnsWidth = 0;

	for (const auto& col : persistent.columns)
	{
		if (!col.isHidden)
			totalColumnsWidth += col.width;
	}

	// get widget width (use available width if not set)
	f32 widgetWidth = ctx->layout.width;

	// account for left and right borders (2px total) when Borders or BordersOuter flags are set
	bool hasBorders = has(flags, TableFlags::Borders) || has(flags, TableFlags::BordersOuter);

	if (hasBorders)
		widgetWidth -= 2.0f;

	// when the body starts a scroll view (height > 0), it always reserves space for the
	// vertical scrollbar on the right, so columns must be sized within the remaining width
	// to keep cell widgets from sliding under the scrollbar.
	f32 columnWidth = widgetWidth;

	if (height > 0)
	{
		auto& scrollBarElemV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		columnWidth -= scrollBarElemV.width * ctx->scale;
	}

	// apply column size specifications (percentage, pixels, or fill)
	f32 specifiedWidth = 0; // total width of columns with specific sizes
	u32 fillCount = 0; // number of columns that fill remaining space

	for (u32 i = 0; i < columnCount; i++)
	{
		// use persistent column for everything
		auto& col = persistent.columns[i];

		if (col.isHidden) continue;

		bool isFixed = static_cast<bool>(col.flags & TableColumnFlags::Fixed) || static_cast<bool>(col.flags & TableColumnFlags::FixedResize);

		if (col.isFillRemaining && !isFixed)
		{
			fillCount++;
		}
		else if (col.specifiedSize > 0)
		{
			if (col.isPercentage && !isFixed)
			{
				// percentage of table width (0..1)
				col.width = columnWidth * col.specifiedSize;
			}
			else
			{
				// fixed pixel size
				col.width = col.specifiedSize;
			}

			specifiedWidth += col.width;
		}
		else
		{
			// use default width from persistent state
			specifiedWidth += col.width;
		}
	}

	// calculate explicit weights for fill columns
	f32 totalExplicitWeight = 0.0f;
	u32 pureFillCount = 0;

	for (u32 i = 0; i < columnCount; i++)
	{
		auto& col = persistent.columns[i];

		if (col.isHidden) continue;

		bool isFixed = static_cast<bool>(col.flags & TableColumnFlags::Fixed) || static_cast<bool>(col.flags & TableColumnFlags::FixedResize);

		if (col.isFillRemaining && !isFixed)
		{
			if (col.specifiedSize > 0 && col.specifiedSize <= 1.0f)
			{
				totalExplicitWeight += col.specifiedSize;
			}
			else
			{
				pureFillCount++;
			}
		}
	}

	// distribute remaining space to fill columns
	if (fillCount > 0)
	{
		f32 remainingWidth = columnWidth - specifiedWidth;
		
		if (remainingWidth > 0)
		{
			// verify if we need to normalize weights or share remaining space
			f32 weightNormalizer = 1.0f;
			f32 weightForPureFills = 0.0f;

			if (pureFillCount == 0)
			{
				// only explicit weights: Normalize them to fill the space
				if (totalExplicitWeight > 0)
					weightNormalizer = 1.0f / totalExplicitWeight;
			}
			else
			{
				// mixed explicit and pure: Pure fills divide the remaining weight
				// (e.g. 1.0 - 0.7 = 0.3 for pure fills)
				if (totalExplicitWeight < 1.0f)
					weightForPureFills = (1.0f - totalExplicitWeight);
			}

			f32 widthPerPureFill = 0;

			if (pureFillCount > 0)
				widthPerPureFill = (remainingWidth * weightForPureFills) / pureFillCount;

			for (u32 i = 0; i < columnCount; i++)
			{
				if (!persistent.columns[i].isHidden && persistent.columns[i].isFillRemaining)
				{
					if (persistent.columns[i].specifiedSize > 0 && persistent.columns[i].specifiedSize <= 1.0f)
					{
						// weighted fill
						persistent.columns[i].width = remainingWidth * (persistent.columns[i].specifiedSize * weightNormalizer);
					}
					else
					{
						// pure fill
						persistent.columns[i].width = widthPerPureFill;
					}
				}
			}
		}
	}

	// recalculate total width after applying specifications
	totalColumnsWidth = 0;

	for (const auto& col : persistent.columns)
	{
		if (!col.isHidden)
			totalColumnsWidth += col.width;
	}

	// collapse logic: if content exceeds widget width, scale down proportionally (respecting min width)
	if (!has(flags, TableFlags::FixedSize) && totalColumnsWidth > columnWidth && columnWidth > 0)
	{
		f32 totalFixed = 0;
		f32 totalFlexible = 0;

		for (u32 i = 0; i < columnCount; i++)
		{
			if (persistent.columns[i].isHidden) continue;

			bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
						   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

			if (isFixed)
				totalFixed += persistent.columns[i].width;
			else
				totalFlexible += persistent.columns[i].width;
		}

		if (totalFixed < columnWidth)
		{
			// we have room for fixed columns, shrink flexible ones
			f32 scale = 0.0f;

			if (totalFlexible > 0)
				scale = (columnWidth - totalFixed) / totalFlexible;

			for (u32 i = 0; i < columnCount; i++)
			{
				if (persistent.columns[i].isHidden) continue;

				bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
							   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

				if (!isFixed)
				{
					persistent.columns[i].width *= scale;
					
					// apply min width floor
					if (persistent.columns[i].width < 1.0f) persistent.columns[i].width = 1.0f;
				}
			}
		}
		else
		{
			// fixed columns alone take up too much space.
			for (u32 i = 0; i < columnCount; i++)
			{
				if (persistent.columns[i].isHidden) continue;

				bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
							   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

				if (!isFixed)
				{
					persistent.columns[i].width = 1.0f; // collapse flexible to minimum
				}
			}
		}

		// re-sum for final total
		totalColumnsWidth = 0;
		
		for (auto& col : persistent.columns)
		{
			if (!col.isHidden)
				totalColumnsWidth += col.width;
		}
	}

	// stretch logic: distribute space proportionally based on column widths
	if (has(flags, TableFlags::Stretch) && columnWidth != totalColumnsWidth)
	{
		// calculate total width of stretchable columns
		f32 stretchableWidth = 0;
		
		for (const auto& col : persistent.columns)
			if (col.isStretchable && !col.isHidden)
				stretchableWidth += col.width;

		if (stretchableWidth > 0)
		{
			// calculate the target width for stretchable columns
			f32 nonStretchableWidth = 0;
			
			for (const auto& col : persistent.columns)
				if (!col.isStretchable && !col.isHidden)
					nonStretchableWidth += col.width;

			f32 availableWidth = columnWidth - nonStretchableWidth;

			// scale each stretchable column proportionally
			for (auto& col : persistent.columns)
			{
				if (col.isStretchable && !col.isHidden)
				{
					f32 proportion = col.width / stretchableWidth;
					col.width = availableWidth * proportion;
				}
			}
		}

		totalColumnsWidth = columnWidth;
	}

	// final Table Width Logic
	// if FixedSize is NOT set, the table conforms to layout width (fills space).
	// if FixedSize IS set, it uses the sum of column widths.
	if (!has(flags, TableFlags::FixedSize))
	{
		if (widgetWidth > totalColumnsWidth)
			totalColumnsWidth = widgetWidth;
	}

	state.innerWidth = totalColumnsWidth;
	// scale height so the scroll body, borders and resize guides match scaled rows
	state.innerHeight = height * ctx->scale; // if 0, auto height

	// store table rectangle start
	// start 1px to the right to leave room for the left border
	state.tableRect = Rect(
		ctx->position.x + 1.0f,
		ctx->position.y,
		totalColumnsWidth,
		state.innerHeight
	);

	state.rowStartY = state.tableRect.y;
	state.bodyStartY = state.tableRect.y; // default start if no header
	state.currentRowY = state.tableRect.y;
	state.currentMaxRowHeight = 0;

	// push state so we can use it
	ctx->tableStack.push_back(state);

	// get row height from theme
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
	auto& bodyElemState = bodyElem.normalState();

	ctx->tableStack.back().rowHeight = bodyElem.currentStyle->getParameter("rowHeight", 25);

	// push a clip rect for the entire table to prevent backgrounds from extending too far
	// start 1px to the left to include the left border, and add 2px to width for both borders
	auto& currentState = ctx->tableStack.back();
	Rect tableClipRect(currentState.tableRect.x - 1.0f, currentState.tableRect.y, currentState.innerWidth + 2.0f, 10000.0f);
	
	ctx->renderer.pushClipRect(tableClipRect);
	currentState.hasTableClip = true;
	ctx->tableStack.back().hasTableClip = true;

	// init splitter: Layer 0 = Background, Layer 1 = Content
	currentState.persistent->splitter->clear();
	currentState.persistent->splitter->split(2);
	currentState.persistent->splitter->setLayer(1);

	// store that we need to begin scroll view after header
	currentState.needsScrollViewStart = false;

	return true;
}

void tableEnd()
{
	if (ctx->tableStack.empty()) return;
	auto& state = ctx->tableStack.back();

	// finish the last row
	finishRow(state);

	f32 finalHeight = state.currentRowY - state.tableRect.y;

	// pop any remaining clip rect BEFORE drawing borders so they don't get clipped
	if (state.isClipping)
	{
		ctx->renderer.popClipRect();
		state.isClipping = false;
	}
	
	// draw borders for body rows AFTER clip rect is popped but BEFORE ending scroll view
	{
		auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
		
		// determine which color to use based on border type (inner vs outer)
		Color innerHColor = tableBodyElem.currentStyle->getColorParameter("innerBorderColorH", Color::white);
		Color innerVColor = tableBodyElem.currentStyle->getColorParameter("innerBorderColorV", Color::white);
		Color outerVColor = tableBodyElem.currentStyle->getColorParameter("outerBorderColorV", Color::white);
		
		// draw borders if enabled
		if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::BordersV) || has(state.flags, TableFlags::BordersH))
		{
			f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
			
			// draw Inner Horizontal Lines
			if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersH))
			{
				ctx->renderer.cmdSetLineStyle(LineStyle(innerHColor, 1.0f));

				for (size_t i = 0; i < state.rowSeparators.size(); i++)
				{
					bool draw = true;
					// skip last line if only inner borders (not outer) or if Borders flag is set
					if (i == state.rowSeparators.size() - 1)
					{
						// skip bottom line if we're only drawing inner borders (no outer borders)
						if (has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter))
							draw = false;
						else if (has(state.flags, TableFlags::Borders))
							draw = false; // outer border will draw it
					}
					
					if (draw)
					{
						ctx->renderer.cmdDrawLine(
							Point(baseX, state.rowSeparators[i]),
							Point(baseX + state.innerWidth, state.rowSeparators[i])
						);
					}
				}
			}
			
			// draw Vertical Lines (Inner + Outer Left/Right)
			if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersV))
			{
				ctx->renderer.cmdSetLineStyle(LineStyle(innerVColor, 1.0f));

				bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
				bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);
				
				// draw vertical lines for each row
				for (u32 rowIdx = 0; rowIdx < state.rowSeparators.size() + 1; rowIdx++)
				{
					// start the band at the previous row's separator so the
					// extra final band stays zero-height (otherwise it would
					// re-draw skipped colspan lines over the whole body)
					f32 lineStartY = rowIdx > 0 ? state.rowSeparators[rowIdx - 1] : state.bodyStartY;
					f32 lineEndY = rowIdx < state.rowSeparators.size() ? state.rowSeparators[rowIdx] : state.currentRowY;
					f32 currentX = baseX;
					
					// draw vertical lines between columns
					for (u32 i = 0; i < state.persistent->columns.size(); i++)
					{
						if (!state.persistent->columns[i].isHidden)
						{
							// draw left line for this column
							bool drawLeftLine = true;

							if (i == 0 && (hasOuter || innerOnly))
								drawLeftLine = false;

							// skip boundaries covered by a colspan cell in this row
							if (drawLeftLine && rowIdx < state.rowColspanBoundaries.size()
								&& std::find(state.rowColspanBoundaries[rowIdx].begin(), state.rowColspanBoundaries[rowIdx].end(), i)
									!= state.rowColspanBoundaries[rowIdx].end())
								drawLeftLine = false;

							if (drawLeftLine)
							{
								ctx->renderer.cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
							}

							currentX += state.persistent->columns[i].width;
						}
					}

					// draw Rightmost line after all columns (skip if only inner borders or if handled by outer box)
					if (!innerOnly && !hasOuter)
					{
						ctx->renderer.cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
						ctx->renderer.cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
					}
				}
			}
		}
	}

	// end scroll view if it was started
	if (state.needsScrollViewStart)
	{
		state.persistent->scrollViewScrollPos = scrollViewEnd();
	}
	
	// handle column resizing
	if (has(state.flags, TableFlags::Resizable) && !ctx->widget.disabled)
	{
		// only process resize logic if mouse is within the table's visible bounds
		// this prevents triggering when hovering over other widgets below the table
		f32 visibleHeight = state.needsScrollViewStart ? 
			(state.headerRect.height + (state.innerHeight > 0 ? state.innerHeight : 200.0f)) : 
			finalHeight;

		if (ctx->currentWindow
			&& state.tableRect.y + finalHeight > ctx->currentWindow->clientRect.bottom())
		{
			// clip visible height to window bottom
			visibleHeight = ctx->currentWindow->clientRect.bottom() - state.tableRect.y;
		}
		
		// check if mouse is within table bounds AND hovering this window
		if (!ctx->hoveringThisWindow || 
			ctx->mousePosition.y < state.tableRect.y || 
			ctx->mousePosition.y > state.tableRect.y + visibleHeight)
		{
			// mouse is outside table bounds or not hovering this window, skip resize handling
		}
		else
		{
			auto& persistent = *state.persistent;
			f32 currentX = state.tableRect.x;
			f32 separatorWidth = 4.0f;

			for (u32 i = 0; i < persistent.columns.size(); i++)
			{
				if (persistent.columns[i].isHidden) continue;

				currentX += persistent.columns[i].width;

				if (persistent.resizingColumn && persistent.resizingColumnIndex == i)
				{
					ctx->mouseCursor = MouseCursorType::SizeWE;

					if (ctx->event.type == InputEvent::Type::MouseUp || ctx->event.type == InputEvent::Type::WindowLostFocus)
					{
						windowReleaseCapture();
						ctx->widget.captureId = 0;
						persistent.resizingColumn = false;
						persistent.resizingColumnIndex = ~0;
					}
					else
					{
						// draw Resize Guide Line - width already applied in beginTable
						f32 guideLineX = currentX;
						// clip line to scroll view bounds if inside scroll view
						// use headerRect.height to be safe (or calculate it)
						f32 svHeight = (state.innerHeight > 0 ? state.innerHeight : 200.0f);
						f32 lineBottomY = state.needsScrollViewStart ? 
							(state.tableRect.y + state.headerRect.height + svHeight) :
							(state.tableRect.y + finalHeight);
						auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);

						ctx->renderer.cmdSetLineStyle(LineStyle(tableBodyElem.currentStyle->getColorParameter("columnResizeLineColor", Color(0.0f, 1.0f, 1.0f, 1.0f)), 2.0f));
						ctx->renderer.cmdDrawLine(Point(guideLineX, state.tableRect.y),
												   Point(guideLineX, lineBottomY));

						// only the dragged column changes; others keep their widths
						if (!(static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed)))
						{
							f32 idealDelta = ctx->mousePosition.x - persistent.resizeStartX;
							f32 leftStart = persistent.resizeStartWidth;

							// clamp delta against min width (10px); growing is unconstrained
							f32 maxNegativeDelta = -(leftStart - 10.0f);

							if (idealDelta < maxNegativeDelta) idealDelta = maxNegativeDelta;

							persistent.columns[i].specifiedSize = leftStart + idealDelta;
						}
					}
				}
				else if (!persistent.resizingColumn)
				{
					// only allow NEW interaction if guards pass
					if (!(static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed)) &&
						ctx->widget.captureId == 0)
					{
						// limit separator height to visible table area
						f32 separatorHeight = finalHeight;

						if (state.needsScrollViewStart)
						{
							f32 svHeight = (state.innerHeight > 0 ? state.innerHeight : 200.0f);
							separatorHeight = state.headerRect.height + svHeight;
						}

						Rect separatorRect(currentX - separatorWidth, state.tableRect.y, separatorWidth * 2.0f, separatorHeight);

						// determine if this separator is covered by a column span in the row under the mouse
						bool isSeparatorCovered = false;

						// find which row the mouse is in
						i32 hoveredRowIndex = -1;
						f32 mouseY = ctx->mousePosition.y;

						if (state.rowSeparators.empty())
						{
							// fallback for single row table if logic failed elsewhere
							if (mouseY >= state.bodyStartY && mouseY < state.currentRowY)
								hoveredRowIndex = 0;
						}
						else
						{
							for (size_t r = 0; r < state.rowSeparators.size(); r++)
							{
								f32 rowTop = (r == 0) ? state.bodyStartY : state.rowSeparators[r - 1];
								f32 rowBottom = state.rowSeparators[r];

								if (mouseY >= rowTop && mouseY <= rowBottom)
								{
									hoveredRowIndex = (i32)r;
									break;
								}
							}
						}

						if (isSeparatorCovered && separatorRect.contains(ctx->mousePosition))
						{
							ctx->mouseCursor = MouseCursorType::Arrow;
						}
						else if (separatorRect.contains(ctx->mousePosition))
						{
							ctx->mouseCursor = MouseCursorType::SizeWE;

							// draw Hover Guide Line
							// clip line to scroll view bounds if inside scroll view
							f32 svHeight = (state.innerHeight > 0 ? state.innerHeight : 200.0f);
							f32 lineBottomY = state.needsScrollViewStart ? 
								(state.tableRect.y + state.headerRect.height + svHeight) :
								(state.tableRect.y + finalHeight);						
							auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);

							ctx->renderer.cmdSetLineStyle(LineStyle(tableBodyElem.currentStyle->getColorParameter("columnResizeLineColor", Color::cyan), 2.0f));
							ctx->renderer.cmdDrawLine(Point(currentX, state.tableRect.y),
													   Point(currentX, lineBottomY));

							if (ctx->event.type == InputEvent::Type::MouseDown && ctx->event.mouse.button == MouseButton::Left)
							{
								windowSetCapture();
								ctx->widget.captureId = state.id;
								persistent.resizingColumn = true;
								persistent.resizingColumnIndex = i; // store SEPARATOR index
								persistent.resizeStartX = ctx->mousePosition.x; // store Absolute Start X
								persistent.resizeStartWidth = persistent.columns[i].width;

								// synchronize ALL columns to their current visual width to prevent jumps,
								// and lock them to fixed pixel sizes so only the dragged column changes
								for (u32 k = 0; k < persistent.columns.size(); k++)
								{
									if (!persistent.columns[k].isHidden)
									{
										persistent.columns[k].specifiedSize = persistent.columns[k].width;
										persistent.columns[k].isPercentage = false;
										persistent.columns[k].isFillRemaining = false;
										persistent.columns[k].userResized = true;
									}
								}
							}
						}
					}
				}
			}
		} // end of mouse-in-bounds check
	}

	auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);

	// pop table clip rect if it was pushed
	if (state.hasTableClip)
	{
		ctx->renderer.popClipRect();
	}

	// merge layers: Background (0) and Content (1)
	state.persistent->splitter->merge();

	// draw outer box for entire table (header + body) if borders are enabled
	// drawn AFTER merge so borders render on top of all table content
	if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter))
	{
		Color outerHColor = tableBodyElem.currentStyle->getColorParameter("outerBorderColorH", Color::white);
		Color outerVColor = tableBodyElem.currentStyle->getColorParameter("outerBorderColorV", Color::white);
		
		f32 borderHeight = state.needsScrollViewStart ? 
			(state.headerRect.height + (state.innerHeight > 0 ? state.innerHeight : 200.0f)) :
			finalHeight;
		
		ctx->renderer.cmdSetLineStyle(LineStyle(outerHColor, 1.0f));
		ctx->renderer.cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
								   Point(state.tableRect.x + state.innerWidth, state.tableRect.y));
		ctx->renderer.cmdDrawLine(Point(state.tableRect.x, state.tableRect.y + borderHeight),
								   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + borderHeight));

		ctx->renderer.cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
		ctx->renderer.cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
								   Point(state.tableRect.x, state.tableRect.y + borderHeight));
		ctx->renderer.cmdDrawLine(Point(state.tableRect.x + state.innerWidth, state.tableRect.y),
								   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + borderHeight));
	}

	// restore layout width and cursor X position
	// we want the cursor to be at the start of the layout (left indentation) for the next widget
	// the table started at state.tableRect.x - 1.0f (since we added 1px shift left).
	// so we restore it to that.
	ctx->layout.width = state.savedLayoutWidth;
	ctx->position.x = state.tableRect.x - 1.0f;
	ctx->tableStack.pop_back();

	widgetPopDisabled();
}

void tableStartHeader()
{
	auto& state = currentTable();
	state.isInHeader = true;
	state.currentColumn = 0;
	state.headerColspanBoundaries.clear();
	// reset row parameters
	state.rowStartY = state.currentRowY;
	state.currentMaxRowHeight = state.rowHeight; // use theme default height as min
	// setup for first cell
	state.cellStartY = state.rowStartY;

	if (state.currentColumn < state.persistent->columns.size())
	{
		ctx->layout.width = state.persistent->columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
		ctx->position.x = state.tableRect.x + ctx->cellPadding.x;
		ctx->position.y = state.rowStartY + ctx->cellPadding.y;

		// start clipping for first cell
		if (state.isClipping) ctx->renderer.popClipRect(); // should not happen here usually, but safe
		
		Rect clipRect(state.tableRect.x, state.rowStartY, state.persistent->columns[state.currentColumn].width, 99999.0f);
		
		ctx->renderer.pushClipRect(clipRect);
		state.isClipping = true;
	}
}

void tableRowNext()
{
	auto& state = currentTable();

	// pop clip rect from previous cell in previous row
	if (state.isClipping)
	{
		ctx->renderer.popClipRect();
		state.isClipping = false;
	}

	// finish the header when leaving it, and finish the previous body row
	// (skip only when there is no previous row nor an open header to finish)
	if (state.isInHeader || state.currentRow > 0)
		finishRow(state);

	// if an external system (e.g. VirtualScrollInfo) moved ctx->position.y forward to skip items,
	// synchronize the table's internal currentRowY so subsequent rows start at the correct Y.
	// also record a separator for the skipped region so borders/vertical lines and clipping are correct.
	if (ctx->position.y > state.currentRowY)
	{
		// only push a separator if it increases the list (avoid duplicates)
		if (state.rowSeparators.empty() || ctx->position.y > state.rowSeparators.back())
		{
			// record the virtual-skip boundary so drawing code knows there was a gap
			state.rowSeparators.push_back(ctx->position.y);
			// the skipped region has no rows, so no colspan covers it
			state.rowColspanBoundaries.emplace_back();
		}

		state.currentRowY = ctx->position.y;
	}

	// start new row
	state.currentRow++;
	state.currentColumn = 0;
	state.isInHeader = false;
	state.rowStartY = state.currentRowY;
	state.currentMaxRowHeight = state.rowHeight; // use theme default height as min
	state.cellStartY = state.rowStartY;
	// one colspan boundary set per row band, aligned with rowSeparators
	state.rowColspanBoundaries.emplace_back();

	// row background drawing REMOVED, deferred to finishRow
	// custom row color and alt row color are handled in finishRow

	// setup for first cell
	if (state.currentColumn < state.persistent->columns.size())
	{
		ctx->layout.width = state.persistent->columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
		
		// use scrollViewBaseX if scroll view is active, otherwise use tableRect.x
		f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
		ctx->position.x = baseX + ctx->cellPadding.x;
		ctx->position.y = state.rowStartY + ctx->cellPadding.y;

		// start clipping
		// note: When inside scroll view, we don't need per-cell clipping as scroll view already clips
		// per-cell clipping can cause issues with left edge being clipped
		if (!state.needsScrollViewStart)
		{
			Rect clipRect(baseX, state.rowStartY, state.persistent->columns[state.currentColumn].width, 99999.0f);
			ctx->renderer.pushClipRect(clipRect);
			state.isClipping = true;
		}
	}
}

void tableCellNext(u32 columnSpan)
{
	auto& state = currentTable();

	// handle end of same-line if it was active (similar to addWidget)
	if (ctx->sameLine.wasEnabled)
	{
		ctx->position.x = ctx->sameLine.currentPosition.x;
		ctx->sameLine.wasEnabled = false;
		// add the previous line max height
		ctx->position.y += ctx->sameLine.maxHeight;
		ctx->sameLine.maxHeight = 0;
		ctx->sameLine.currentPosition.y = ctx->position.y;
	}

	// pop previous clip
	if (state.isClipping)
	{
		ctx->renderer.popClipRect();
		state.isClipping = false;
	}

	if (state.currentColumn >= state.persistent->columns.size())
		return;

	// calculate height of the cell we just finished
	// note: ctx->position.y points to where the next widget would go, so it represents the bottom of content
	// position.y already includes the top padding we added at start of cell, so we only need to add bottom padding
	f32 finishedCellHeight = ctx->position.y - state.cellStartY + ctx->cellPadding.y;

	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, finishedCellHeight);

	// reset cell color flag for previous cell
	state.currentCellColorSet = false;

	f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;

	// advance to the cell that gets set up for the next widget.
	// a normal cell (span 1) starts right after the cell we just finished,
	// a spanning cell (columnSpan > 1) starts at the finished column and covers the next columns.
	u32 spanStart = state.currentColumn;
	u32 cellStart = columnSpan > 1 ? spanStart : spanStart + 1;
	u32 cellEnd = std::min(cellStart + (columnSpan > 1 ? columnSpan : 1), (u32)state.persistent->columns.size());
	state.currentColumn = cellStart;

	// remember the column boundaries a spanning cell covers, so vertical separator
	// lines do not cut through the merged cell when borders are drawn
	if (columnSpan > 1)
	{
		if (state.isInHeader)
		{
			for (u32 b = spanStart + 1; b < cellEnd; b++)
				state.headerColspanBoundaries.push_back(b);
		}
		else
		{
			// ensure the row has a boundary list even if it was started with a
			// merged cell before the first tableRowNext (e.g. headerless table)
			if (state.rowColspanBoundaries.empty())
				state.rowColspanBoundaries.emplace_back();

			for (u32 b = spanStart + 1; b < cellEnd; b++)
				state.rowColspanBoundaries.back().push_back(b);
		}
	}

	// compute the X and width of the cell being set up
	f32 cellX = baseX;
	f32 cellWidth = 0;

	for (u32 i = 0; i < cellEnd; i++)
	{
		if (state.persistent->columns[i].isHidden)
			continue;

		if (i < cellStart)
			cellX += state.persistent->columns[i].width;
		else
			cellWidth += state.persistent->columns[i].width;
	}

	if (cellWidth <= 0)
		return;

	ctx->layout.width = cellWidth - (ctx->cellPadding.x * 2.0f);
	ctx->position.x = cellX + ctx->cellPadding.x;
	ctx->position.y = state.rowStartY + ctx->cellPadding.y; // reset Y to top of row
	state.cellStartY = state.rowStartY; // new cell starts at row top

	// push Clip rect to prevent cell content from overflowing
	// add 1px to width to include the border line on the right
	f32 clipHeight = 99999.0f;
	Rect clipRect(cellX, state.rowStartY, cellWidth + 1.0f, clipHeight);

	ctx->renderer.pushClipRect(clipRect);
	state.isClipping = true;
}


Rect tableCellGetRect()
{
	auto& state = currentTable();

	if (state.currentColumn >= state.persistent->columns.size()) return Rect();

	// calculate cell X position
	f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
	f32 cellX = baseX;

	for (u32 i = 0; i < state.currentColumn; i++)
	{
		if (!state.persistent->columns[i].isHidden)
			cellX += state.persistent->columns[i].width;
	}

	// get current column width (no spanning)
	f32 cellWidth = 0;

	if (!state.persistent->columns[state.currentColumn].isHidden)
	{
		cellWidth = state.persistent->columns[state.currentColumn].width;
	}

	// current row height so far
	f32 currentRowHeight = state.currentMaxRowHeight > 0 ? state.currentMaxRowHeight : state.rowHeight;

	return Rect(
		cellX,
		state.rowStartY,
		cellWidth,
		currentRowHeight
	);
}

void tableRowSetColor(const Color& color)
{
	auto& state = currentTable();
	state.currentRowColor = color;
	state.currentRowColorSet = true;
}

void tableCellSetColor(const Color& color)
{
	auto& state = currentTable();

	state.currentCellColor = color;
	state.currentCellColorSet = true;
	// defer cell background drawing
	state.cellColorRequests.push_back({ state.currentColumn, state.currentCellColor });
}

void tableCellPaddingPush(f32 paddingX, f32 paddingY)
{
	ctx->cellPaddingStack.push_back(ctx->cellPadding);
	ctx->cellPadding = Point(paddingX, paddingY);
}

void tableCellPaddingPop()
{
	if (!ctx->cellPaddingStack.empty())
	{
		ctx->cellPadding = ctx->cellPaddingStack.back();
		ctx->cellPaddingStack.pop_back();
	}
}

void tableColumnSetup(u32 columnIndex, f32 size, TableColumnFlags flags)
{
	if (ctx->tableStack.empty())
		return;

	auto& state = ctx->tableStack.back();

	// get persistent state
	auto iter = ctx->tablePersistentStates.find(state.id);
	
	if (iter == ctx->tablePersistentStates.end())
		return;
	
	auto& persistent = iter->second;

	if (columnIndex >= persistent.columns.size())
		return;

	// always contextUpdate flags
	persistent.columns[columnIndex].flags = flags;

	// if user resized, we generally respect that, BUT we might want to re-apply flags logic?
	// let's apply properties based on size first, then override with flags.
	// auto-detect: values <= 1 are percentages/weighted fill, values > 1 are pixels. 0 is pure fill.
	if (!persistent.columns[columnIndex].userResized)
	{
		persistent.columns[columnIndex].specifiedSize = size;

		// auto-detect: values <= 1 are percentages/weighted fill, values > 1 are pixels. 0 is pure fill.
		if (size > 0.0f && size <= 1.0f)
		{
			persistent.columns[columnIndex].isPercentage = false; // user requested percentage not be forced
			persistent.columns[columnIndex].isFillRemaining = true;
			persistent.columns[columnIndex].isStretchable = true;
		}
		else if (size == 0.0f)
		{
			persistent.columns[columnIndex].isPercentage = false;
			persistent.columns[columnIndex].isFillRemaining = true;
			persistent.columns[columnIndex].isStretchable = true;
		}
		else
		{
			persistent.columns[columnIndex].isPercentage = false;
			persistent.columns[columnIndex].isFillRemaining = false;
			persistent.columns[columnIndex].isStretchable = false;
		}
	}

	// apply overrides from flags (precedence over auto-detect)
	if (static_cast<bool>(flags & TableColumnFlags::Fixed))
	{
		persistent.columns[columnIndex].isStretchable = false;
		persistent.columns[columnIndex].isFillRemaining = false;
	}
	else if (static_cast<bool>(flags & TableColumnFlags::FixedResize))
	{
		persistent.columns[columnIndex].isStretchable = false;
		persistent.columns[columnIndex].isFillRemaining = false;
	}
	else if (static_cast<bool>(flags & TableColumnFlags::Stretch))
	{
		persistent.columns[columnIndex].isStretchable = true;
		persistent.columns[columnIndex].isFillRemaining = true;
	}
}

}