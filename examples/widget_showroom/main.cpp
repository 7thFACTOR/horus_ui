#pragma execution_character_set("utf-8")
#include "horus.h"
#include "sdl2_input_provider.h"
#include "opengl_graphics_provider.h"
#include <string.h>
#include <string>
#include <unordered_map>

using namespace hui;
char str[1500] = { 0 };
char str2[1500] = { 0 };
bool checks[100] = { false };
bool changeScale = false;
f32 scale = 1.f;
Font fntBig;
Font fntVeryBig;
Font fntBold;
Font fntItalic;
Font fntLed1;
Font fntLed2;
Font fntLed3;
Image lenaImg;
Image nodeBodyImg;
Image drawLineImg;
Font fntNodeTitle = 0;
ViewPaneTab console1Tab;
ViewPaneTab console2Tab;
Image tabIcon1;
Image tabIcon2;
Image tabIcon3;
Image deleteIcon;
Image sadIcon;
Image xaxisIcon;
Image yaxisIcon;
Image zaxisIcon;
Image targetIcon;
Image clearIcon;
MouseCursor dragDropCur;

char* stristr(const char* haystack, const char* needle)
{
	do
	{
		const char* h = haystack;
		const char* n = needle;
		while (tolower((unsigned char)*h) == tolower((unsigned char)*n) && *n)
		{
			h++;
			n++;
		}
		if (*n == 0) {
			return (char *)haystack;
		}
	} while (*haystack++);
	return 0;
}

struct MyCustomTabData
{
	bool showAddComponent = false;
	bool focusEditBox = false;
	static const int strSearchMaxSize = 256;
	char strSearch[strSearchMaxSize] = { 0 };
	f32 scrollPosNewComponentList = 0;
	char strNewCompScript[1000] = { "" };
	f32 scrollPosComponents = 0;
	std::vector<std::string> components;
	std::vector<bool> componentPanelExpanded;
	std::vector<bool> componentPanelEnabled;
} customPaneData[2];

struct MyViewHandler : hui::ViewHandler
{
	Image moveIcon = 0;
	Image playIcon = 0;
	Image stopIcon = 0;
	Image pauseIcon = 0;
	Image horusLogo = 0;
	bool chooseColorPopup = false;
	bool exitAppMsgBox = false;
	bool moreInfoMsgBox = false;
	Point chooseColorPopupPos;
	Point moreInfoBoxPos;
	MyCustomTabData customData[2];
	Color color = Color::red;

	void onTopAreaRender(Window wnd) override
	{
		hui::beginMenuBar();

		if (hui::beginMenu("File"))
		{
			if (hui::menuItem("New...", "Ctrl+N"))
			{
				printf("new\n");
			}

			if (hui::menuItem("Open...", "Ctrl+O", tabIcon1))
			{
				printf("open\n");
				char file[1000] = { 0 };

				if (hui::openFileDialog("png;zip;exe;jpg", "file.png", file, 1000))
				{
					printf("File open: '%s'\n", file);
				}
			}

			if (hui::menuItem("Open many files...", "Ctrl+Alt+O", tabIcon1))
			{
				printf("open many\n");
				OpenMultipleFileSet fs;

				if (hui::openMultipleFileDialog("png;zip;exe;jpg", "file.png", fs))
				{

					for (int i = 0; i < fs.count; i++)
						printf("File open#%d: '%s'\n", i, fs.filenameBuffer + fs.bufferIndices[i]);
				}
			}

			if (hui::menuItem("Save", "Ctrl+S"))
			{
				printf("save\n");
				char file[1000] = { 0 };

				if (hui::saveFileDialog("*.png", "file.png", file, 1000))
				{
					printf("File save: '%s'\n", file);
				}
			}

			if (hui::menuItem("Pick folder...", "Ctrl+F"))
			{
				printf("pick folder\n");
				char file[1000] = { 0 };

				if (hui::pickFolderDialog("C:\\", file, 1000))
				{
					printf("Picked folder: '%s'\n", file);
				}
			}

			hui::menuSeparator();

			if (hui::menuItem("Export...", "Ctrl+E"))
			{
				printf("export\n");
			}

			if (hui::menuItem("Exit", "Ctrl+Alt+X"))
			{
				printf("exit\n");
				hui::quitApplication();
			}

			hui::endMenu();
		}

		hui::endMenuBar();

		if (1){
			pushSpacing(5);
			static bool tbDown[10] = { 0 };
			hui::pushSameLineSpacing(5);
			beginSameLine();
			hui::beginToolbar();
			pushPadding(0);
			if (hui::toolbarButton(moveIcon, 0, tbDown[0]))
			{
				tbDown[0] = true;
				tbDown[1] = false;
			}

			if (hui::toolbarButton(moveIcon, 0, tbDown[1]))
			{
				tbDown[0] = false;
				tbDown[1] = true;
			}
			hui::toolbarSeparator();

			static bool showPopupToolbar = false;

			if (hui::toolbarButton(moveIcon))
				showPopupToolbar = true;

			if (showPopupToolbar)
			{
				beginPopup(38, PopupPositionMode::BelowLastWidget, hui::Point(), WidgetElementId::ButtonBody);
				beginToolbar(ToolbarDirection::Vertical);
				toolbarButton(moveIcon);
				toolbarButton(moveIcon);
				toolbarButton(moveIcon);
				endToolbar();

				if (mustClosePopup())
				{
					showPopupToolbar = false;
					closePopup();
				}

				endPopup();
			}

			hui::toolbarGap();

			hui::toolbarButton(moveIcon);
			hui::tooltip("Full Border");

			if (beginContextMenu(ContextMenuFlags::AllowLeftClickOpen))
			{
				menuItem("Cut", "Ctrl+X");
				hui::tooltip("Cut the current selection");
				menuItem("Copy", "Ctrl+C");
				hui::tooltip("Copy the current selection");
				menuItem("Paste", "Ctrl+V");
				menuSeparator();
				menuItem("Delete All", "Delete");
				endContextMenu();
			}

			hui::toolbarButton(stopIcon);
			hui::endToolbar();
			popPadding();
			hui::pushWidth(70);
			hui::button("Add");
			hui::button("Duplicate");
			hui::button("Erase");
			hui:check("Activate", true);
			hui::label("Simple label");
			static float val = 0;
			hui::comboSliderFloat(val);
			hui::radio("Add", true);
			hui::radio("Mul", false);
			hui::radio("Xor", false);
			hui::popWidth();
			endSameLine();
			hui::popSameLineSpacing();
			popSpacing();
		}
	}

	void onLeftAreaRender(Window window) override
	{
		static bool tbDown[10] = { 0 };
		pushPadding(1);
		hui::pushSameLineSpacing(5);
		hui::beginToolbar(ToolbarDirection::Vertical);
		if (hui::toolbarButton(moveIcon, 0, tbDown[0]))
		{
			tbDown[0] = true;
			tbDown[1] = false;
		}

		if (hui::toolbarButton(moveIcon, 0, tbDown[1]))
		{
			tbDown[0] = false;
			tbDown[1] = true;
		}
		hui::toolbarSeparator();

		static bool showPopupToolbar = false;

		if (hui::toolbarButton(moveIcon))
			showPopupToolbar = true;

		if (showPopupToolbar)
		{
			beginPopup(200, PopupPositionMode::RightSideLastWidget, hui::Point(), WidgetElementId::ButtonBody);
			beginToolbar();
			//toolbarButton(moveIcon);
			//toolbarButton(moveIcon);
			//toolbarButton(moveIcon);
			endToolbar();

			if (mustClosePopup())
			{
				showPopupToolbar = false;
				closePopup();
			}

			endPopup();
		}

		//hui::toolbarGap();

		hui::toolbarButton(moveIcon);
		hui::tooltip("Full Border");
		if (beginContextMenu(ContextMenuFlags::AllowLeftClickOpen))
		{
			menuItem("Cut", "Ctrl+X");
			hui::tooltip("Cut the current selection");
			menuItem("Copy", "Ctrl+C");
			hui::tooltip("Copy the current selection");
			menuItem("Paste", "Ctrl+V");
			menuSeparator();
			menuItem("Delete All", "Delete");
			endContextMenu();
		}

		hui::toolbarButton(stopIcon);
		hui::endToolbar();
		hui::popSameLineSpacing();
		popPadding();
	}

	void onBottomAreaRender(Window window) override
	{
		space();
		label("   HINT OF THE DAY: Roll over to get cookie.");
	}

	void onRightAreaRender(Window window) override
	{
		button("Left area");
	}

	void onAfterFrameRender(Window wnd) override
	{
		if (changeScale)
			hui::setGlobalScale(scale);

		changeScale = false;

		if (hui::wantsToQuit())
		{
			hui::quitApplication();
		}
	}

	void drawFlowNode(const char* name, const Rect& rc)
	{
		hui::Rect box = rc;

		box.y += 6;

		hui::drawBorderedImage(nodeBodyImg, 6, rc);
		hui::setFont(fntNodeTitle);
		hui::drawTextInBox(name, box, hui::HAlignType::Center, hui::VAlignType::Top);
	}

	void onViewRender(Window wnd, ViewPane viewPane, ViewId viewId, u64 userDataId) override
	{
		Rect viewRect = hui::getViewPaneRect(viewPane);

		switch (viewId)
		{
		case 0:
		{
			static char searchStr[100] = { 0 };
			hui::space();
			hui::textInput(searchStr, 100, TextInputValueMode::Any, "Search asset name");
			hui::space();

			static f32 scrollPos = 0;
			hui::pushSpacing(0);
			u32 totalItemCount = 50000;
			f32 baseItemSize = 128;
			static f32 rowHeight = 128;
			f32 itemWidth = baseItemSize + hui::getPadding() * 2.0f;

			auto viewWidth = hui::getParentSize().x;
			int itemCountPerRow = viewWidth / itemWidth;
			f32 availablePaneHeight = hui::getRemainingViewPaneHeight(viewPane);
			f32 skipRowCount = scrollPos / rowHeight;
			f32 totalRowCount = ceilf((f32)totalItemCount / itemCountPerRow);
			f32 visibleRowCount = availablePaneHeight / rowHeight;

			hui::beginScrollView(availablePaneHeight, scrollPos);
			static bool isListFocused = false;
			hui::beginVirtualListContent(totalRowCount, rowHeight, scrollPos);
			static int selection = 0;

			for (int row = skipRowCount; row <= skipRowCount + visibleRowCount; row++)
			{
				bool itemVisible = true;
				hui::pushSpacing(0);
				f32 startY = hui::getPenPosition().y;
				hui::beginColumns(itemCountPerRow);

				for (int rowItemIndex = 0; rowItemIndex < itemCountPerRow; rowItemIndex++)
				{
					int index = row * itemCountPerRow + rowItemIndex;

					if (index < totalItemCount)
					{
						hui::beginBox(index == selection ? (isListFocused ? Color::cyan : Color::gray) : Color::transparent);

						hui::image(lenaImg, baseItemSize);

						if (index == selection && hui::isFocused())
						{
							isListFocused = true;
						}

						if (hui::isPressed())
							selection = index;

						if (hui::wantsToDragDrop())
						{
							static char str[100];
							sprintf(str, "File%d", index + 1);
							hui::beginDragDrop(0, str);
						}

						hui::endBox();


						char sss[111];

						sprintf(sss, "%d", index + 1);
						hui::beginBox(Color::black);
						if (index == selection && hui::isFocused())
						{
							isListFocused = true;
						}
						hui::label(sss, HAlignType::Center);
						if (index == selection && hui::isFocused())
						{
							isListFocused = true;
						}
						itemVisible = hui::isVisible();
						hui::endBox();
					}
					else
					{
						itemVisible = false;
					}

					if (rowItemIndex < itemCountPerRow - 1)
						hui::nextColumn();
				}

				hui::endColumns();
				auto rowH = hui::getPenPosition().y - startY;

				//TODO: bad height zero when last row reached
				if (rowH > 0)
				{
					rowHeight = rowH;
				}

				hui::popSpacing();
			}

			hui::endVirtualListContent();
			scrollPos = hui::endScrollView();
			hui::popSpacing();
			break;
		}
		case 1:
		{
			static bool showDxf = false;
			// userDataId here is an index into customPaneData
			MyCustomTabData& cdata = customPaneData[userDataId];

			hui::beginMenuBar();

			if (hui::beginMenu("File"))
			{
				if (hui::menuItem("New...", "Ctrl+N"))
				{
					printf("new\n");
				}

				if (hui::menuItem("Open...", "Ctrl+O", tabIcon1))
				{
					printf("open\n");
					char file[1000] = { 0 };

					if (hui::openFileDialog("png;zip;exe;jpg", "file.png", file, 1000))
					{
						printf("File open: '%s'\n", file);
					}
				}

				if (hui::menuItem("Save", "Ctrl+S"))
				{
					printf("save\n");
					char file[1000] = { 0 };

					if (hui::saveFileDialog("*.png", "file.png", file, 1000))
					{
						printf("File save: '%s'\n", file);
					}
				}

				if (hui::menuItem("Pick folder...", "Ctrl+F"))
				{
					printf("pick folder\n");
					char file[1000] = { 0 };

					if (hui::pickFolderDialog("C:\\", file, 1000))
					{
						printf("Picked folder: '%s'\n", file);
					}
				}

				hui::labelCustomFont("Custom items", fntBold);
				hui::line();
				hui::image(horusLogo, 25);
				hui::label("Velocity:");
				static f32 flt;
				hui::sliderFloat(0, 1, flt);
				static char opo[155] = { 0 };
				hui::textInput(opo, 155);

				u32 chk = 0;
				static bool autosave = false;

				if (autosave)
					chk = (u32)hui::SelectableFlags::Checked;

				if (hui::menuItem("Autosave", "", 0, (hui::SelectableFlags)((u32)hui::SelectableFlags::Checkable | chk)))
				{
					autosave = !autosave;
				}

				if (hui::beginMenu("Save As..."))
				{
					if (hui::menuItem("DXF", "Ctrl+Shift+X"))
					{
						showDxf = true;
					}

					if (hui::menuItem("OBJ", ""))
					{
						printf("obj\n");
					}

					if (hui::beginMenu("FBX"))
					{
						if (hui::menuItem("Other", ""))
						{
							printf("other\n");
						}

						if (hui::menuItem("Smart", "F4"))
						{
							printf("smart\n");
						}

						if (hui::menuItem("Coffee", "C"))
						{
							printf("coffee\n");
						}

						hui::endMenu();
					}

					hui::endMenu();
				}

				hui::menuSeparator();

				if (hui::menuItem("Export...", "Ctrl+E"))
				{
					printf("export\n");
				}

				if (hui::menuItem("Exit", "Ctrl+Alt+X"))
				{
					printf("exit\n");
				}

				hui::endMenu();
			}

			if (hui::beginMenu("Edit"))
			{
				if (hui::menuItem("Cut", "Ctrl+X"))
				{
					printf("cut\n");
				}

				if (hui::menuItem("Copy", "Ctrl+C"))
				{
					printf("copy\n");
				}

				if (hui::menuItem("Paste", "Ctrl+V"))
				{
					printf("paste\n");
				}

				hui::menuSeparator();

				if (hui::menuItem("Preferences...", "Ctrl+F8"))
				{
					printf("preferences\n");
				}

				hui::endMenu();
			}

			hui::endMenuBar();

			if (showDxf)
			{
				auto mbb = hui::messageBox("To be or not to be", "Once upon a time in a galaxy far far away there was a small green creature...", hui::MessageBoxButtons::OkCancel);

				if (!!(mbb & MessageBoxButtons::Ok) || !!(mbb & MessageBoxButtons::Cancel))
				{
					showDxf = false;
				}
			}

			hui::gap(10);

			if (1) {
				hui::pushPadding(1);

				static bool tbDown[10] = { 0 };

				hui::pushSameLineSpacing(5);
				beginSameLine();
				hui::beginToolbar();
				if (hui::toolbarButton(moveIcon, 0, tbDown[0]))
				{
					tbDown[0] = true;
					tbDown[1] = false;
				}

				if (hui::toolbarButton(moveIcon, 0, tbDown[1]))
				{
					tbDown[0] = false;
					tbDown[1] = true;
				}
				hui::toolbarSeparator();

				static bool showPopupToolbar = false;

				if (hui::toolbarButton(moveIcon))
					showPopupToolbar = true;

				if (showPopupToolbar)
				{
					beginPopup(38, PopupPositionMode::BelowLastWidget, hui::Point(), WidgetElementId::ButtonBody);
					beginToolbar();
					toolbarButton(moveIcon);
					toolbarButton(moveIcon);
					toolbarButton(moveIcon);
					endToolbar();

					if (mustClosePopup())
					{
						showPopupToolbar = false;
						closePopup();
					}

					endPopup();
				}

				hui::toolbarGap();

				hui::toolbarButton(moveIcon);
				hui::tooltip("Full Border");
				if (beginContextMenu(ContextMenuFlags::AllowLeftClickOpen))
				{
					menuItem("Cut", "Ctrl+X");
					hui::tooltip("Cut the current selection");
					menuItem("Copy", "Ctrl+C");
					hui::tooltip("Copy the current selection");
					menuItem("Paste", "Ctrl+V");
					menuSeparator();
					menuItem("Delete All", "Delete");
					endContextMenu();
				}

				hui::toolbarButton(stopIcon);
				hui::endToolbar();

				hui::pushWidth(70);
				//hui::beginSameLine();
				hui::button("Add");
				hui::button("Duplicate");
				hui::button("Erase");
			hui:check("Activate", true);
				hui::label("Simple label");
				static float val = 0;
				hui::comboSliderFloat(val);
				hui::radio("Add", true);
				hui::radio("Mul", false);
				hui::radio("Xor", false);
				hui::popWidth();
				endSameLine();
				hui::popSameLineSpacing();

				//hui::beginColumns(6, toolbarCols, toolbarCols, toolbarCols);
				//hui::iconButton(moveIcon, 20);
				//hui::nextColumn();
				//hui::iconButton(moveIcon, 20);
				//hui::nextColumn();
				//hui::iconButton(moveIcon, 20);
				//hui::nextColumn();
				//hui::check("Autofill", true);
				//hui::nextColumn();
				//static i32 sel = -1;
				//static const char* strs[] = { "Perspective", "Top", "Front", "Left" };
				//hui::dropdown(sel, strs, 4);
				//hui::nextColumn();
				//static char tx[100] = { 0 };
				//hui::textInput(tx, 100, TextInputValueMode::Any, "Search");
				//hui::endColumns();
				hui::popPadding();
			}
			WidgetElementInfo wel;

			hui::getThemeWidgetElementInfo(hui::WidgetElementId::ButtonBody, hui::WidgetStateType::Normal, wel);

			//hui::image(wel.image);
			//hui::beginViewport(100);
			//static f32 w = 60;
			//w += 0.1;
			//hui::drawBorderedImage(wel.image, 3, { 10, 10, w, w });
			//hui::endViewport();
			if (hui::button("Add component"))
			{
				cdata.showAddComponent = true;
				cdata.focusEditBox = true;
			}

			if (cdata.showAddComponent)
			{
				auto rc = hui::getWidgetRect();
				hui::beginPopup(rc.width, hui::PopupPositionMode::BelowLastWidget);

				if (cdata.focusEditBox)
				{
					hui::setFocused();
					cdata.focusEditBox = false;
				}

				f32 widths[] = { 1.f, 20.f };
				hui::pushPadding(0);
				hui::beginColumns(2, widths);

				if (hui::textInput(cdata.strSearch, cdata.strSearchMaxSize, TextInputValueMode::Any, "Type component name"))
				{
					printf("Text changed: %s\n", cdata.strSearch);
				}

				hui::nextColumn();

				if (hui::iconButton(deleteIcon))
				{
					strcpy(cdata.strSearch, "");
				}

				hui::endColumns();
				hui::popPadding();

				hui::beginScrollView(200, cdata.scrollPosNewComponentList);

				static int folderIndex = 0;

				char* compNames1[] = {
					"UiText",
					"Light",
					"Camera",
					"Mesh",
					"Material",
					"UiCanvas",
					"UiButton",
					"UiPanel",
					"SoundSource",
					"SoundEffect",
					"SoundCue",
					"FlowGraph",
					"Script",
					"Collider",

				};

				char* compNames2[] = {
					"SoundSource",
					"SoundEffect",
					"SoundCue",
				};

				char** compNames = (folderIndex == 0) ? compNames1 : compNames2;

				char* compFolders[] = {
					"Graphics",
					"Audio",
				};


				int numComp = (folderIndex == 0) ? sizeof(compNames1) / sizeof(compNames1[0]) : sizeof(compNames2) / sizeof(compNames2[0]);
				int numCompFolders = sizeof(compFolders) / sizeof(compFolders[0]);
				bool foundSome = false;

				hui::pushPadding(0);
				hui::pushSpacing(0);

				for (int i = 0; i < numCompFolders; i++)
				{
					if (stristr(compFolders[i], cdata.strSearch))
					{
						foundSome = true;
						hui::pushTint(Color::orange, TintColorType::Text);
						if (hui::selectableCustomFont(compFolders[i], fntBold))
						{
							hui::popTint();
							//closePopup();
							folderIndex = i;
							break;
						}
						hui::popTint();
					}
				}

				for (int i = 0; i < numComp; i++)
				{
					if (stristr(compNames[i], cdata.strSearch))
					{
						foundSome = true;

						if (hui::selectable(compNames[i]))
						{
							closePopup();
							cdata.showAddComponent = false;
							cdata.components.push_back(compNames[i]);
							cdata.componentPanelExpanded.push_back(false);
							cdata.componentPanelEnabled.push_back(true);
							break;
						}
					}
				}
				hui::popPadding();
				hui::popSpacing();

				if (!foundSome)
				{
					hui::pushPadding(15);
					hui::pushSpacing(5);
					hui::beginBox(hui::Color::darkRed);
					hui::image(sadIcon);
					hui::multilineLabelCustomFont((std::string("No component names found to contain '") + std::string(cdata.strSearch) + std::string("' text\nTry other names")).c_str(), fntItalic, HAlignType::Center);
					hui::endBox();
					hui::popPadding();
					hui::popSpacing();
				}

				cdata.scrollPosNewComponentList = hui::endScrollView();

				hui::beginTwoColumns();

				hui::textInput(cdata.strNewCompScript, 1000, TextInputValueMode::Any, "Type script component name");
				hui::nextColumn();

				if (hui::button("< Add Script"))
				{
					if (strcmp(cdata.strNewCompScript, ""))
					{
						cdata.components.push_back(cdata.strNewCompScript);
						cdata.componentPanelExpanded.push_back(false);
						cdata.componentPanelEnabled.push_back(true);
						strcpy(cdata.strNewCompScript, "");
					}

					hui::closePopup();
					cdata.showAddComponent = false;
				}

				hui::endColumns();
				if (hui::mustClosePopup())
				{
					hui::closePopup();
					cdata.showAddComponent = false;
				}

				hui::endPopup();
			}

			hui::pushPadding(2);
			hui::beginFourColumns();
			hui::button("Select");
			hui::nextColumn();
			hui::button("Break");
			hui::nextColumn();
			hui::button("Revert");
			hui::nextColumn();
			hui::button("Update");
			hui::endColumns();
			hui::popPadding();
			hui::label("Drag this to the None target");

			if (hui::wantsToDragDrop())
			{
				hui::beginDragDrop(0, (void*)"Dragged man!");
			}

			hui::label("Drag this other the None target");

			if (hui::wantsToDragDrop())
			{
				hui::beginDragDrop(0, (void*)"AHA!");
			}

			hui::pushPadding(2);
			hui::beginFourColumns();
			hui::button("Expand All");
			hui::nextColumn();
			hui::button("Collapse All");
			hui::nextColumn();
			hui::button("Enable All");
			hui::nextColumn();
			hui::button("Disable All");
			hui::endColumns();
			hui::popPadding();

			hui::beginScrollView(hui::getRemainingViewPaneHeight(viewPane), cdata.scrollPosComponents);

			srand(0);

			auto iter = cdata.components.begin();
			auto iter2 = cdata.componentPanelExpanded.begin();
			auto iter3 = cdata.componentPanelEnabled.begin();

			while (iter != cdata.components.end())
			{
				f32 widths[] = { 1, 50 };
				bool deleted = false;

				if (*iter3 == false)
					hui::pushTint(Color::gray);

				*iter2 = hui::panel(iter->c_str(), *iter2);

				if (*iter3 == false)
					hui::popTint();

				if (*iter2)
				{
					f32 colWidthsForProps[] = { 0.3, -1 };
					hui::beginColumns(2, colWidthsForProps);

					hui::label("Name");
					hui::label("Position");
					hui::label("Rotation");
					hui::label("Scale");
					hui::label("Visibility");

					hui::nextColumn();

					hui::pushPadding(0);
					hui::textInput(str, 100);

					static f32 x1 = 0, y1 = 0, z1 = 0;
					static f32 x2 = 0, y2 = 0, z2 = 0;
					static f32 x3 = 0, y3 = 0, z3 = 0;

					hui::vec3Editor(x1, y1, z1);
					hui::vec3Editor(x2, y2, z2);
					hui::vec3Editor(x3, y3, z3);
					hui::vec2Editor(x1, y1);

					static bool checker = false;
					checker = hui::check("", checker);

					static char objName[1000] = { 0 };
					static char* str;
					bool wasModified;

					if (hui::objectRefEditor(targetIcon, clearIcon, "Mesh", objName, 0, (void**)&str, &wasModified))
					{
						//show browser to choose item
					}

					if (wasModified && str)
					{
						strcpy(objName, str);
					}

					hui::popPadding();
					hui::endColumns();

					hui::beginColumns(2, widths);
					hui::pushTint(Color::yellow);
					hui::gap(30);
					hui::line();

					if (hui::button("Disable"))
					{
						*iter3 = false;
					}

					hui::popTint();

					hui::nextColumn();
					hui::pushTint(Color::red);
					hui::gap(30);
					hui::line();

					if (hui::button("Delete"))
					{
						iter = cdata.components.erase(iter);
						iter2 = cdata.componentPanelExpanded.erase(iter2);
						iter3 = cdata.componentPanelEnabled.erase(iter3);
						deleted = true;
					}

					hui::popTint();
					hui::endColumns();
				}

				if (!deleted)
				{
					++iter;
					++iter2;
					++iter3;
				}
			}

			cdata.scrollPosComponents = hui::endScrollView();

			break;
		}

		// custom viewport pane
		case 2:
		{
			hui::pushWidth(30);
			hui::pushSpacing(0);
			hui::beginSameLine();
			hui::iconButton(playIcon);
			//hui::iconButton(stopIcon);
			//hui::iconButton(pauseIcon);
			//hui::textInput(str, 100, TextInputValueMode::Any, "Type to filter scene");
			//hui::button("Search");
			hui::endSameLine();
			hui::popSpacing();
			hui::popWidth();

			// no margins for our viewport
			//hui::pushPadding(0);

			hui::pushPadding(10);
			auto viewRc = hui::beginViewport(500);
			auto ls = hui::LineStyle(hui::Color::blue, 1);
			hui::setLineStyle(ls);

			static Point o;

			o.x += 1;
			o.y += sinf(o.x*0.2f) * 20;

			//for (int i = 0; i < 100; i++)
			//{
			//	Point p1, p2, p3;

			//	p1.x = 2000.0f * (f32)rand() / RAND_MAX;
			//	p1.y = 1000.0f * (f32)rand() / RAND_MAX;
			//	p2.x = 2000.0f * (f32)rand() / RAND_MAX;
			//	p2.y = 1000.0f * (f32)rand() / RAND_MAX;
			//	p3.x = 2000.0f * (f32)rand() / RAND_MAX;
			//	p3.y = 1000.0f * (f32)rand() / RAND_MAX;

			//	ls = hui::LineStyle(hui::Color::random(), 10);
			//	hui::setLineStyle(ls);

			//	p1 += o;

			//	drawEllipse(p1, 20, 20, 4);
			//	drawCircle(p2, fabs(sinf(GetTickCount()/1000) * 10), 30);
			//	drawCircle(p3, 30, 10);

			//	WidgetElementInfo btnInf;

			//	getThemeWidgetElementInfo(WidgetElementId::ButtonBody, WidgetStateType::Normal, btnInf);
			//	drawBorderedImage(btnInf.image, btnInf.border, { -10, -10, 200, 200 });

			//	Point triPts[30];
			//	u32 ptCount = 0;

			//	ls = hui::LineStyle(hui::Color::random(), 1);
			//	hui::setLineStyle(ls);

			//	//hui::drawTriangle(p1, p2, p3);
			//}

			//printf("Custom viewport:%f %f %f %f\n", viewRc.x, viewRc.y, viewRc.width, viewRc.height);

			//ls = hui::LineStyle(hui::Color::yellow, 30);
			//hui::setLineStyle(ls);
			//hui::drawCircle({ 200, 200 }, 50, 300);
			static f32 ampl = 0;

			//Point pts[] = {{10, 240}, {100, 240}, {120, 240}, {330, 240}, {350, 240}, {380, 100}, {400, 120}};
			//{
			//	hui::LineStyle ls;

			//	ls.width = 54;
			//	ls.color = Color::yellow;
			//	hui::setLineStyle(ls);
			//	std::vector<Point> pts;
			//	float x = 2;
			//	for (float u = 0; u < 3.14*2; u += 0.1)
			//	{
			//		pts.push_back(Point(x+=5, 300 + sinf(u)*ampl));

			//	}
			//	hui::drawPolyLine(pts.data(), pts.size());
			//}

			//{
			//	hui::LineStyle ls;

			//	ls.width = 3;
			//	ls.color = Color::white;
			//	hui::setLineStyle(ls);
			//	std::vector<Point> pts;
			//	float x = 2;
			//	for (float u = 0; u < 3.14 * 2; u += 0.1)
			//	{
			//		pts.push_back(Point(x += 15, 370 + sinf(u) * 100));

			//	}
			//	hui::drawPolyLine(pts.data(), pts.size());
			//}

			{
				hui::LineStyle ls;

				ls.width = 10;
				ls.color = Color::green;
				
				hui::setLineStyle(ls);
				std::vector<Point> pts;
				float x = 2;
				for (float u = 0; u < 3.14 * 2; u += 2)
				{
					pts.push_back(Point(x += 15, 400 + sinf(u) * 100));

				}
				//hui::drawPolyLine(pts.data(), pts.size(), false);
			}
			hui::setLineStyle({ Color::red, 20 });
			drawCircle({ 140, 140 }, 110, 40);
			hui::setLineStyle({ Color::white, 1 });
			drawRectangle({ 50, 50, viewRc.width - 100, viewRc.height - 100 });
			static float ff = 0;
			static f32 tim = 0;

			LineStyle ls1;
			static f32 phs = 0;
			ls1.stipplePattern[0] = 34;
			ls1.stipplePattern[1] = 11;
			ls1.stipplePattern[2] = 5;
			ls1.stipplePattern[3] = 10;
			ls1.stipplePatternCount = 2;
			ls1.useStipple = true;
			ls1.width = 5;
			ls1.color = Color::yellow;
			ls1.stipplePhase = phs;



			LineStyle ls2;

			ls2.stipplePattern[0] = 4;
			ls2.stipplePattern[1] = 3;
			ls2.stipplePatternCount = 2;
			ls2.useStipple = false;
			ls2.width = 1;
			ls2.stipplePhase = phs;
			ls2.color = Color::red;

			LineStyle ls3;

			ls3.stipplePattern[0] = 2;
			ls3.stipplePattern[1] = 2;
			ls3.stipplePatternCount = 2;
			ls3.useStipple = true;
			ls3.width = 1;
			ls3.stipplePhase = phs;
			ls3.color = Color::lightGray;

			//phs += 1;
			hui::setLineStyle(ls2);

			auto mpos = getMousePosition();
			mpos.x -= viewRc.x + 20;
			mpos.y -= viewRc.y + 20;

			if (hui::getInputEvent().type != InputEvent::Type::MouseMove)
				mpos.clear();

			Point ps[] = { { 100, 150 },mpos, {170, 150}, {200, 350}, {300, 250}, {500, 230}, {600, 150} };
			//hui::setLineStyle({ Color::red, 25.5f });
			//drawPolyLine(ps, 7);
			//hui::setLineStyle({ Color::red, 1 });
			//drawPolyLine(ps, 7);


			ff = sinf(tim) * 100.0f;
			tim+=0.01f;
			Point ps2[] = { { 100, 100 },{ 250, 100 },{250, 250},{ 100, 250 } };
			//Point ps2[] = { { 100, 100 },{ 250, 100 },{ 350, 100 },{ 410, 100 } };
			//Point ps2[] = { { 100, 100 },{ 150, 150 },{ 200, 200 },{ 310, 310 } };
			
			setLineStyle(ls1);
			//drawPolyLine(ps2, 4, true);
			setLineStyle(ls3);
			setLineStyle(ls2);
			drawPolyLine(ps2, 4, false);
			setLineStyle(ls3);

			for (f32 m = 0; m < 1500; m += 20)
			{
				drawLine({ 0.0f, m }, { 1000.0f, m });
			}

			for (f32 m = 0; m < 1500; m += 20)
			{
				drawLine({ m, 0.0f }, { m, 1000.0f });
			}

			FillStyle fs;
			fs.color = Color::sky;
			setFillStyle(fs);
			drawSolidRectangle({ 100, 100, mpos.x - 100, mpos.y - 100 });

			{
				hui::LineStyle ls;

				ls.width = 8;
				ls.color = Color::orange;
				hui::setLineStyle(ls);
				std::vector<Point> pts;
				float x = 2;
				for (float u = 0; u < 3.14 * 2; u += 0.1)
				{
					pts.push_back(Point(x += 15, 440 + sinf(u) * 100));

				}
				hui::drawPolyLine(pts.data(), pts.size());
			}

			{
				hui::LineStyle ls;

				ls.width = 6;
				ls.color = Color::cyan;
				ls.stipplePattern[0] = 20;
				ls.stipplePattern[1] = 10;
				ls.stipplePattern[2] = 5;
				ls.stipplePattern[3] = 5;
				ls.stipplePattern[4] = 5;
				ls.stipplePattern[5] = 5;
				ls.stipplePatternCount = 2;
				ls.useStipple = true;
				hui::setLineStyle(ls);
				std::vector<SplineControlPoint> pts;

				pts.resize(4);

				pts[0].center.x = 100;
				pts[0].center.y = 100;
				pts[0].rightTangent.x = 30;
				pts[0].rightTangent.y = 20;

				pts[1].center.x = 200;
				pts[1].center.y = 100;
				pts[1].leftTangent.x = 0;
				pts[1].leftTangent.y = 4;
				pts[1].rightTangent.x = 0;
				pts[1].rightTangent.y = 90;

				pts[2].center.x = 400;
				pts[2].center.y = 100;
				pts[2].leftTangent.x = 320;
				pts[2].leftTangent.y = 1420+ampl;
				pts[1].rightTangent.x = 20;
				pts[1].rightTangent.y = 190;

				pts[3].center.x = 700;
				pts[3].center.y = 150;
				pts[3].leftTangent.x = 320;
				pts[3].leftTangent.y = 520;

				//hui::setLineStyle(ls2);
				hui::drawSpline(pts.data(), pts.size(), 40);

				//hui::setLineStyle({ Color::red, 2 });
				//hui::drawLine({ 50,50 }, { 100, 100 });
			}
			
			setFont(fntVeryBig);
			FillStyle fs2;
			fs2.color = Color::white;
			setFillStyle(fs2);
			drawTextAt("Custom Graphics abcdefghijklmnoprstuvxyz", { 40, 140 });

			fs2.color = Color::green;
			setFillStyle(fs2);
			setFont(fntBold);
			drawTextAt("TEST HERE SOME SMALLER TEXT", { 40, 340 });

			setFont(fntBold);
			setFillColor(Color::magenta);
			drawSolidTriangle({ 200, 200 }, { 300, 210 }, { 100, 400 });

			WidgetElementInfo ei;
			getThemeWidgetElementInfo(WidgetElementId::PopupBody, WidgetStateType::Normal, ei);
			setColor(Color::white);
			drawBorderedImage(ei.image, ei.border, { 310, 322, 300, 300 });

			drawImage(horusLogo, { 500, 300 }, 1);

			ampl += sinf(GetTickCount()) * 12;

			if (hui::isHovered())
			{
				hui::setMouseCursor(hui::MouseCursorType::CrossHair);
			}

			hui::endViewport();
			hui::popPadding();



			static f32 spos = 0;
			hui::beginScrollView(hui::getRemainingViewPaneHeight(viewPane), spos);

			hui::beginBox(Color::darkCyan);

			hui::multilineLabel("The graphical user interface (GUI /ɡuːiː/), is a type of user interface that allows users to interact with electronic devices through graphical icons and visual indicators such as secondary notation, instead of text-based user interfaces, typed command labels or text navigation. GUIs were introduced in reaction to the perceived steep learning curve of command-line interfaces (CLIs),[1][2][3] which require commands to be typed on a computer keyboard.胡户口", HAlignType::Left);

			srand(1);

			for (int k = 0; k < 10; k++)
			{
				hui::beginThreeColumns();
				hui::beginBox(Color::random());
				hui::label("ETA");
				hui::endBox();
				hui::nextColumn();
				hui::beginBox(Color::random());
				hui::label("ETA");
				hui::endBox();
				hui::nextColumn();
				hui::beginBox(Color::random());
				hui::label("ETA");
				hui::endBox();
				hui::endColumns();
			}
			hui::endBox();

			spos = hui::endScrollView();

			break;
		}
		case 3:
		{
			Rect wrect = hui::beginCustomWidget(hui::getRemainingViewPaneHeight(viewPane));

			{
				//hui::LineStyle ls;

				//ls.width = 6;
				//ls.color = Color::cyan();
				//hui::setLineStyle(ls);
				//std::vector<SplineControlPoint> pts;

				//pts.resize(4);

				//pts[0].center.x = 100;
				//pts[0].center.y = 100;
				//pts[0].rightTangent.x = 120;
				//pts[0].rightTangent.y = 20;

				//pts[1].center.x = 200;
				//pts[1].center.y = 100;
				//pts[1].leftTangent.x = 0;
				//pts[1].leftTangent.y = 140;
				//pts[1].rightTangent.x = 0;
				//pts[1].rightTangent.y = 90;

				//pts[2].center.x = wrect.width;
				//pts[2].center.y = 499;
				//pts[2].leftTangent.x = 320;
				//pts[2].leftTangent.y = 1420;
				//pts[1].rightTangent.x = 20;
				//pts[1].rightTangent.y = 190;

				//pts[3].center.x = 700;
				//pts[3].center.y = 150;
				//pts[3].leftTangent.x = 320;
				//pts[3].leftTangent.y = 520;

				//hui::drawSpline(pts.data(), pts.size());

				//hui::setLineStyle({ Color::red(), 2 });
				//hui::drawLine({ 50,40 }, { wrect.width, wrect.height });

				drawFlowNode("Action Node", { wrect.x + 30, wrect.y + 30, 200, 200 });
				drawFlowNode("Play Node", { wrect.x + 130, wrect.y + 150, 250, 200 });
			}

			if (hui::isHovered())
			{
				hui::setMouseCursor(hui::MouseCursorType::Hand);
			}

			hui::endCustomWidget();

			break;
		}
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		{
			hui::gap(5);
			hui::labelCustomFont("General Settings", hui::getFont("title"));
			static f32 scr[100] = { 0 };
			hui::image(horusLogo);
			const char* s[] = { "Red", "Green", "Blue", "Yellow", "Pink" };
			static i32 crtSel = 0;
			hui::dropdown(crtSel, s, 5, 3);
			if (isChangeEnded()) printf("Drop changed\n");
			hui::beginScrollView(hui::getRemainingViewPaneHeight(viewPane), scr[viewId]);
			hui::gap(5);

			// showing a message box
			static bool exiting = false;
			static u64 udid = 0;

			if (hui::button("Exit"))
			{
				exiting = true;
				udid = userDataId;
			}

			if (exiting && udid == userDataId)
			{
				hui::MessageBoxButtons mb = hui::messageBox("Exit?", "Do you want to exit ?", hui::MessageBoxButtons::YesNo, hui::MessageBoxIcon::Question);

				if (!!(mb & MessageBoxButtons::Yes))
				{
					hui::quitApplication();
				}
				else if (!!(mb & MessageBoxButtons::No))
				{
					exiting = false;
				}
			}

			//---

			static f32 prog = 0;
			hui::label("Processing the procedural Universe...");
			hui::progress(prog += 0.0001f);

			hui::beginEqualColumns(2);

			hui::pushTint(color, hui::TintColorType::Body);

			static bool chooseColorPopup = false;
			static u64 userDataIdForLena = 0;

			if (hui::button("Choose color..."))
			{
				auto rect = hui::getWidgetRect();
				chooseColorPopupPos = { rect.x, rect.bottom() };
				chooseColorPopup = true;
				userDataIdForLena = userDataId;
			}

			hui::popTint(hui::TintColorType::Body);

			if (chooseColorPopup && userDataId == userDataIdForLena)
			{
				hui::beginPopup(400);
				hui::pushTint(Color::yellow);
				hui::labelCustomFont("Meet Lena", fntBig);
				hui::popTint();

				hui::beginFourColumns();
				hui::image(lenaImg, 100);
				hui::nextColumn();
				hui::image(lenaImg, 100);
				hui::nextColumn();
				hui::image(lenaImg, 100);
				hui::nextColumn();
				hui::image(lenaImg, 100);
				hui::endColumns();

				hui::beginMenuBar();

				if (hui::beginMenu("File"))
				{
					hui::menuItem("New", "Ctrl+N");
					hui::menuItem("Save", "Ctrl+S");
					hui::menuItem("Save As", "Ctrl+A");
					hui::menuSeparator();
					hui::menuItem("Export", "Ctrl+E");
					hui::menuItem("Import", "Ctrl+I");
					hui::menuSeparator();
					hui::menuItem("Close", "Ctrl+X");
					hui::endMenu();
				}

				if (hui::beginMenu("Help"))
				{
					hui::menuItem("About", "Ctrl+Shift+A");
					hui::menuSeparator();
					hui::menuItem("Go to website", "Ctrl+W");
					hui::endMenu();
				}

				hui::endMenuBar();

				hui::multilineLabel("Lenna or Lena is the name given to a standard test image widely used in the field of image processing since 1973.[1] It is a picture of Lena Söderberg, shot by photographer Dwight Hooker, cropped from the centerfold of the November 1972 issue of Playboy magazine.", HAlignType::Left);

				hui::gap(20);

				static bool ap = false;
				ap = hui::check("Approve her", ap);

				if (hui::isClicked())
				{
					printf("Changed approved\n");
				}

				static int selTab = 0;

				hui::beginTabGroup(selTab);
				hui::tab("General", 0);
				hui::tab("Others", 0);
				hui::tab("More", 0);
				selTab = hui::endTabGroup();

				hui::gap(10);

				static int projType = 0;
				static const char* projTypes[] = { "Perspective", "Free Ortho", "Left", "Top", "Front" };
				static char someText[200] = "Please select projection type";
				static char colorR[10] = { 0 };
				static char colorG[10] = { 0 };
				static char colorB[10] = { 0 };
				static hui::Color c;

				if (selTab == 0)
				{
					//hui::line();
					hui::beginTwoColumns();
					hui::label("Projection type", HAlignType::Right);
					hui::nextColumn();

					if (hui::dropdown(projType, projTypes, 5))
					{
						strcpy((char*)someText, projTypes[projType]);
					}

					hui::endColumns();
				}
				else
				{
					hui::toString(c.r, colorR, 10);
					hui::toString(c.g, colorG, 10);
					hui::toString(c.b, colorB, 10);

					hui::beginTwoColumns();
					hui::label("Text.Red", HAlignType::Right);
					hui::nextColumn();
					hui::textInput(colorR, 10);
					hui::endColumns();

					hui::label("Red", HAlignType::Right);
					hui::sliderFloat(0, 1, c.r);


					hui::beginTwoColumns();
					hui::label("Text.Green", HAlignType::Right);
					hui::nextColumn();
					hui::textInput(colorG, 10);
					hui::endColumns();

					hui::label("Green", HAlignType::Right);
					hui::sliderFloat(0, 1, c.g);

					hui::beginTwoColumns();
					hui::label("Text.Blue", HAlignType::Right);
					hui::nextColumn();
					hui::textInput(colorB, 10);
					hui::endColumns();

					hui::label("Blue", HAlignType::Right);
					hui::sliderFloat(0, 1, c.b);

					hui::beginThreeColumns();
					hui::labelCustomFont(colorR, fntBold, HAlignType::Center);
					hui::nextColumn();
					hui::labelCustomFont(colorG, fntBold, HAlignType::Center);
					hui::nextColumn();
					hui::labelCustomFont(colorB, fntBold, HAlignType::Center);
					hui::endColumns();
				}

				hui::gap(20);
				hui::pushTint(c);
				hui::multilineLabelCustomFont(someText, fntBig, HAlignType::Center);
				hui::popTint();
				hui::textInput(someText, 200, TextInputValueMode::Any, "Input projection");
				hui::gap(20);
				if (hui::button("More Info..."))
				{
					moreInfoMsgBox = true;
				}

				if (hui::button("Close") || hui::mustClosePopup())
				{
					printf("Close\n");
					hui::closePopup();
					chooseColorPopup = false;
				}

				if (moreInfoMsgBox)
				{
					auto answer = hui::messageBox("Info", "More");

					if (!!(answer & MessageBoxButtons::Ok) || !!(answer & MessageBoxButtons::ClosedByEscape))
					{
						moreInfoMsgBox = false;
					}
				}

				hui::gap(20);
				hui::endPopup();
			}

			if (hui::button("Move"))
			{
			}

			hui::button("Rotate");
			hui::button("Scale");
			hui::nextColumn();
			if (hui::button("Subtract"))
			{
				printf("SUB\n");
			}
			const char* s2[] = { "White", "Red", "Green", "Blue", "Yellow", "Pink" };
			static int sel = 0;
			static hui::Color currentTint = Color::white;

			hui::pushTint(currentTint);

			if (hui::dropdown(sel, s2, 6, 3))
			{
				printf("Selected: %s\n", s2[sel]);

				switch (sel)
				{
				case 0: currentTint = Color::white; break;
				case 1: currentTint = Color::red; break;
				case 2: currentTint = Color::green; break;
				case 3: currentTint = Color::blue; break;
				case 4: currentTint = Color::yellow; break;
				case 5: currentTint = Color::magenta; break;
				default:
					break;
				}
			}

			hui::popTint();

			hui::button("Extrude");
			hui::button("Trim edges");
			hui::button("Generate All edges");
			static f32 slideVal = 0;
			static f32 slideVal55= 0;
			hui::endColumns();

			f32 colWidths[] = { 0.2f, 15, -1, 15, -1, 15, -1 };
			f32 colMinWidths[] = { 50, 0, 0, 0, 0, 0, 0 };
			hui::beginColumns(7, colWidths, colMinWidths);
			hui::label("Position"); hui::nextColumn();
			hui::label("X", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10); hui::nextColumn();
			hui::label("Y", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10); hui::nextColumn();
			hui::label("Z", HAlignType::Right); hui::nextColumn();	hui::textInput(str, 10);
			hui::endColumns();

			hui::beginColumns(7, colWidths, colMinWidths);
			hui::label("Rotation"); hui::nextColumn();
			hui::label("X", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10); hui::nextColumn();
			hui::label("Y", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10); hui::nextColumn();
			hui::label("Z", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10);
			hui::endColumns();

			hui::beginColumns(7, colWidths, colMinWidths);
			hui::label("Scale"); hui::nextColumn();
			hui::label("X", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10); hui::nextColumn();
			hui::label("Y", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10); hui::nextColumn();
			hui::label("Z", HAlignType::Right); hui::nextColumn(); hui::textInput(str, 10);
			hui::endColumns();

			hui::beginEqualColumns(3);
			hui::label("Far Plane:", HAlignType::Right);
			hui::nextColumn();
			hui::pushPadding(0);
			hui::getContextSettings().sliderDragDirection = hui::SliderDragDirection::Any;
			hui::getContextSettings().sliderInvertVerticalDragAmount = false;

			hui::comboSliderFloat(slideVal, 1);
			if (isChangeEnded()) printf("Combo slider changed\n");

			button("Inner");

			hui::comboSliderFloatRanged(slideVal55, 0, 100, .1f);
			hui::line();
			static f32 sval[3] = {0.5f, 1, 0};
			static char txtSlider[64] = "";
			hui::beginThreeColumns();
			hui::rotarySliderFloat("Volume",sval[0], -1, 1, .01, true);
			if (isChangeEnded()) printf("Rotary changed\n");
			hui::progress(sval[0]);
			toString(sval[0], txtSlider, 64);
			hui::beginBox(Color::black);
			hui::pushTint(Color::red);
			//hui::button("test");
			hui::multilineLabel(txtSlider, HAlignType::Center);
			hui::popTint();
			hui::endBox();
			hui::nextColumn();
			hui::rotarySliderFloat("Pitch", sval[1], 0, 1, .01);
			hui::progress(sval[1]);
			toString(sval[1], txtSlider, 64);
			hui::beginBox(Color(199/255.0f, 202 / 255.0f, 1 / 255.0f, 1));
			hui::pushTint(Color::black);
			hui::button("test");
			hui::labelCustomFont(txtSlider, fntLed1, HAlignType::Center);
			hui::popTint();
			hui::endBox();
			hui::nextColumn();

			hui::rotarySliderFloat("Echo", sval[2], 0, 1, .01);
			hui::progress(sval[2]);
			toString(sval[2], txtSlider, 64);
			hui::beginBox(Color::veryDarkGreen);
			hui::pushTint(Color::green);
			hui::button("test");
			hui::labelCustomFont(txtSlider, fntLed1, HAlignType::Center);
			hui::popTint();
			hui::endBox();
			hui::endColumns();
			hui::popPadding();
			hui::nextColumn();
			static char valStr[20];
			sprintf(valStr, "%.2f", slideVal);
			hui::textInput(valStr, 10);
			if (isChangeEnded()) printf("Text changed\n");

			slideVal = atof(valStr);
			hui::nextColumn();

			static f32 slideVal2 = 0;
			hui::beginEqualColumns(3);
			hui::label("X Scale:");
			hui::nextColumn();
			hui::pushPadding(0);
			hui::sliderFloat(0, 100, slideVal2);
			hui::popPadding();
			hui::nextColumn();
			static char valStr2[10];
			sprintf(valStr2, "%.2f", slideVal2);
			hui::textInput(valStr2, 10);
			slideVal2 = atof(valStr2);
			hui::nextColumn();

			static f32 slideVal3 = 0;
			hui::beginEqualColumns(3);
			hui::label("Y Scale:");
			hui::nextColumn();
			hui::pushPadding(0);
			hui::sliderFloat(0, 100, slideVal3);
			hui::popPadding();
			hui::nextColumn();
			static char valStr3[20];
			sprintf(valStr3, "%.2f", slideVal3);
			hui::textInput(valStr3, 10);
			slideVal3 = atof(valStr3);
			hui::nextColumn();

			hui::line();
			char some[33];
			sprintf(some, "%.2f", slideVal);
			hui::beginEqualColumns(2);
			hui::label("Value:");
			hui::nextColumn();
			hui::label(some);
			hui::nextColumn();

			hui::setWidgetStyle(WidgetType::Button, "important");
			hui::button("Important button");
			hui::setWidgetDefaultStyle(WidgetType::Button);

			static bool pnl1 = false;
			static bool pnl2 = false;
			static bool pnl3 = false;

			hui::label("UI Scale");
			if (hui::sliderFloat(0.1f, 2, scale, true, 0.2f))
			{
				changeScale = true;
			}

			if (hui::button("Reset UI scale"))
			{
				changeScale = true;
				scale = 1;
			}

			//hui::colorPickerPopup(color, color);

			pnl1 = hui::panel("Options1", pnl1);

			if (pnl1)
			{
				hui::textInput(str, 100, TextInputValueMode::Any);
				if (hui::beginCustomTooltip(200))
				{
					hui::pushTint(Color::darkGray, TintColorType::Text);
					hui::labelCustomFont("How to edit", fntBig);
					hui::image(lenaImg);
					hui::gap(10);
					hui::multilineLabel("The tooltip or infotip or a hint is a common graphical user interface element. It is used in conjunction with a cursor, usually a pointer. The user hovers the pointer over an item, without clicking it, and a tooltip may appear—a small \"hover box\" with information about the item being hovered over.[1][2] Tooltips do not usually appear on mobile operating systems, because there is no cursor (though tooltips may be displayed when using a mouse).", HAlignType::Left);
					hui::popTint();
					hui::line();
					hui::endCustomTooltip();
				}

				if (hui::getInputEvent().type == hui::InputEvent::Type::OsDragDrop)
				{
					if (hui::getInputEvent().drop.type == InputEvent::OsDragDropData::Type::DropFile)
					{
						strcpy(str, hui::getInputEvent().drop.filename);
						delete[] hui::getInputEvent().drop.filename;
						hui::cancelEvent();
					}
				}
				if (hui::getDragDropObjectType() == 0)
					hui::allowDragDrop();

				if (hui::droppedOnWidget() && hui::getDragDropObjectType() == 0)
				{
					strcpy(str, (char*)hui::getDragDropObject());
					hui::endDragDrop();
				}

				hui::beginEqualColumns(2);
				checks[0] = hui::check("Show markers", checks[0]);
				hui::tooltip("And some amazing tooltip\nMultiline of course!");
				hui::radio("Shokologo", true);
				checks[1] = hui::check("Show ships", checks[1]);
				hui::tooltip("SHIPS!");
				checks[2] = hui::check("Show bullets", checks[2]);
				hui::tooltip("BULLETS!");
				hui::nextColumn();
				checks[3] = hui::check("Always update", checks[3]);
				checks[4] = hui::check("Trim ribbon", checks[4]);
				hui::line();
				static int opt = 0;

				if (hui::radio("Use X axis", opt == 0))
					opt = 0;
				if (hui::radio("Use Y axis", opt == 1))
					opt = 1;
				if (hui::radio("Use Z axis", opt == 2))
					opt = 2;

				hui::nextColumn();
			}

			pnl2 = hui::panel("Options2", pnl2);

			if (pnl2)
			{
				hui::button("fasfdsd");
				hui::button("fasfdsd");
				hui::button("fasfdsd");
				hui::button("fasfdsd");
				hui::textInput(str, 100);
			}

			pnl3 = hui::panel("Options with scroll", pnl3);

			if (pnl3)
			{
				hui::textInput(str, 100);
				static f32 val = 0;
				hui::sliderFloat(0, 100, val);
				hui::button("Gogo1");
				hui::button("Gogo2");
				hui::button("Gogo3");
				hui::button("Gogo4");
				hui::button("Gogo5");
				hui::button("Gogo6");
				hui::button("Gogo7");
				hui::button("Gogo8");
				hui::button("Gogo last");
				hui::line();
			}

			scr[viewId] = hui::endScrollView();
			break;
		}
		}
	}

} myViewHandler;

void createMyDefaultViewPanes()
{
	auto myViewContainer = hui::createViewContainer(hui::getMainWindow());
	auto viewPane1 = hui::createViewPane(myViewContainer, hui::DockType::Left);
	hui::addViewPaneTab(viewPane1, "Assets", 0, 0);
	console1Tab = hui::addViewPaneTab(viewPane1, "Console1", 1, 0);
	console2Tab = hui::addViewPaneTab(viewPane1, "Console2", 1, 1);
	hui::addViewPaneTab(viewPane1, "Scene", 2, 0);
	auto viewPane2 = hui::createViewPane(myViewContainer, hui::DockType::Left);
	hui::addViewPaneTab(viewPane2, "Game", 3, 0);
	auto viewPane3 = hui::createViewPane(myViewContainer, hui::DockType::Left);
	hui::addViewPaneTab(viewPane3, "Particles", 4, 1);
	auto viewPane4 = hui::createViewPane(myViewContainer, hui::DockType::Bottom);
	hui::addViewPaneTab(viewPane4, "Properties", 5, 2);
	auto viewPane5 = hui::createViewPane(myViewContainer, hui::DockType::Right);
	hui::addViewPaneTab(viewPane5, "Object Inspector", 6, 3);
}

int main(int argc, char** args)
{
	hui::SdlSettings settings;

	settings.mainWindowTitle = "HorusUI Example - Widget Showroom";
	settings.mainWindowRect = { 0, 0, 800, 600 };
	settings.gfxProvider = new OpenGLGraphicsProvider();
	settings.antiAliasing = hui::AntiAliasing::None;
	hui::initializeWithSDL(settings);

	printf("Loading theme...\n");
	//TODO: load themes and images and anything from memory also
	auto theme = hui::loadTheme("../themes/default.theme");
	fntBig = hui::createFont(theme, "customBig", "../themes/fonts/arial.ttf", 20);
	fntVeryBig = hui::createFont(theme, "customVeryBig", "../themes/fonts/arial.ttf", 50);
	fntBold = hui::createFont(theme, "customBold", "../themes/fonts/arial.ttf", 15);
	fntItalic = hui::createFont(theme, "customItalic", "../themes/fonts/arial.ttf", 15);
	fntNodeTitle = hui::createFont(theme, "customNodeTitle", "../themes/fonts/Roboto-Bold.ttf", 12);
	printf("Loading images...\n");

	hui::setTheme(theme);
	drawLineImg = hui::loadImage("../themes/drawline.png");
	lenaImg = hui::loadImage("../themes/lena.png");
	nodeBodyImg = hui::loadImage("../themes/node-body.png");
	tabIcon1 = hui::loadImage("../themes/tabicon.png");
	tabIcon2 = hui::loadImage("../themes/tabicon2.png");
	deleteIcon = hui::loadImage("../themes/delete_icon.png");
	sadIcon = hui::loadImage("../themes/delete_icon.png");
	xaxisIcon = hui::loadImage("../themes/xaxis.png");
	yaxisIcon = hui::loadImage("../themes/yaxis.png");
	zaxisIcon = hui::loadImage("../themes/zaxis.png");
	targetIcon = hui::loadImage("../themes/target_icon.png");
	clearIcon = hui::loadImage("../themes/clear_icon.png");
	myViewHandler.moveIcon = hui::loadImage("../themes/move_icon.png");
	myViewHandler.playIcon = hui::loadImage("../themes/play-icon.png");
	myViewHandler.stopIcon = hui::loadImage("../themes/stop-icon.png");
	myViewHandler.pauseIcon = hui::loadImage("../themes/pause-icon.png");
	myViewHandler.horusLogo = hui::loadImage("../themes/horus.png");
	dragDropCur = hui::createMouseCursor("../themes/dragdrop_cursor.png");

	fntLed1 = hui::getFont("led1");
	fntLed2 = hui::getFont("led2");
	fntLed3 = hui::getFont("led3");

	hui::setDragDropMouseCursor(dragDropCur);
	printf("Loading views...\n");

	// if there is no state, then create the default panes and tabs
	if (!hui::loadViewContainersState("layout.hui"))
	{
		createMyDefaultViewPanes();
	}

	ViewContainer vcs[100];

	auto count = getViewContainers(vcs, 100);

	for (int i = 0; i < count; i++)
	{
		setViewContainerSideSpacing(vcs[i], 45, 150, 36);
	}

	hui::setViewIcon(1, tabIcon1);
	hui::setViewIcon(2, tabIcon2);
	hui::setViewIcon(3, tabIcon1);
	printf("Starting loop...\n");
	hui::dockingSystemLoop(&myViewHandler);
	hui::saveViewContainersState("layout.hui");
	hui::shutdown();

	return 0;
}
