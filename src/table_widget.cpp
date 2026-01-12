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
		f32 userWidth = 100.0f; // Width explicitly set by user interaction
		bool isHidden = false;
		bool isStretchable = true; // Track if this column should participate in auto-stretch
		bool manuallyResized = false;
	};

	std::vector<ColumnState> columns;
	bool resizingColumn = false;
	u32 resizingColumnIndex = ~0;
	Point lastMousePos;
	bool initialized = false;
	f32 resizeStartX = 0; // Starting X coordinate of the column being resized
};

static std::vector<TableState> tableStack;
static std::map<WidgetId, TablePersistentState> persistentStates;
static const f32 cellPaddingX = 5.0f;
static const f32 cellPaddingY = 3.0f;

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
	f32 lastCellHeight = ctx->position.y - state.cellStartY + (cellPaddingY * 2.0f);
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
			ctx->renderer->cmdSetColor(Color::fromU8(125, 125, 125, 255));
			ctx->renderer->cmdDrawFilledRectangle(rowRect);
		}

		ctx->renderer->endDrawCmdInsertion();

		// Store the bottom Y position of this row for deferred border drawing
		state.rowSeparators.push_back(state.rowStartY + state.currentMaxRowHeight);

		// Advance Y position ONLY if we actually finished a row (not header)
		state.currentRowY += state.currentMaxRowHeight;
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
			persistent.columns[i].userWidth = 100.0f;
			persistent.columns[i].isHidden = false;
			persistent.columns[i].isStretchable = has(flags, TableFlags::Stretch);
			persistent.columns[i].manuallyResized = false;
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

	// Copy persistent data to transient state
	for (u32 i = 0; i < columnCount; i++)
	{
		state.columns[i].width = persistent.columns[i].width;
		state.columns[i].minWidth = 50.0f;
		state.columns[i].maxWidth = 500.0f;
		state.columns[i].isResizable = has(flags, TableFlags::Resizable);

		// If manually resized, disable stretching for this column
		if (persistent.columns[i].manuallyResized)
			state.columns[i].isStretchable = false;
		else
			state.columns[i].isStretchable = persistent.columns[i].isStretchable; // Should match persistent/flags

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

	// Stretch logic: distribute extra space
	if (has(flags, TableFlags::Stretch) && widgetWidth > totalColumnsWidth)
	{
		f32 extraWidth = widgetWidth - totalColumnsWidth;
		u32 stretchableCount = 0;

		for (const auto& col : state.columns)
			if (col.isStretchable && !col.isHidden) stretchableCount++;

		if (stretchableCount > 0)
		{
			f32 widthPerColumn = extraWidth / stretchableCount;
			for (auto& col : state.columns)
				if (col.isStretchable && !col.isHidden) col.width += widthPerColumn;
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

	// Handle Resizing Logic Here (Global resize)
	if (has(flags, TableFlags::Resizable))
	{
		f32 currentX = state.tableRect.x;
		const f32 resizeGrabWidth = 8.0f;
		f32 resizeHeight = height > 0 ? height : 99999.0f; // Use huge height if auto, clipped by window/scissor ideally
		// Only clamp to resize in current view if needed, but for now assuming full height or large enough
		if (height <= 0) resizeHeight = ctx->renderer->getClipRect().height; // Approximation

		// Force resize cursor if currently resizing, regardless of hover
		if (persistent.resizingColumn)
			ctx->mouseCursor = MouseCursorType::SizeWE;

		for (u32 i = 0; i < state.columns.size(); i++)
		{
			if (state.columns[i].isHidden) continue;

			// currentX is the START of column i
			f32 columnStartX = currentX;
			currentX += state.columns[i].width;
			// currentX is now the END of column i (separator position)

			if (state.columns[i].isResizable)
			{
				// Splitter rect centered at the end of the column
				Rect resizeRect(currentX - resizeGrabWidth/2, state.tableRect.y, resizeGrabWidth, resizeHeight);

				bool hovered = resizeRect.contains(ctx->mousePosition);

				if (persistent.resizingColumn)
				{
					if (persistent.resizingColumnIndex == i)
					{
						if (ctx->event.type == InputEvent::Type::MouseUp)
						{
							persistent.resizingColumn = false;
							persistent.resizingColumnIndex = ~0;
						}
						else if (ctx->event.type == InputEvent::Type::MouseMove)
						{
							// Calculate NEW width based on absolute mouse position relative to column start
							// This avoids the 'delta' issue when sticking at min/max limits
							f32 newWidth = ctx->mousePosition.x - persistent.resizeStartX;

							// Enforce constraints on newWidth
							if (newWidth < state.columns[i].minWidth)
								newWidth = state.columns[i].minWidth;
							if (newWidth > state.columns[i].maxWidth)
								newWidth = state.columns[i].maxWidth;

							// Calculate delta for userWidth update if needed, or just set it
							// But we need to update persistent state
							persistent.columns[i].width = newWidth;
							persistent.columns[i].userWidth = newWidth;

							// Mark as manually resized
							persistent.columns[i].manuallyResized = true;
							persistent.columns[i].isStretchable = false;
							tableStack.back().columns[i].isStretchable = false;

							// Update transient state
							tableStack.back().columns[i].width = newWidth;

							persistent.lastMousePos = ctx->mousePosition;
							ctx->mustRedraw = true;
						}
					}
				}
				else if (hovered)
				{
					ctx->mouseCursor = MouseCursorType::SizeWE;
					if (ctx->event.type == InputEvent::Type::MouseDown && ctx->event.mouse.button == MouseButton::Left)
					{
						persistent.resizingColumn = true;
						persistent.resizingColumnIndex = i;
						persistent.lastMousePos = ctx->mousePosition;
						// Store the starting X of this column so we can calculate width from mouse pos
						persistent.resizeStartX = columnStartX;

						// Lock ALL columns to their current visual size on first interaction.
						// This prevents other columns from shifting/resizing due to "Stretch" logic
						// as soon as we start modifying one.
						for (u32 k = 0; k < state.columns.size(); k++)
						{
							if (persistent.columns[k].isStretchable)
							{
								// Bake the current stretched width into the persistent width
								persistent.columns[k].width = state.columns[k].width;
								persistent.columns[k].userWidth = state.columns[k].width;
								persistent.columns[k].isStretchable = false;
								persistent.columns[k].manuallyResized = true;

								// Update transient state immediately to match
								tableStack.back().columns[k].isStretchable = false;
							}
						}
					}
				}
			}
		}
	}

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

	// Batch draw borders if enabled
	if (has(state.flags, TableFlags::Borders) || has(state.flags, TableFlags::BordersOuter) || has(state.flags, TableFlags::BordersInner))
	{
		ctx->renderer->cmdSetLineStyle(LineStyle(Color::fromU8(200, 200, 200, 255), 1.0f));

		// Draw Inner Horizontal Lines
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders))
		{
			for (size_t i = 0; i < state.rowSeparators.size(); i++)
			{
				bool draw = true;
				// Skip last line if outer borders are drawn (because outer border draws the bottom
				// line, and we don't want to draw over it or double draw)
				// However, if we want to ensure robustness, we can draw it.
				// Let's stick to standard behavior: if BordersOuter is ON, skip last separator.
				if (i == state.rowSeparators.size() - 1 && has(state.flags, TableFlags::Borders))
					draw = false;

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
		// We iterate columns and draw Left side of each.
		if (has(state.flags, TableFlags::BordersInner) || has(state.flags, TableFlags::Borders))
		{
			f32 currentX = state.tableRect.x;
			f32 lineStartY = state.bodyStartY; // Start below header if present
			f32 lineEndY = state.currentRowY;   // End at table bottom

			for (u32 i = 0; i < state.columns.size(); i++)
			{
				if (!state.columns[i].isHidden)
				{
					bool drawLine = true;
					// Logic for first line (Leftmost)
					// If Borders (Outer) is ON, it draws the box. Do we want double draw?
					// Optimization request says "draw border on top at the end".
					// Drawing the box is fine, but maybe redundant if we draw per-column.
					// Let's draw ALL vertical dividers here.

					ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));

					currentX += state.columns[i].width;

					// Draw Rightmost line of last column
					if (i == state.columns.size() - 1)
					{
						ctx->renderer->cmdDrawLine(Point(currentX, lineStartY), Point(currentX, lineEndY));
					}
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

	// Get theme info
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::ColumnsHeaderBody);
	auto& bodyElemState = bodyElem.normalState();
	f32 headerHeight = bodyElemState.height > 0 ? bodyElemState.height : 25.0f;

	state.headerRect = Rect(
		state.tableRect.x,
		state.tableRect.y,
		state.innerWidth,
		headerHeight
	);

	// Draw header background
	ctx->renderer->cmdSetColor(bodyElemState.color);
	ctx->renderer->cmdDrawFilledRectangle(state.headerRect);

	// Draw vertical separators between columns
	f32 currentX = state.headerRect.x;
	for (u32 i = 0; i < state.columns.size(); i++)
	{
		if (!state.columns[i].isHidden)
		{
			// Draw Left line for each column
			ctx->renderer->cmdSetLineStyle(LineStyle(Color::fromU8(180, 180, 180, 255), 1.0f));

			// Full height now, because bottom border will draw over it ideally
			ctx->renderer->cmdDrawLine(
				Point(currentX, state.headerRect.y),
				Point(currentX, state.headerRect.y + state.headerRect.height)
			);

			currentX += state.columns[i].width;

			if (i == state.columns.size() - 1)
			{
				// Draw Right line for last column
				ctx->renderer->cmdDrawLine(
					Point(currentX, state.headerRect.y),
					Point(currentX, state.headerRect.y + state.headerRect.height)
				);
			}
		}
	}

	// Draw header bottom border AFTER vertical separators
	ctx->renderer->cmdSetLineStyle(LineStyle(Color::fromU8(150, 150, 150, 255), 2.0f)); // Thicker line
	ctx->renderer->cmdDrawLine(
		Point(state.headerRect.x, state.headerRect.y + state.headerRect.height),
		Point(state.headerRect.x + state.headerRect.width, state.headerRect.y + state.headerRect.height)
	);

	// Set position for first header cell
	state.currentRowY = state.tableRect.y + headerHeight;
	state.bodyStartY = state.currentRowY; // Update Start of Body to be below header

	if (state.currentColumn < state.columns.size())
	{
		ctx->layout.width = state.columns[state.currentColumn].width - (cellPaddingX * 2.0f);
		ctx->position.x = state.tableRect.x + cellPaddingX;
		ctx->position.y = state.rowStartY + cellPaddingY;
	}
}

void nextRow()
{
	auto& state = currentTable();

	// Finish previous row
	finishRow(state);

	// Start new row
	state.currentRow++;
	state.currentColumn = 0;
	state.isInHeader = false;

	state.rowStartY = state.currentRowY;
	state.rowDrawCmdIndex = ctx->renderer->getDrawCommandCount();
	state.currentMaxRowHeight = state.rowHeight; // Use theme default height as min

	state.cellStartY = state.rowStartY;

	// Setup for first cell
	if (state.currentColumn < state.columns.size())
	{
		ctx->layout.width = state.columns[state.currentColumn].width - (cellPaddingX * 2.0f);
		ctx->position.x = state.tableRect.x + cellPaddingX;
		ctx->position.y = state.rowStartY + cellPaddingY;
	}
}

void nextCell()
{
	auto& state = currentTable();

	if (state.currentColumn < state.columns.size())
	{
		// Calculate height of the cell we just finished
		// Note: ctx->position.y points to where the next widget would go, so it represents the bottom of content
		f32 finishedCellHeight = ctx->position.y - state.cellStartY + (cellPaddingY * 2.0f);
		state.currentMaxRowHeight = std::max(state.currentMaxRowHeight, finishedCellHeight);

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
			ctx->layout.width = state.columns[state.currentColumn].width - (cellPaddingX * 2.0f);
			ctx->position.x = cellX + cellPaddingX;
			ctx->position.y = state.rowStartY + cellPaddingY; // Reset Y to top of row
			state.cellStartY = state.rowStartY; // New cell starts at row top
		}
	}
}

void setCellSpan(u32 colSpan, u32 rowSpan)
{
	auto& state = currentTable();
	if (colSpan > 1)
	{
		state.currentColumn += (colSpan - 1);
	}
}

}