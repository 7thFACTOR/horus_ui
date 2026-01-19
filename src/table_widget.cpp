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
		ctx->renderer->beginDrawCmdInsertion(state.rowDrawCmdIndex);

		// Draw custom row color if set
		if (state.currentRowColorSet)
		{
			Rect rowRect(
				state.tableRect.x,
				state.rowStartY,
				state.innerWidth,
				state.currentMaxRowHeight
			);

			rowRect = rowRect.contract(1.0);

			ctx->renderer->cmdSetColor(state.currentRowColor);
			ctx->renderer->cmdDrawFilledRectangle(rowRect);
		}
		// Draw alternating row background if enabled and no custom color
		else if (has(state.flags, TableFlags::AltRowBg))
		{
			Rect rowRect(
				state.tableRect.x,
				state.rowStartY,
				state.innerWidth,
				state.currentMaxRowHeight
			);

			rowRect = rowRect.contract(1.0);
			auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
			
			if (state.currentRow % 2 == 1)
			{
				ctx->renderer->cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor0", Color::transparent));
			}
			else
			{
				ctx->renderer->cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor1", Color::transparent));
			}
			
			ctx->renderer->cmdDrawFilledRectangle(rowRect);
		}

		ctx->renderer->endDrawCmdInsertion();

		// Store the bottom Y position of this row for deferred border drawing
		state.rowSeparators.push_back(state.rowStartY + state.currentMaxRowHeight);

		// Reset row color flag
		state.currentRowColorSet = false;

		// Advance Y position ONLY if we actually finished a row (not header)
		state.currentRowY += state.currentMaxRowHeight;
		ctx->position.y = state.currentRowY;
	}
	else
	{
		// Header deferred drawing
		ctx->renderer->beginDrawCmdInsertion(state.rowDrawCmdIndex);

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
		ctx->renderer->cmdSetColor(bodyElemState.color);
		ctx->renderer->cmdDrawFilledRectangle(state.headerRect);

		// Draw vertical separators between columns (only if BordersV or compatible flags are set)
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders) ||
			has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersV))
		{
			f32 currentX = state.headerRect.x;
			bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
			bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);

			auto& tableHeaderElem = ctx->theme->getElement(WidgetElementId::TableHeaderBody);
			ctx->renderer->cmdSetLineStyle(LineStyle(tableHeaderElem.currentStyle->getColorParameter("borderColor", Color::white), 1.0f));

			// Draw leftmost line first if outer borders are needed
			if (hasOuter)
			{
				// Draw Left line
				ctx->renderer->cmdDrawLine(
					Point(currentX, state.headerRect.y),
					Point(currentX, state.headerRect.y + state.headerRect.height)
				);
			}

			for (u32 i = 0; i < state.columns.size(); i++)
			{
				if (!state.columns[i].isHidden)
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

					currentX += state.columns[i].width;
				}
			}

			// Draw Rightmost line after all columns (skip if only inner borders)
			if (!innerOnly)
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
			ctx->renderer->cmdSetLineStyle(LineStyle(tableHeaderElem.currentStyle->getColorParameter("borderColor", Color::white), 1.0f));
			ctx->renderer->cmdDrawLine(
				Point(state.headerRect.x, state.headerRect.y + state.headerRect.height),
				Point(state.headerRect.x + state.headerRect.width, state.headerRect.y + state.headerRect.height)
			);
		}

		ctx->renderer->endDrawCmdInsertion();

		state.currentRowY += headerHeight;
		state.bodyStartY = state.currentRowY;
		ctx->position.y = state.currentRowY;
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
	state.currentColumn = 0;
	state.columns.resize(columnCount);
	state.isInHeader = false;
	state.currentRow = 0;
	state.flags = flags;
	state.headerRect = Rect(0,0,0,0);
	state.rowSeparators.clear(); // Clear separate list

	// Copy persistent data to transient state
	for (u32 i = 0; i < columnCount; i++)
	{
		state.columns[i].width = persistent.columns[i].width;

		// Always use the persistent stretchable state (from flags)
		state.columns[i].isStretchable = persistent.columns[i].isStretchable;

		state.columns[i].isHidden = persistent.columns[i].isHidden;
	}

	// Calculate total used width from columns
	f32 totalColumnsWidth = 0;
	for (const auto& col : state.columns)
	{
		if (!col.isHidden)
			totalColumnsWidth += col.width;
	}

	// Get widget width (use available width if not set)
	f32 widgetWidth = ctx->widget.width > 0 ? ctx->widget.width : ctx->layout.width;

	// Account for left and right borders (2px total) when Borders or BordersOuter flags are set
	bool hasBorders = has(flags, TableFlags::Borders) || has(flags, TableFlags::BordersOuter);
	if (hasBorders)
		widgetWidth -= 2.0f;

	// Apply column size specifications (percentage, pixels, or fill)
	f32 specifiedWidth = 0; // Total width of columns with specific sizes
	u32 fillCount = 0; // Number of columns that fill remaining space

	for (u32 i = 0; i < columnCount; i++)
	{
		auto& pCol = persistent.columns[i];
		auto& sCol = state.columns[i];

		if (sCol.isHidden) continue;

		bool isFixed = static_cast<bool>(pCol.flags & TableColumnFlags::Fixed) || static_cast<bool>(pCol.flags & TableColumnFlags::FixedResize);

		if (pCol.isFillRemaining && !isFixed)
		{
			fillCount++;
		}
		else if (pCol.specifiedSize > 0)
		{
			if (pCol.isPercentage && !isFixed)
			{
				// Percentage of table width (0..1)
				sCol.width = widgetWidth * pCol.specifiedSize;
			}
			else
			{
				// Fixed pixel size
				sCol.width = pCol.specifiedSize;
			}
			specifiedWidth += sCol.width;
		}
		else
		{
			// Use default width from persistent state
			specifiedWidth += sCol.width;
		}
	}

	// Calculate explicit weights for fill columns
	f32 totalExplicitWeight = 0.0f;
	u32 pureFillCount = 0;

	for (u32 i = 0; i < columnCount; i++)
	{
		auto& pCol = persistent.columns[i];
		if (state.columns[i].isHidden) continue;

		bool isFixed = static_cast<bool>(pCol.flags & TableColumnFlags::Fixed) || static_cast<bool>(pCol.flags & TableColumnFlags::FixedResize);

		if (pCol.isFillRemaining && !isFixed)
		{
			if (pCol.specifiedSize > 0 && pCol.specifiedSize <= 1.0f)
			{
				totalExplicitWeight += pCol.specifiedSize;
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
				if (!state.columns[i].isHidden && persistent.columns[i].isFillRemaining)
				{
					if (persistent.columns[i].specifiedSize > 0 && persistent.columns[i].specifiedSize <= 1.0f)
					{
						// Weighted Fill
						state.columns[i].width = remainingWidth * (persistent.columns[i].specifiedSize * weightNormalizer);
					}
					else
					{
						// Pure Fill
						state.columns[i].width = widthPerPureFill;
					}
				}
			}
		}
	}

	// Recalculate total width after applying specifications
	totalColumnsWidth = 0;
	for (const auto& col : state.columns)
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
			if (state.columns[i].isHidden) continue;

			bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
						   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

			if (isFixed)
				totalFixed += state.columns[i].width;
			else
				totalFlexible += state.columns[i].width;
		}

		if (totalFixed < widgetWidth)
		{
			// We have room for fixed columns, shrink flexible ones
			f32 scale = 0.0f;
			if (totalFlexible > 0)
				scale = (widgetWidth - totalFixed) / totalFlexible;

			for (u32 i = 0; i < columnCount; i++)
			{
				if (state.columns[i].isHidden) continue;

				bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
							   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

				if (!isFixed)
				{
					state.columns[i].width *= scale;
					// Apply min width floor for flexible columns to avoid complete collapse if desired,
					// but rigorous math says we should fit. If we clamp, we might overflow.
					// Let's stick to the math 'scale' but ensure at least 1px to avoid div by zero issues elsewhere.
					if (state.columns[i].width < 1.0f) state.columns[i].width = 1.0f;
				}
			}
		}
		else
		{
			// Fixed columns alone take up too much space.
			// We must keep Fixed columns as is (overflow), and crush flexible columns.
			for (u32 i = 0; i < columnCount; i++)
			{
				if (state.columns[i].isHidden) continue;

				bool isFixed = static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed) ||
							   static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::FixedResize);

				if (!isFixed)
				{
					state.columns[i].width = 1.0f; // Collapse flexible to minimum
				}
			}
		}

		// Re-sum for final total
		totalColumnsWidth = 0;
		for (auto& col : state.columns)
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
		for (const auto& col : state.columns)
			if (col.isStretchable && !col.isHidden)
				stretchableWidth += col.width;

		if (stretchableWidth > 0)
		{
			// Calculate the target width for stretchable columns
			f32 nonStretchableWidth = 0;
			for (const auto& col : state.columns)
				if (!col.isStretchable && !col.isHidden)
					nonStretchableWidth += col.width;

			f32 availableWidth = widgetWidth - nonStretchableWidth;

			// Scale each stretchable column proportionally
			for (auto& col : state.columns)
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
	state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();

	// Push a clip rect for the entire table to prevent backgrounds from extending too far
	// Start 1px to the left to include the left border, and add 2px to width for both borders
	auto& currentState = ctx->tableStack.back();
	Rect tableClipRect(currentState.tableRect.x - 1.0f, currentState.tableRect.y, currentState.innerWidth + 2.0f, 10000.0f);
	ctx->renderer->pushClipRect(tableClipRect);
	currentState.hasTableClip = true;
	ctx->tableStack.back().hasTableClip = true;

	// Draw first row background (row 0) before any content
	// Note: If user calls startHeader, this will be overdrawn by header background
	if (has(currentState.flags, TableFlags::AltRowBg))
	{
		Rect rowRect(
			currentState.tableRect.x,
			currentState.rowStartY,
			currentState.innerWidth,
			9999.0f  // Use large height, actual row height not known yet
		);

		rowRect = rowRect.contract(1.0);
		
		// First row is row 0, so use rowAltBgColor1 (even row)
		ctx->renderer->cmdSetColor(bodyElem.currentStyle->getColorParameter("rowAltBgColor1", Color::transparent));
		ctx->renderer->cmdDrawFilledRectangle(rowRect);
	}

	return true;
}

void endTable()
{
	if (ctx->tableStack.empty()) return;
	auto& state = ctx->tableStack.back();

	// Finish the last row
	finishRow(state);

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
		auto& persistent = ctx->tablePersistentStates[state.id];
		f32 currentX = state.tableRect.x;
		f32 separatorWidth = 4.0f;

		for (u32 i = 0; i < state.columns.size(); i++)
		{
			if (state.columns[i].isHidden) continue;

			currentX += state.columns[i].width;

			u32 nextColIndex = i + 1;
			while (nextColIndex < state.columns.size() && state.columns[nextColIndex].isHidden)
				nextColIndex++;

			u32 targetRightIndex = nextColIndex;

			// Find the first column to the right that CAN be resized (absorb the delta)
			// Pass-through Fixed and FixedResize columns
			while (targetRightIndex < state.columns.size() &&
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
					
					auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
					ctx->renderer->cmdSetLineStyle(LineStyle(tableBodyElem.currentStyle->getColorParameter("columnResizeLineColor", Color(0.0f, 1.0f, 1.0f, 1.0f)), 2.0f));
					ctx->renderer->cmdDrawLine(Point(guideLineX, state.tableRect.y),
											   Point(guideLineX, state.tableRect.y + finalHeight));

					// Only apply resize if validity checks pass (though we started, so they should)
					if (targetRightIndex < state.columns.size() && !(static_cast<bool>(persistent.columns[i].flags & TableColumnFlags::Fixed)))
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
				if (targetRightIndex < state.columns.size() &&
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
						auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
						ctx->renderer->cmdSetLineStyle(LineStyle(tableBodyElem.currentStyle->getColorParameter("columnResizeLineColor", Color(0.0f, 1.0f, 1.0f, 1.0f)), 2.0f));
						ctx->renderer->cmdDrawLine(Point(currentX, state.tableRect.y),
												   Point(currentX, state.tableRect.y + finalHeight));

						if (ctx->event.type == InputEvent::Type::MouseDown && ctx->event.mouse.button == MouseButton::Left)
						{
							setWindowCapture();
							ctx->widget.captureId = state.id;
							persistent.resizingColumn = true;
							persistent.resizingColumnIndex = i; // Store SEPARATOR index
							persistent.resizeStartX = ctx->mousePosition.x; // Store Absolute Start X

							// Reciprocal Resize Setup: Capture BOTH Left and Right attributes
							persistent.resizeStartWidth = state.columns[i].width;
							persistent.resizeStartWidthRight = state.columns[targetRightIndex].width;

							// Synchronize ALL columns to their current visual width to prevent jumps
							for (u32 k = 0; k < persistent.columns.size(); k++)
							{
								if (!state.columns[k].isHidden)
								{
									persistent.columns[k].specifiedSize = state.columns[k].width;
								}
							}

							// Lock Left Column
							persistent.columns[i].specifiedSize = state.columns[i].width;
							persistent.columns[i].isPercentage = false;
							persistent.columns[i].isFillRemaining = false;
							persistent.columns[i].userResized = true;

							// Lock Right Column
							persistent.columns[targetRightIndex].specifiedSize = state.columns[targetRightIndex].width;
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
	
	// Determine which color to use based on border type (inner vs outer)
	Color innerHColor = tableBodyElem.currentStyle->getColorParameter("innerHorizontalLineColor", Color::white);
	Color innerVColor = tableBodyElem.currentStyle->getColorParameter("innerVerticalLineColor", Color::white);
	Color outerHColor = tableBodyElem.currentStyle->getColorParameter("outerHorizontalLineColor", Color::white);
	Color outerVColor = tableBodyElem.currentStyle->getColorParameter("outerVerticalLineColor", Color::white);

	// Batch draw borders if enabled
	if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::BordersV) || has(state.flags, TableFlags::BordersH))
	{
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
						Point(state.tableRect.x, state.rowSeparators[i]),
						Point(state.tableRect.x + state.innerWidth, state.rowSeparators[i])
					);
				}
			}
		}

		// Draw Vertical Lines (Inner + Outer Left/Right)
		// We need to draw these per-row to respect column spans
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

				f32 currentX = state.tableRect.x;

				// Draw leftmost line if outer borders are needed
				if (hasOuter)
				{
					ctx->renderer->cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
					ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
					ctx->renderer->cmdSetLineStyle(LineStyle(innerVColor, 1.0f));
				}

				// Draw vertical lines between columns
				for (u32 i = 0; i < state.columns.size(); i++)
				{
					if (!state.columns[i].isHidden)
					{
						// Draw left line for this column
						bool drawLeftLine = true;
						if (i == 0 && (hasOuter || innerOnly))
							drawLeftLine = false;

						if (drawLeftLine)
						{
							ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
						}
						currentX += state.columns[i].width;
					}
				}

				// Draw Rightmost line after all columns (skip if only inner borders)
				if (!innerOnly)
				{
					ctx->renderer->cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
					ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
				}
			}
		}

		// Draw Outer Box (Top and Bottom only if vertical lines cover sides?)
		// To be safe and ensure corners are perfect:
		if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter))
		{
			// Top Line (at tableRect.y)
			ctx->renderer->cmdSetLineStyle(LineStyle(outerHColor, 1.0f));
			ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
									   Point(state.tableRect.x + state.innerWidth, state.tableRect.y));

			// Bottom Line (at finalHeight)
			ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y + finalHeight),
									   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + finalHeight));

			// If we didn't draw vertical lines (e.g. no BordersInner), we still need sides for BordersOuter
			if (!has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders))
			{
				// Draw Sides
				ctx->renderer->cmdSetLineStyle(LineStyle(outerVColor, 1.0f));
				ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
										   Point(state.tableRect.x, state.tableRect.y + finalHeight));
				ctx->renderer->cmdDrawLine(Point(state.tableRect.x + state.innerWidth, state.tableRect.y),
										   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + finalHeight));
			}
		}
	}

	// Pop table clip rect if it was pushed
	if (state.hasTableClip)
	{
		ctx->renderer->popClipRect();
	}

	ctx->tableStack.pop_back();
}

void startHeader()
{
	auto& state = currentTable();
	state.isInHeader = true;
	state.currentColumn = 0;

	// Reset row parameters
	state.rowStartY = state.currentRowY;
	state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();
	state.currentMaxRowHeight = state.rowHeight; // Use theme default height as min

	// Setup for first cell
	state.cellStartY = state.rowStartY;

	if (state.currentColumn < state.columns.size())
	{
		ctx->layout.width = state.columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
		ctx->position.x = state.tableRect.x + ctx->cellPadding.x;
		ctx->position.y = state.rowStartY + ctx->cellPadding.y;

		// Start Clipping for first cell
		if (state.isClipping) ctx->renderer->popClipRect(); // Should not happen here usually, but safe
		Rect clipRect(state.tableRect.x, state.rowStartY, state.columns[state.currentColumn].width, 99999.0f);
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
	state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();
	state.currentMaxRowHeight = 3;// state.rowHeight; // Use theme default height as min

	state.cellStartY = state.rowStartY;

	// Draw row background BEFORE any content (use large height, will be clipped/overdrawn)
	// Draw custom row color if set
	if (state.currentRowColorSet)
	{
		Rect rowRect(
			state.tableRect.x,
			state.rowStartY,
			state.innerWidth,
			9999.0f  // Use large height, actual row height not known yet
		);

		rowRect = rowRect.contract(1.0);

		ctx->renderer->cmdSetColor(state.currentRowColor);
		ctx->renderer->cmdDrawFilledRectangle(rowRect);
	}
	// Draw alternating row background if enabled and no custom color
	else if (has(state.flags, TableFlags::AltRowBg))
	{
		Rect rowRect(
			state.tableRect.x,
			state.rowStartY,
			state.innerWidth,
			9999.0f  // Use large height, actual row height not known yet
		);

		rowRect = rowRect.contract(1.0);
		auto& tableBodyElem = ctx->theme->getElement(WidgetElementId::TableBody);
		
		if (state.currentRow % 2 == 1)
		{
			ctx->renderer->cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor0", Color::transparent));
		}
		else
		{
			ctx->renderer->cmdSetColor(tableBodyElem.currentStyle->getColorParameter("rowAltBgColor1", Color::transparent));
		}
		
		ctx->renderer->cmdDrawFilledRectangle(rowRect);
	}

	// Setup for first cell
	if (state.currentColumn < state.columns.size())
	{
		ctx->layout.width = state.columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
		ctx->position.x = state.tableRect.x + ctx->cellPadding.x;
		ctx->position.y = state.rowStartY + ctx->cellPadding.y;

		// Start Clipping
		// Note: We don't know the full row height yet, so we clip to a large height or wait?
		// But widgets are drawn immediately. We must clip now.
		// Since we handle dynamic row height, maybe we clip 9999 height?
		// Or update clip rect later? Creating a clip rect with limited width and "infinite" height
		// (clipped by parent window/panel) is the standard way to handle auto-height cells.
		Rect clipRect(state.tableRect.x, state.rowStartY, state.columns[state.currentColumn].width, 99999.0f);
		ctx->renderer->pushClipRect(clipRect);
		state.isClipping = true;
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

	if (state.currentColumn < state.columns.size())
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
		f32 cellX = state.tableRect.x;
		for (u32 i = 0; i < state.currentColumn && i < state.columns.size(); i++)
		{
			if (!state.columns[i].isHidden)
				cellX += state.columns[i].width;
		}

		if (state.currentColumn < state.columns.size())
		{
			ctx->layout.width = state.columns[state.currentColumn].width - (ctx->cellPadding.x * 2.0f);
			ctx->position.x = cellX + ctx->cellPadding.x;
			ctx->position.y = state.rowStartY + ctx->cellPadding.y; // Reset Y to top of row
			state.cellStartY = state.rowStartY; // New cell starts at row top

			// Push Clip
			f32 clipHeight = 99999.0f;
			Rect clipRect(cellX, state.rowStartY, state.columns[state.currentColumn].width, clipHeight);
			ctx->renderer->pushClipRect(clipRect);
			state.isClipping = true;
		}
	}
}


Rect getCellRect()
{
	auto& state = currentTable();

	if (state.currentColumn >= state.columns.size()) return Rect();

	// Calculate cell X position
	f32 cellX = state.tableRect.x;

	for (u32 i = 0; i < state.currentColumn; i++)
	{
		if (!state.columns[i].isHidden)
			cellX += state.columns[i].width;
	}

	// Get current column width (no spanning)
	f32 cellWidth = 0;
	if (!state.columns[state.currentColumn].isHidden)
	{
		cellWidth = state.columns[state.currentColumn].width;
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

	// Draw cell background immediately (single column width only)
	// Calculate cell X position
	f32 cellX = state.tableRect.x;
	for (u32 i = 0; i < state.currentColumn && i < state.columns.size(); i++)
	{
		if (!state.columns[i].isHidden)
			cellX += state.columns[i].width;
	}

	// Use current column width (no spanning)
	f32 cellWidth = 0;
	if (state.currentColumn < state.columns.size() && !state.columns[state.currentColumn].isHidden)
	{
		cellWidth = state.columns[state.currentColumn].width;
	}

	Rect cellRect(cellX, state.rowStartY, cellWidth, 9999.0f);  // Use large height
	cellRect = cellRect.contract(1.0);

	ctx->renderer->cmdSetColor(state.currentCellColor);
	ctx->renderer->cmdDrawFilledRectangle(cellRect);
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