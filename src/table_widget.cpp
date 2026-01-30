#include "horus.h"
#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "util.h"
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
	// Calculate height of the last cell in the row
	// Note: ctx->position.y points to where the next widget would go, so it represents the bottom of content
	f32 lastCellHeight = ctx->position.y - state.cellStartY + ctx->cellPadding.y;
	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, lastCellHeight);

	// Ensure row has at least the minimum height (e.g. from theme)
	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, state.rowHeight);

	// Draw background for this row using deferred drawing
	// Note: If we are in the header, startHeader already drew the background and borders.
	// We should only draw here for body rows.
	if (!state.isInHeader)
	{
		f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
		
		// Draw custom row color if set
		if (state.currentRowColorSet)
		{
			Rect rowRect(
				baseX,
				state.rowStartY,
				state.innerWidth,
				state.currentMaxRowHeight
			);

			rowRect = rowRect.contract(1.0);

			ctx->renderer->cmdSetColor(state.currentRowColor);
			// Skip splitter when inside scroll view to respect clip rect
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(0);
			ctx->renderer->cmdDrawFilledRectangle(rowRect);
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(1);
		}
		// Draw alternating row background if enabled and no custom color
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

			// Switch to background layer (0), skip splitter when inside scroll view
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(0);

			if (state.currentRow % 2 == 1)
			{
				ctx->renderer->cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor0", Color::transparent));
			}
			else
			{
				ctx->renderer->cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor1", Color::transparent));
			}

			ctx->renderer->cmdDrawFilledRectangle(rowRect);

			// Switch back to content layer (1)
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(1);
		}

		// Draw pending cell backgrounds (on top of row background)
		if (!state.cellColorRequests.empty())
		{
			state.persistent->splitter->setLayer(0);
			for (const auto& req : state.cellColorRequests)
			{
				f32 cx = baseX;
				f32 cw = 0;
				// Find column x and width
				for (u32 i = 0; i < state.persistent->columns.size(); i++)
				{
					if (state.persistent->columns[i].isHidden) continue;
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
					ctx->renderer->cmdSetColor(req.color);
					ctx->renderer->cmdDrawFilledRectangle(cellRect);
				}
			}
			state.persistent->splitter->setLayer(1);
		}

		// Clear requests for next row
		state.cellColorRequests.clear();
		state.rowSeparators.push_back(state.rowStartY + state.currentMaxRowHeight);

		// Reset row color flag
		state.currentRowColorSet = false;

		// Advance Y position ONLY if we actually finished a row (not header)
		state.currentRowY += state.currentMaxRowHeight;
		ctx->position.y = state.currentRowY;
	}
	else
	{
		// Header drawing
		f32 headerHeight = state.currentMaxRowHeight;

		state.headerRect = Rect(
			state.tableRect.x,
			state.tableRect.y,
			state.innerWidth,
			headerHeight
		);

		auto& bodyElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);
		auto& bodyElemState = bodyElem.normalState();

		// Draw header background
		state.persistent->splitter->setLayer(0);
		ctx->renderer->cmdSetColor(bodyElemState.color);
		ctx->renderer->cmdDrawFilledRectangle(state.headerRect);

		// Header pending cell backgrounds? Usually not used, but supported just in case
		if (!state.cellColorRequests.empty())
		{
			for (const auto& req : state.cellColorRequests)
			{
				f32 cx = state.tableRect.x;
				f32 cw = 0;
				for (u32 i = 0; i < state.persistent->columns.size(); i++)
				{
					if (state.persistent->columns[i].isHidden) continue;
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
					ctx->renderer->cmdSetColor(req.color);
					ctx->renderer->cmdDrawFilledRectangle(cellRect);
				}
			}
			state.cellColorRequests.clear();
		}

		state.persistent->splitter->setLayer(1);

		// Draw vertical separators between columns (only if BordersV or compatible flags are set)
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) ||
			has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersV))
		{
			f32 currentX = state.headerRect.x;
			bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
			bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);

			auto& tableHeaderElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);
			ctx->renderer->cmdSetLineStyle(LineStyle(tableHeaderElem.currentStyle->getColorParameter("borderColorV", Color::white), 1.0f));

			for (u32 i = 0; i < state.persistent->columns.size(); i++)
			{
				if (!state.persistent->columns[i].isHidden)
				{
					// Draw left line for this column
					// Skip the first column's left line only if:
					// - We already drew it as the leftmost outer border (hasOuter and i==0)
					// - OR we're in inner-only mode and it's the first column
					bool drawLeftLine = true;
					if (i == 0 && (hasOuter || innerOnly))
						drawLeftLine = false;

					if (drawLeftLine)
					{
						ctx->renderer->cmdDrawLine(
							Point(currentX, state.headerRect.y),
							Point(currentX, state.headerRect.y + state.headerRect.height)
						);
					}

					currentX += state.persistent->columns[i].width;
				}
			}

			// Draw Rightmost line after all columns (skip if only inner borders)
			if (!innerOnly && !hasOuter)
			{
				// Draw Right line for last column
				ctx->renderer->cmdDrawLine(
					Point(currentX, state.headerRect.y),
					Point(currentX, state.headerRect.y + state.headerRect.height)
				);
			}
		}

		// Draw header bottom border AFTER vertical separators (only if BordersH or compatible flags are set)
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) ||
			has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersH))
		{
			auto& tableHeaderElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);
			ctx->renderer->cmdSetLineStyle(LineStyle(tableHeaderElem.currentStyle->getColorParameter("borderColorH", Color::white), 1.0f));
			ctx->renderer->cmdDrawLine(
				Point(state.headerRect.x, state.headerRect.y + state.headerRect.height),
				Point(state.headerRect.x + state.headerRect.width, state.headerRect.y + state.headerRect.height)
			);
		}

		state.currentRowY += headerHeight;
		state.bodyStartY = state.currentRowY;
		ctx->position.y = state.currentRowY;

		// Start scroll view for the body content
		f32 scrollViewHeight = state.innerHeight > 0 ? state.innerHeight : 200.0f;
		if (scrollViewHeight > 0)
		{
			// Calculate scroll view padding to compensate
			const auto& padding = getPadding(PaddingType::ScrollView);
			
			// Adjust position left by padding, and increase width by padding to compensate
			// This makes the content area align with table edge while scrollbar stays at right edge
			ctx->position.x = state.tableRect.x - padding.x;
			ctx->layout.width = state.innerWidth + padding.x;
			
			beginScrollView("##tableScrollView", scrollViewHeight, state.persistent->scrollViewScrollPos, 10000.0f, ScrollViewFlags::NoBorder);
			state.needsScrollViewStart = true;
			
			// After scroll view starts, ctx->position.x should now align with table edge
			state.scrollViewBaseX = ctx->position.x;
			state.currentRowY = ctx->position.y;
			state.rowStartY = ctx->position.y;
			state.bodyStartY = ctx->position.y;
		}
	}
}

bool beginTable(const char* id, u32 columnCount, f32 height, TableFlags flags)
{
	if (columnCount == 0)
		return false;

	WidgetId tableId = genId(id);
	auto& persistent = ctx->tablePersistentStates[tableId];

	// Initialize persistent state if needed
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

	// Apply active resize BEFORE calculating widths
	if (persistent.resizingColumn && persistent.resizingColumnIndex < columnCount)
	{
		u32 i = persistent.resizingColumnIndex;

		// Find target right column (same logic as endTable)
		u32 targetRightIndex = i + 1;
		while (targetRightIndex < columnCount &&
			   (static_cast<bool>(persistent.columns[targetRightIndex].flags & TableColumnFlags::Fixed) ||
				static_cast<bool>(persistent.columns[targetRightIndex].flags & TableColumnFlags::FixedResize)))
		{
			targetRightIndex++;
		}

		if (targetRightIndex < columnCount)
		{
			// Calculate delta
			f32 idealDelta = ctx->mousePosition.x - persistent.resizeStartX;
			f32 leftStart = persistent.resizeStartWidth;
			f32 rightStart = persistent.resizeStartWidthRight;

			// Clamp delta against min widths (10px)
			f32 maxNegativeDelta = -(leftStart - 10.0f);
			f32 maxPositiveDelta = (rightStart - 10.0f);

			if (idealDelta < maxNegativeDelta) idealDelta = maxNegativeDelta;
			if (idealDelta > maxPositiveDelta) idealDelta = maxPositiveDelta;

			// Apply
			persistent.columns[i].specifiedSize = leftStart + idealDelta;
			persistent.columns[targetRightIndex].specifiedSize = rightStart - idealDelta;
		}
	}

	// Create new transient table state
	TableState state;
	state.id = tableId;
	state.persistent = &persistent;
	state.currentColumn = 0;
	// state.columns removed
	state.isInHeader = false;
	state.currentRow = 0;
	state.flags = flags;
	state.headerRect = Rect(0,0,0,0);
	state.rowSeparators.clear(); // Clear separate list
	state.savedLayoutWidth = ctx->layout.width; // Save layout width to restore later

	// Copy persistent data to transient state -> REMOVED
	// We now use persistent directly
	// Ensure hidden/stretchable flags are synchronized if needed, but they are now in one place.
	// We might need to reset 'width' to defaults or recalc them?
	// The logic below recalculates 'width' based on 'specifiedSize'.


	// Calculate total used width from columns
	f32 totalColumnsWidth = 0;
	for (const auto& col : persistent.columns)
	{
		if (!col.isHidden)
			totalColumnsWidth += col.width;
	}

	// Get widget width (use available width if not set)
	f32 widgetWidth = ctx->layout.width;

	// Account for left and right borders (2px total) when Borders or BordersOuter flags are set
	bool hasBorders = has(flags, TableFlags::Borders) || has(flags, TableFlags::BordersOuter);
	if (hasBorders)
		widgetWidth -= 2.0f;

	// Apply column size specifications (percentage, pixels, or fill)
	f32 specifiedWidth = 0; // Total width of columns with specific sizes
	u32 fillCount = 0; // Number of columns that fill remaining space

	for (u32 i = 0; i < columnCount; i++)
	{
		// Use persistent column for everything
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
				// Percentage of table width (0..1)
				col.width = widgetWidth * col.specifiedSize;
			}
			else
			{
				// Fixed pixel size
				col.width = col.specifiedSize;
			}
			specifiedWidth += col.width;
		}
		else
		{
			// Use default width from persistent state
			specifiedWidth += col.width;
		}
	}

	// Calculate explicit weights for fill columns
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

	// Distribute remaining space to fill columns
	if (fillCount > 0)
	{
		f32 remainingWidth = widgetWidth - specifiedWidth;
		if (remainingWidth > 0)
		{
			// Verify if we need to normalize weights or share remaining space
			f32 weightNormalizer = 1.0f;
			f32 weightForPureFills = 0.0f;

			if (pureFillCount == 0)
			{
				// Only explicit weights: Normalize them to fill the space
				if (totalExplicitWeight > 0)
					weightNormalizer = 1.0f / totalExplicitWeight;
			}
			else
			{
				// Mixed explicit and pure: Pure fills divide the remaining weight
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
						// Weighted Fill
						persistent.columns[i].width = remainingWidth * (persistent.columns[i].specifiedSize * weightNormalizer);
					}
					else
					{
						// Pure Fill
						persistent.columns[i].width = widthPerPureFill;
					}
				}
			}
		}
	}

	// Recalculate total width after applying specifications
	totalColumnsWidth = 0;
	for (const auto& col : persistent.columns)
	{
		if (!col.isHidden)
			totalColumnsWidth += col.width;
	}

	// Collapse logic: if content exceeds widget width, scale down proportionally (respecting min width)
	if (!has(flags, TableFlags::FixedSize) && totalColumnsWidth > widgetWidth && widgetWidth > 0)
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

		if (totalFixed < widgetWidth)
		{
			// We have room for fixed columns, shrink flexible ones
			f32 scale = 0.0f;
			if (totalFlexible > 0)
				scale = (widgetWidth - totalFixed) / totalFlexible;

			for (u32 i = 0; i < columnCount; i++)
			{
				if (persistent.columns[i].isHidden) continue;

				bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
							   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

				if (!isFixed)
				{
					persistent.columns[i].width *= scale;
					// Apply min width floor
					if (persistent.columns[i].width < 1.0f) persistent.columns[i].width = 1.0f;
				}
			}
		}
		else
		{
			// Fixed columns alone take up too much space.
			for (u32 i = 0; i < columnCount; i++)
			{
				if (persistent.columns[i].isHidden) continue;

				bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
							   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

				if (!isFixed)
				{
					persistent.columns[i].width = 1.0f; // Collapse flexible to minimum
				}
			}
		}

		// Re-sum for final total
		totalColumnsWidth = 0;
		for (auto& col : persistent.columns)
		{
			if (!col.isHidden)
				totalColumnsWidth += col.width;
		}
	}

	// Stretch logic: distribute space proportionally based on column widths
	if (has(flags, TableFlags::Stretch) && widgetWidth != totalColumnsWidth)
	{
		// Calculate total width of stretchable columns
		f32 stretchableWidth = 0;
		for (const auto& col : persistent.columns)
			if (col.isStretchable && !col.isHidden)
				stretchableWidth += col.width;

		if (stretchableWidth > 0)
		{
			// Calculate the target width for stretchable columns
			f32 nonStretchableWidth = 0;
			for (const auto& col : persistent.columns)
				if (!col.isStretchable && !col.isHidden)
					nonStretchableWidth += col.width;

			f32 availableWidth = widgetWidth - nonStretchableWidth;

			// Scale each stretchable column proportionally
			for (auto& col : persistent.columns)
			{
				if (col.isStretchable && !col.isHidden)
				{
					f32 proportion = col.width / stretchableWidth;
					col.width = availableWidth * proportion;
				}
			}
			totalColumnsWidth = widgetWidth;
		}
	}
	// Final Table Width Logic
	// If FixedSize is NOT set, the table conforms to layout width (fills space).
	// If FixedSize IS set, it uses the sum of column widths.
	if (!has(flags, TableFlags::FixedSize))
	{
		if (widgetWidth > totalColumnsWidth)
			totalColumnsWidth = widgetWidth;
	}

	state.innerWidth = totalColumnsWidth;
	state.innerHeight = height; // If 0, auto height

	// Store table rectangle start
	// Start 1px to the right to leave room for the left border
	state.tableRect = Rect(
		ctx->position.x + 1.0f,
		ctx->position.y,
		totalColumnsWidth,
		height // Note: if height is 0, this might be misleading until endTable
	);

	state.rowStartY = state.tableRect.y;
	state.bodyStartY = state.tableRect.y; // Default start if no header
	state.currentRowY = state.tableRect.y;
	state.currentMaxRowHeight = 0;

	// Push state so we can use it
	ctx->tableStack.push_back(state);

	// Get row height from theme
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
	auto& bodyElemState = bodyElem.normalState();
	ctx->tableStack.back().rowHeight = bodyElem.currentStyle->getParameter("rowHeight", 25);
	//state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();

	// Push a clip rect for the entire table to prevent backgrounds from extending too far
	// Start 1px to the left to include the left border, and add 2px to width for both borders
	auto& currentState = ctx->tableStack.back();
	Rect tableClipRect(currentState.tableRect.x - 1.0f, currentState.tableRect.y, currentState.innerWidth + 2.0f, 10000.0f);
	ctx->renderer->pushClipRect(tableClipRect);
	currentState.hasTableClip = true;
	ctx->tableStack.back().hasTableClip = true;

	// Init Splitter: Layer 0 = Background, Layer 1 = Content
	currentState.persistent->splitter->clear();
	currentState.persistent->splitter->split(2);
	currentState.persistent->splitter->setLayer(1);

	// Store that we need to begin scroll view after header
	currentState.needsScrollViewStart = false;

	return true;
}

void endTable()
{
	if (ctx->tableStack.empty()) return;
	auto& state = ctx->tableStack.back();

	// Finish the last row
	finishRow(state);

	// Draw borders for body rows BEFORE ending scroll view so they get clipped
	if (state.needsScrollViewStart)
	{
		auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
		
		// Determine which color to use based on border type (inner vs outer)
		Color innerHColor = tableBodyElem.currentStyle->getColorParameter("innerBorderColorH", Color::white);
		Color innerVColor = tableBodyElem.currentStyle->getColorParameter("innerBorderColorV", Color::white);
		Color outerVColor = tableBodyElem.currentStyle->getColorParameter("outerBorderColorV", Color::white);
		
		// Draw borders if enabled
		// Skip splitter when inside scroll view to respect clip rect immediately
		if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::BordersV) || has(state.flags, TableFlags::BordersH))
		{
			// Only use splitter if not in scroll view
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(0);
			
			f32 baseX = state.scrollViewBaseX;
			
			// Draw Inner Horizontal Lines
			if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersH))
			{
				ctx->renderer->cmdSetLineStyle(LineStyle(innerHColor, 1.0f));
				for (size_t i = 0; i < state.rowSeparators.size(); i++)
				{
					bool draw = true;
					// Skip last line if only inner borders (not outer) or if Borders flag is set
					if (i == state.rowSeparators.size() - 1)
					{
						// Skip bottom line if we're only drawing inner borders (no outer borders)
						if (has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter))
							draw = false;
						else if (has(state.flags, TableFlags::Borders))
							draw = false; // Outer border will draw it
					}
					
					if (draw)
					{
						ctx->renderer->cmdDrawLine(
							Point(baseX, state.rowSeparators[i]),
							Point(baseX + state.innerWidth, state.rowSeparators[i])
						);
					}
				}
			}
			
			// Draw Vertical Lines (Inner + Outer Left/Right)
			if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersV))
			{
				ctx->renderer->cmdSetLineStyle(LineStyle(innerVColor, 1.0f));
				bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
				bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);
				
				// Draw vertical lines for each row
				for (u32 rowIdx = 0; rowIdx < state.rowSeparators.size() + 1; rowIdx++)
				{
					f32 lineStartY = rowIdx < state.rowSeparators.size() && rowIdx > 0 ? state.rowSeparators[rowIdx - 1] : state.bodyStartY;
					f32 lineEndY = rowIdx < state.rowSeparators.size() ? state.rowSeparators[rowIdx] : state.currentRowY;
					
					f32 currentX = baseX;
					
					// Draw vertical lines between columns
					for (u32 i = 0; i < state.persistent->columns.size(); i++)
					{
						if (!state.persistent->columns[i].isHidden)
						{
							// Draw left line for this column
							bool drawLeftLine = true;
							if (i == 0 && (hasOuter || innerOnly))
								drawLeftLine = false;
							
							if (drawLeftLine)
							{
								ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
							}
							currentX += state.persistent->columns[i].width;
						}
					}
					
					// Draw Rightmost line after all columns (skip if only inner borders or if handled by outer box)
					if (!innerOnly && !hasOuter)
					{
						ctx->renderer->cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
						ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
					}
				}
			}
			
			if (!state.needsScrollViewStart)
				state.persistent->splitter->setLayer(1);
		}
	}

	// End scroll view if it was started
	if (state.needsScrollViewStart)
	{
		state.persistent->scrollViewScrollPos = endScrollView();
	}

	f32 finalHeight = state.currentRowY - state.tableRect.y;

	// Pop any remaining clip rect BEFORE drawing borders so they don't get clipped
	if (state.isClipping)
	{
		ctx->renderer->popClipRect();
		state.isClipping = false;
	}

	// Handle column resizing
	if (has(state.flags, TableFlags::Resizable))
	{
		auto& persistent = *state.persistent;
		f32 currentX = state.tableRect.x;
		f32 separatorWidth = 4.0f;

		for (u32 i = 0; i < persistent.columns.size(); i++)
		{
			if (persistent.columns[i].isHidden) continue;

			currentX += persistent.columns[i].width;

			u32 nextColIndex = i + 1;
			while (nextColIndex < persistent.columns.size() && persistent.columns[nextColIndex].isHidden)
				nextColIndex++;

			u32 targetRightIndex = nextColIndex;

			// Find the first column to the right that CAN be resized (absorb the delta)
			// Pass-through Fixed and FixedResize columns
			while (targetRightIndex < persistent.columns.size() &&
				   (static_cast<bool>(persistent.columns[targetRightIndex].flags & TableColumnFlags::Fixed) ||
					static_cast<bool>(persistent.columns[targetRightIndex].flags & TableColumnFlags::FixedResize)))
			{
				targetRightIndex++;
			}

			if (persistent.resizingColumn && persistent.resizingColumnIndex == i)
			{
				ctx->mouseCursor = MouseCursorType::SizeWE;

				if (ctx->event.type == InputEvent::Type::MouseUp || ctx->event.type == InputEvent::Type::WindowLostFocus)
				{
					releaseWindowCapture();
					ctx->widget.captureId = 0;
					persistent.resizingColumn = false;
					persistent.resizingColumnIndex = ~0;
				}
				else
				{
					// Draw Resize Guide Line - width already applied in beginTable
					f32 guideLineX = currentX;
					// Clip line to scroll view bounds if inside scroll view
					f32 lineBottomY = state.needsScrollViewStart ? 
						(state.bodyStartY + (state.innerHeight > 0 ? state.innerHeight : 200.0f)) :
						(state.tableRect.y + finalHeight);

					auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
					ctx->renderer->cmdSetLineStyle(LineStyle(tableBodyElem.currentStyle->getColorParameter("columnResizeLineColor", Color(0.0f, 1.0f, 1.0f, 1.0f)), 2.0f));
					ctx->renderer->cmdDrawLine(Point(guideLineX, state.tableRect.y),
											   Point(guideLineX, lineBottomY));

					// Only apply resize if validity checks pass (though we started, so they should)
					if (targetRightIndex < persistent.columns.size() && !(static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed)))
					{
						// Reciprocal Resize: Change Left and Right columns
						f32 idealDelta = ctx->mousePosition.x - persistent.resizeStartX;

						f32 leftStart = persistent.resizeStartWidth;
						f32 rightStart = persistent.resizeStartWidthRight;

						// Clamp delta against min widths (10px)
						f32 maxNegativeDelta = -(leftStart - 10.0f); // Limit shrinking Left
						f32 maxPositiveDelta = (rightStart - 10.0f); // Limit shrinking Right (by growing Left)

						if (idealDelta < maxNegativeDelta) idealDelta = maxNegativeDelta;
						if (idealDelta > maxPositiveDelta) idealDelta = maxPositiveDelta;

						// Apply
						persistent.columns[i].specifiedSize = leftStart + idealDelta;
						persistent.columns[targetRightIndex].specifiedSize = rightStart - idealDelta;
					}
				}
			}
			else if (!persistent.resizingColumn)
			{
				// Only allow NEW interaction if guards pass
				if (targetRightIndex < persistent.columns.size() &&
					!(static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed)))
				{
					Rect separatorRect(currentX - separatorWidth, state.tableRect.y, separatorWidth * 2.0f, finalHeight);

					// Determine if this separator is covered by a column span in the row under the mouse
					bool isSeparatorCovered = false;

					// Find which row the mouse is in
					i32 hoveredRowIndex = -1;
					f32 mouseY = ctx->mousePosition.y;

					if (state.rowSeparators.empty())
					{
						// Fallback for single row table if logic failed elsewhere
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

						// Draw Hover Guide Line
						// Clip line to scroll view bounds if inside scroll view
						f32 lineBottomY = state.needsScrollViewStart ? 
							(state.bodyStartY + (state.innerHeight > 0 ? state.innerHeight : 200.0f)) :
							(state.tableRect.y + finalHeight);
						
						auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
						ctx->renderer->cmdSetLineStyle(LineStyle(tableBodyElem.currentStyle->getColorParameter("columnResizeLineColor", Color::cyan), 2.0f));
						ctx->renderer->cmdDrawLine(Point(currentX, state.tableRect.y),
												   Point(currentX, lineBottomY));

						if (ctx->event.type == InputEvent::Type::MouseDown && ctx->event.mouse.button == MouseButton::Left)
						{
							setWindowCapture();
							ctx->widget.captureId = state.id;
							persistent.resizingColumn = true;
							persistent.resizingColumnIndex = i; // Store SEPARATOR index
							persistent.resizeStartX = ctx->mousePosition.x; // Store Absolute Start X

							// Reciprocal Resize Setup: Capture BOTH Left and Right attributes
							persistent.resizeStartWidth = persistent.columns[i].width;
							persistent.resizeStartWidthRight = persistent.columns[targetRightIndex].width;

							// Synchronize ALL columns to their current visual width to prevent jumps
							for (u32 k = 0; k < persistent.columns.size(); k++)
							{
								if (!persistent.columns[k].isHidden)
								{
									persistent.columns[k].specifiedSize = persistent.columns[k].width;
								}
							}

							// Lock Left Column
							persistent.columns[i].specifiedSize = persistent.columns[i].width;
							persistent.columns[i].isPercentage = false;
							persistent.columns[i].isFillRemaining = false;
							persistent.columns[i].userResized = true;

							// Lock Right Column
							persistent.columns[targetRightIndex].specifiedSize = persistent.columns[targetRightIndex].width;
							persistent.columns[targetRightIndex].isPercentage = false;
							persistent.columns[targetRightIndex].isFillRemaining = false;
							persistent.columns[targetRightIndex].userResized = true;
						}
					}
				}
			}
		}
	}

	auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);

	// Draw outer box for entire table (header + body) if borders are enabled
	// This is drawn outside the scroll view to frame the entire table
	if (!state.needsScrollViewStart && (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter)))
	{
		Color outerHColor = tableBodyElem.currentStyle->getColorParameter("outerBorderColorH", Color::white);
		Color outerVColor = tableBodyElem.currentStyle->getColorParameter("outerBorderColorV", Color::white);
		
		state.persistent->splitter->setLayer(0);
		
		// Top Line (at tableRect.y)
		ctx->renderer->cmdSetLineStyle(LineStyle(outerHColor, 1.0f));
		ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
								   Point(state.tableRect.x + state.innerWidth, state.tableRect.y));

		// Bottom Line (at finalHeight)
		ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y + finalHeight),
								   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + finalHeight));

		// Sides
		ctx->renderer->cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
		ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
								   Point(state.tableRect.x, state.tableRect.y + finalHeight));
		ctx->renderer->cmdDrawLine(Point(state.tableRect.x + state.innerWidth, state.tableRect.y),
								   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + finalHeight));
		
		state.persistent->splitter->setLayer(1);
	}

	// Pop table clip rect if it was pushed
	if (state.hasTableClip)
	{
		ctx->renderer->popClipRect();
	}

	// Merge layers: Background (0) and Content (1)
	state.persistent->splitter->merge();

	// Restore layout width and cursor X position
	// We want the cursor to be at the start of the layout (left indentation) for the next widget
	// The table started at state.tableRect.x - 1.0f (since we added 1px shift left).
	// So we restore it to that.
	ctx->layout.width = state.savedLayoutWidth;
	ctx->position.x = state.tableRect.x - 1.0f;
	ctx->tableStack.pop_back();
}

void startHeader()
{
	auto& state = currentTable();
	state.isInHeader = true;
	state.currentColumn = 0;

	// Reset row parameters
	state.rowStartY = state.currentRowY;
	//state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();
	state.currentMaxRowHeight = state.rowHeight; // Use theme default height as min

	// Setup for first cell
	state.cellStartY = state.rowStartY;

	if (state.currentColumn < state.persistent->columns.size())
	{
		ctx->layout.width = state.persistent->columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
		ctx->position.x = state.tableRect.x + ctx->cellPadding.x;
		ctx->position.y = state.rowStartY + ctx->cellPadding.y;

		// Start Clipping for first cell
		if (state.isClipping) ctx->renderer->popClipRect(); // Should not happen here usually, but safe
		Rect clipRect(state.tableRect.x, state.rowStartY, state.persistent->columns[state.currentColumn].width, 99999.0f);
		ctx->renderer->pushClipRect(clipRect);
		state.isClipping = true;
	}
}

void nextRow()
{
	auto& state = currentTable();

	// Pop clip rect from previous cell in previous row
	if (state.isClipping)
	{
		ctx->renderer->popClipRect();
		state.isClipping = false;
	}

	// Finish previous row
	finishRow(state);

	// Start new row
	state.currentRow++;
	state.currentColumn = 0;
	state.isInHeader = false;

	state.rowStartY = state.currentRowY;
	//state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();
	state.currentMaxRowHeight = state.rowHeight; // Use theme default height as min

	state.cellStartY = state.rowStartY;

	// Row background drawing REMOVED, deferred to finishRow
	// Custom row color and alt row color are handled in finishRow

	// Setup for first cell
	if (state.currentColumn < state.persistent->columns.size())
	{
		ctx->layout.width = state.persistent->columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
		
		// Use scrollViewBaseX if scroll view is active, otherwise use tableRect.x
		f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
		ctx->position.x = baseX + ctx->cellPadding.x;
		ctx->position.y = state.rowStartY + ctx->cellPadding.y;

		// Start Clipping
		// Note: When inside scroll view, we don't need per-cell clipping as scroll view already clips
		// Per-cell clipping can cause issues with left edge being clipped
		if (!state.needsScrollViewStart)
		{
			Rect clipRect(baseX, state.rowStartY, state.persistent->columns[state.currentColumn].width, 99999.0f);
			ctx->renderer->pushClipRect(clipRect);
			state.isClipping = true;
		}
	}
}

void nextCell()
{
	auto& state = currentTable();

	// Pop previous clip
	if (state.isClipping)
	{
		ctx->renderer->popClipRect();
		state.isClipping = false;
	}

	if (state.currentColumn < state.persistent->columns.size())
	{
		// Calculate height of the cell we just finished
		// Note: ctx->position.y points to where the next widget would go, so it represents the bottom of content
		// position.y already includes the top padding we added at start of cell, so we only need to add bottom padding
		f32 finishedCellHeight = ctx->position.y - state.cellStartY + ctx->cellPadding.y;
		state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, finishedCellHeight);

		// Reset cell color flag for previous cell
		state.currentCellColorSet = false;

		// Advance to next column (no spanning)
		state.currentColumn++;

		// Move to next column
		f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
		f32 cellX = baseX;
		for (u32 i = 0; i < state.currentColumn && i < state.persistent->columns.size(); i++)
		{
			if (!state.persistent->columns[i].isHidden)
				cellX += state.persistent->columns[i].width;
		}

		if (state.currentColumn < state.persistent->columns.size())
		{
			ctx->layout.width = state.persistent->columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
			ctx->position.x = cellX + ctx->cellPadding.x;
			ctx->position.y = state.rowStartY + ctx->cellPadding.y; // Reset Y to top of row
			state.cellStartY = state.rowStartY; // New cell starts at row top

			// Push Clip
			// Skip clipping when inside scroll view to prevent left edge clipping
			if (!state.needsScrollViewStart)
			{
				f32 clipHeight = 99999.0f;
				Rect clipRect(cellX, state.rowStartY, state.persistent->columns[state.currentColumn].width, clipHeight);
				ctx->renderer->pushClipRect(clipRect);
				state.isClipping = true;
			}
		}
	}
}


Rect getCellRect()
{
	auto& state = currentTable();

	if (state.currentColumn >= state.persistent->columns.size()) return Rect();

	// Calculate cell X position
	f32 baseX = state.needsScrollViewStart ? state.scrollViewBaseX : state.tableRect.x;
	f32 cellX = baseX;

	for (u32 i = 0; i < state.currentColumn; i++)
	{
		if (!state.persistent->columns[i].isHidden)
			cellX += state.persistent->columns[i].width;
	}

	// Get current column width (no spanning)
	f32 cellWidth = 0;
	if (!state.persistent->columns[state.currentColumn].isHidden)
	{
		cellWidth = state.persistent->columns[state.currentColumn].width;
	}

	// Current row height so far
	f32 currentRowHeight = state.currentMaxRowHeight > 0 ? state.currentMaxRowHeight : state.rowHeight;

	return Rect(
		cellX,
		state.rowStartY,
		cellWidth,
		currentRowHeight
	);
}

void setRowColor(const Color& color)
{
	auto& state = currentTable();
	state.currentRowColor = color;
	state.currentRowColorSet = true;
}

void setCellColor(const Color& color)
{
	auto& state = currentTable();
	state.currentCellColor = color;
	state.currentCellColorSet = true;

	// Defer cell background drawing
	state.cellColorRequests.push_back({ state.currentColumn, state.currentCellColor });
}

void pushCellPadding(f32 paddingX, f32 paddingY)
{
	ctx->cellPaddingStack.push_back(ctx->cellPadding);
	ctx->cellPadding = Point(paddingX, paddingY);
}

void popCellPadding()
{
	if (!ctx->cellPaddingStack.empty())
	{
		ctx->cellPadding = ctx->cellPaddingStack.back();
		ctx->cellPaddingStack.pop_back();
	}
}

void setupColumn(u32 columnIndex, f32 size, TableColumnFlags flags)
{
	if (ctx->tableStack.empty()) return;
	auto& state = ctx->tableStack.back();

	// Get persistent state
	auto iter = ctx->tablePersistentStates.find(state.id);
	if (iter == ctx->tablePersistentStates.end()) return;
	auto& persistent = iter->second;

	if (columnIndex >= persistent.columns.size()) return;

	// Always update flags
	persistent.columns[columnIndex].flags = flags;

	// If user resized, we generally respect that, BUT we might want to re-apply flags logic?
	// The prompt implies we want to set these properties.
	// If userResized is true, the width is fixed to what they set.
	// However, flags like 'Fixed' might imply it can NEVER be resized?
	// Let's apply properties based on size first, then override with flags.

	if (!persistent.columns[columnIndex].userResized)
	{
		persistent.columns[columnIndex].specifiedSize = size;

		// Auto-detect: values <= 1 are percentages/weighted fill, values > 1 are pixels. 0 is pure fill.
		if (size > 0.0f && size <= 1.0f)
		{
			persistent.columns[columnIndex].isPercentage = false; // User requested percentage not be forced
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

	// Apply overrides from flags (Precedence over auto-detect)
	if (static_cast<bool>(flags & TableColumnFlags::Fixed))
	{
		persistent.columns[columnIndex].isStretchable = false;
		persistent.columns[columnIndex].isFillRemaining = false;
		// Ideally Fixed also means not resizable by user? We should handle that in resize logic.
	}
	else if (static_cast<bool>(flags & TableColumnFlags::FixedResize))
	{
		persistent.columns[columnIndex].isStretchable = false;
		persistent.columns[columnIndex].isFillRemaining = false;
	}
	else if (static_cast<bool>(flags & TableColumnFlags::Stretch))
	{
		persistent.columns[columnIndex].isStretchable = true;
		// Should Stretch implies fill remaining? Usually yes.
		persistent.columns[columnIndex].isFillRemaining = true;
	}
}

}