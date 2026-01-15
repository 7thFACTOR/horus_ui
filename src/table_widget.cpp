#include "horus.h"
#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "util.h"
#include <map>

namespace hui
{
struct TablePersistentState
{
	struct ColumnState
	{
		f32 width = 100.0f;
		f32 specifiedSize = 0.0f; // User-specified size (percentage or pixels)
		bool isPercentage = false; // If true, specifiedSize is 0..1 percentage
		bool isFillRemaining = false; // If true, this column fills remaining space
		bool isHidden = false;
		bool isStretchable = true; // Track if this column should participate in auto-stretch
	};

	std::vector<ColumnState> columns;
	bool initialized = false;
};

static std::vector<TableState> tableStack;
static std::map<WidgetId, TablePersistentState> persistentStates;
static const f32 cellPaddingX = 2.0f;
static const f32 cellPaddingY = 2.0f;

static TableState& currentTable()
{
	static TableState dummy;
	if (tableStack.empty())
		return dummy;
	return tableStack.back();
}

static void finishRow(TableState& state)
{
	// Calculate height of the last cell in the row
	// Note: ctx->position.y points to where the next widget would go, so it represents the bottom of content
	f32 lastCellHeight = ctx->position.y - state.cellStartY + cellPaddingY;
	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, lastCellHeight);

	// Ensure row has at least the minimum height (e.g. from theme)
	state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, state.rowHeight);

	// Draw background for this row using deferred drawing
	// Note: If we are in the header, startHeader already drew the background and borders.
	// We should only draw here for body rows.
	if (!state.isInHeader)
	{
		ctx->renderer->beginDrawCmdInsertion(state.rowDrawCmdIndex);

		// Draw alternating row background if enabled
		if (has(state.flags, TableFlags::AltRowBg) && (state.currentRow % 2 == 1))
		{
			Rect rowRect(
				state.tableRect.x,
				state.rowStartY,
				state.innerWidth,
				state.currentMaxRowHeight
			);

			rowRect = rowRect.contract(1.0);

			ctx->renderer->cmdSetColor(Color::fromU8(60, 60, 60, 255));
			ctx->renderer->cmdDrawFilledRectangle(rowRect);
		}

		ctx->renderer->endDrawCmdInsertion();

		// Store the bottom Y position of this row for deferred border drawing
		state.rowSeparators.push_back(state.rowStartY + state.currentMaxRowHeight);

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

		auto& bodyElem = ctx->theme->getElement(WidgetElementId::ColumnsHeaderBody);
		auto& bodyElemState = bodyElem.normalState();

		// Draw header background
		ctx->renderer->cmdSetColor(bodyElemState.color);
		ctx->renderer->cmdDrawFilledRectangle(state.headerRect);

		// Draw vertical separators between columns
		f32 currentX = state.headerRect.x;
		bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
		bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);
		
		ctx->renderer->cmdSetLineStyle(LineStyle(Color::white, 1.0f));

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

		// Draw header bottom border AFTER vertical separators
		ctx->renderer->cmdDrawLine(
			Point(state.headerRect.x, state.headerRect.y + state.headerRect.height),
			Point(state.headerRect.x + state.headerRect.width, state.headerRect.y + state.headerRect.height)
		);

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
	auto& persistent = persistentStates[tableId];

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
	state.rowSpanInfo.clear(); // Clear span info for all rows

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
		
		if (pCol.isFillRemaining)
		{
			fillCount++;
		}
		else if (pCol.specifiedSize > 0)
		{
			if (pCol.isPercentage)
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
	
	// Distribute remaining space to fill columns
	if (fillCount > 0)
	{
		f32 remainingWidth = widgetWidth - specifiedWidth;
		if (remainingWidth > 0)
		{
			f32 widthPerFillColumn = remainingWidth / fillCount;
			for (u32 i = 0; i < columnCount; i++)
			{
				if (!state.columns[i].isHidden && persistent.columns[i].isFillRemaining)
				{
					state.columns[i].width = widthPerFillColumn;
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
	tableStack.push_back(state);

	// Get row height from theme
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::ColumnsHeaderBody);
	auto& bodyElemState = bodyElem.normalState();
	tableStack.back().rowHeight = bodyElemState.height > 0 ? bodyElemState.height : 25.0f;
	state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();

	return true;
}

void endTable()
{
	if (tableStack.empty()) return;
	auto& state = tableStack.back();

	// Finish the last row
	finishRow(state);

	f32 finalHeight = state.currentRowY - state.tableRect.y;

	// Pop any remaining clip rect BEFORE drawing borders so they don't get clipped
	if (state.isClipping)
	{
		ctx->renderer->popClipRect();
		state.isClipping = false;
	}

	ctx->renderer->cmdSetLineStyle(LineStyle(Color::white, 1.0f));

	// Batch draw borders if enabled
	if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersInner))
	{
		// Draw Inner Horizontal Lines
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders))
		{
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
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders))
		{
			bool innerOnly = has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders) && !has(state.flags, TableFlags::BordersOuter);
			bool hasOuter = has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter);

			// Draw vertical lines for each row
			for (u32 rowIdx = 0; rowIdx < state.rowSpanInfo.size(); rowIdx++)
			{
				f32 lineStartY = rowIdx < state.rowSeparators.size() && rowIdx > 0 ? state.rowSeparators[rowIdx - 1] : state.bodyStartY;
				f32 lineEndY = rowIdx < state.rowSeparators.size() ? state.rowSeparators[rowIdx] : state.currentRowY;
				
				f32 currentX = state.tableRect.x;
				const auto& rowSpans = state.rowSpanInfo[rowIdx];

				// Draw leftmost line if outer borders are needed
				if (hasOuter)
				{
					ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
				}

				// Draw vertical lines between columns
				for (u32 i = 0; i < state.columns.size(); i++)
				{
					if (!state.columns[i].isHidden)
					{
						// Check if this column is inside a span from a previous column
						bool isInsideSpan = false;
						for (u32 j = 0; j < i && j < rowSpans.size(); j++)
						{
							if (rowSpans[j] > 0)
							{
								// Check if this column i is within the span starting at j
								if (i < j + rowSpans[j])
								{
									isInsideSpan = true;
									break;
								}
							}
						}
						
						// Draw left line for this column
						bool drawLeftLine = true;
						if (i == 0 && (hasOuter || innerOnly))
							drawLeftLine = false;
						if (isInsideSpan)
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
					ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
				}
			}
		}

		// Draw Outer Box (Top and Bottom only if vertical lines cover sides?)
		// To be safe and ensure corners are perfect:
		if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter))
		{
			// Top Line (at tableRect.y)
			ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
									   Point(state.tableRect.x + state.innerWidth, state.tableRect.y));

			// Bottom Line (at finalHeight)
			ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y + finalHeight),
									   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + finalHeight));

			// If we didn't draw vertical lines (e.g. no BordersInner), we still need sides for BordersOuter
			if (!has(state.flags, TableFlags::BordersInner) && !has(state.flags, TableFlags::Borders))
			{
				// Draw Sides
				ctx->renderer->cmdDrawLine(Point(state.tableRect.x, state.tableRect.y),
										   Point(state.tableRect.x, state.tableRect.y + finalHeight));
				ctx->renderer->cmdDrawLine(Point(state.tableRect.x + state.innerWidth, state.tableRect.y),
										   Point(state.tableRect.x + state.innerWidth, state.tableRect.y + finalHeight));
			}
		}
	}

	tableStack.pop_back();
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
		ctx->layout.width = state.columns[state.currentColumn].width - (cellPaddingX * 2.0f);
		ctx->position.x = state.tableRect.x + cellPaddingX;
		ctx->position.y = state.rowStartY + cellPaddingY;

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
	
	// Add new row for span info
	state.rowSpanInfo.push_back(std::vector<u32>(state.columns.size(), 0));

	state.rowStartY = state.currentRowY;
	state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();
	state.currentMaxRowHeight = 3;// state.rowHeight; // Use theme default height as min

	state.cellStartY = state.rowStartY;

	// Setup for first cell
	if (state.currentColumn < state.columns.size())
	{
		ctx->layout.width = state.columns[state.currentColumn].width - (cellPaddingX * 2.0f);
		ctx->position.x = state.tableRect.x + cellPaddingX;
		ctx->position.y = state.rowStartY + cellPaddingY;

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
		f32 finishedCellHeight = ctx->position.y - state.cellStartY + cellPaddingY; 
		state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, finishedCellHeight);
		
		// Advance by the span amount (default is 1)
		state.currentColumn += state.currentColSpan;
		// Reset span for next cell
		state.currentColSpan = 1;

		// Move to next column
		f32 cellX = state.tableRect.x;
		for (u32 i = 0; i < state.currentColumn && i < state.columns.size(); i++)
		{
			if (!state.columns[i].isHidden)
				cellX += state.columns[i].width;
		}

		if (state.currentColumn < state.columns.size())
		{
			ctx->layout.width = state.columns[state.currentColumn].width - (cellPaddingX * 2.0f);
			ctx->position.x = cellX + cellPaddingX;
			ctx->position.y = state.rowStartY + cellPaddingY; // Reset Y to top of row
			state.cellStartY = state.rowStartY; // New cell starts at row top

			// Push Clip
			f32 clipHeight = 99999.0f;
			Rect clipRect(cellX, state.rowStartY, state.columns[state.currentColumn].width, clipHeight);
			ctx->renderer->pushClipRect(clipRect);
			state.isClipping = true;
		}
	}
}

void setCellSpan(u32 colSpan, u32 rowSpan)
{
	auto& state = currentTable();
	if (colSpan > 1)
	{
		// Store the span for use in nextCell()
		state.currentColSpan = colSpan;
		
		// Record span info for border drawing in the current row
		if (!state.rowSpanInfo.empty() && state.currentColumn < state.rowSpanInfo.back().size())
		{
			state.rowSpanInfo.back()[state.currentColumn] = colSpan;
		}

		// Calculate total width of spanned columns
		f32 spanWidth = 0;
		for (u32 i = 0; i < colSpan && (state.currentColumn + i) < state.columns.size(); i++)
		{
			if (!state.columns[state.currentColumn + i].isHidden)
				spanWidth += state.columns[state.currentColumn + i].width;
		}

		// Update layout width to cover spanned columns
		ctx->layout.width = spanWidth - (cellPaddingX * 2.0f);

		// Pop current clip rect and push new one covering the span
		if (state.isClipping)
		{
			ctx->renderer->popClipRect();
		}

		// Calculate cell X position
		f32 cellX = state.tableRect.x;
		for (u32 i = 0; i < state.currentColumn && i < state.columns.size(); i++)
		{
			if (!state.columns[i].isHidden)
				cellX += state.columns[i].width;
		}

		// Push new clip rect with spanned width
		f32 clipHeight = 99999.0f;
		Rect clipRect(cellX, state.rowStartY, spanWidth, clipHeight);
		ctx->renderer->pushClipRect(clipRect);
		state.isClipping = true;
	}
}

void setupColumn(u32 columnIndex, f32 size, bool isFillRemaining)
{
	if (tableStack.empty()) return;
	auto& state = tableStack.back();
	
	// Get persistent state
	auto iter = persistentStates.find(state.id);
	if (iter == persistentStates.end()) return;
	auto& persistent = iter->second;
	
	if (columnIndex >= persistent.columns.size()) return;
	
	persistent.columns[columnIndex].specifiedSize = size;
	// Auto-detect: values <= 1 are percentages, values > 1 are pixels
	persistent.columns[columnIndex].isPercentage = (size <= 1.0f && size > 0.0f && !isFillRemaining);
	persistent.columns[columnIndex].isFillRemaining = isFillRemaining;
}

}