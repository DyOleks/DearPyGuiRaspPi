#include "mvBasicWidgets.h"
#include "AppItems/mvFontItems.h"
#include "AppItems/mvThemes.h"
#include "AppItems/containers/mvDragPayload.h"
#include "mvPyObject.h"
#include "AppItems/mvItemHandlers.h"
#include "mvPythonExceptions.h"

//-----------------------------------------------------------------------------
// [SECTION] get_item_configuration(...) specifics
//-----------------------------------------------------------------------------

void
DearPyGui::fill_configuration_dict(const mvButtonConfig& inConfig, PyObject* outDict)
{
	if (outDict == nullptr)
		return;

	mvPyObject py_small = ToPyBool(inConfig.small_button);
	mvPyObject py_arrow = ToPyBool(inConfig.arrow);
	mvPyObject py_direction = ToPyInt(inConfig.direction);

	PyDict_SetItemString(outDict, "small", py_small);
	PyDict_SetItemString(outDict, "arrow", py_arrow);
	PyDict_SetItemString(outDict, "direction", py_direction);
}

void
DearPyGui::fill_configuration_dict(const mvComboConfig& inConfig, PyObject* outDict)
{
	if (outDict == nullptr)
		return;

	// helper to check and set bit
	auto checkbitset = [outDict](const char* keyword, int flag, const int& flags)
	{
		mvPyObject py_value = ToPyBool(flags & flag);
		PyDict_SetItemString(outDict, keyword, py_value);
	};
	checkbitset("popup_align_left", ImGuiComboFlags_PopupAlignLeft, inConfig.flags);
	checkbitset("no_arrow_button", ImGuiComboFlags_NoArrowButton, inConfig.flags);
	checkbitset("no_preview", ImGuiComboFlags_NoPreview, inConfig.flags);

	mvUUID mode = (long)mvComboHeightMode::mvComboHeight_Largest;
	if (inConfig.flags & ImGuiComboFlags_HeightSmall) mode = (long)mvComboHeightMode::mvComboHeight_Small;
	else if (inConfig.flags & ImGuiComboFlags_HeightRegular) mode = (long)mvComboHeightMode::mvComboHeight_Regular;
	else if (inConfig.flags & ImGuiComboFlags_HeightLarge) mode = (long)mvComboHeightMode::mvComboHeight_Large;

	mvPyObject py_height_mode = ToPyLong(mode);
	mvPyObject py_items = ToPyList(inConfig.items);
	PyDict_SetItemString(outDict, "height_mode", py_height_mode);
	PyDict_SetItemString(outDict, "items", py_items);
}

//-----------------------------------------------------------------------------
// [SECTION] configure_item(...) specifics
//-----------------------------------------------------------------------------

void
DearPyGui::set_configuration(PyObject* inDict, mvButtonConfig& outConfig)
{
	if (inDict == nullptr)
		return;

	if (PyObject* item = PyDict_GetItemString(inDict, "small")) outConfig.small_button = ToBool(item);
	if (PyObject* item = PyDict_GetItemString(inDict, "arrow")) outConfig.arrow = ToBool(item);
	if (PyObject* item = PyDict_GetItemString(inDict, "direction")) outConfig.direction = ToInt(item);
}

void
DearPyGui::set_configuration(PyObject* inDict, mvComboConfig& outConfig)
{
	if (inDict == nullptr)
		return;

	if (PyObject* item = PyDict_GetItemString(inDict, "items")) outConfig.items = ToStringVect(item);

	if (PyObject* item = PyDict_GetItemString(inDict, "height_mode"))
	{
		long height_mode = (long)ToUUID(item);
		if (height_mode == (long)mvComboHeightMode::mvComboHeight_Small) outConfig.flags = ImGuiComboFlags_HeightSmall;
		else if (height_mode == (long)mvComboHeightMode::mvComboHeight_Regular) outConfig.flags = ImGuiComboFlags_HeightRegular;
		else if (height_mode == (long)mvComboHeightMode::mvComboHeight_Large) outConfig.flags = ImGuiComboFlags_HeightLarge;
		else outConfig.flags = ImGuiComboFlags_HeightLargest;
	}

	// helpers for bit flipping
	auto flagop = [inDict](const char* keyword, int flag, int& flags)
	{ if (PyObject* item = PyDict_GetItemString(inDict, keyword)) ToBool(item) ? flags |= flag : flags &= ~flag;};

	flagop("popup_align_left", ImGuiComboFlags_PopupAlignLeft, outConfig.flags);
	flagop("no_arrow_button", ImGuiComboFlags_NoArrowButton, outConfig.flags);
	flagop("no_preview", ImGuiComboFlags_NoPreview, outConfig.flags);
}

//-----------------------------------------------------------------------------
// [SECTION] positional args specifics
//-----------------------------------------------------------------------------
void
DearPyGui::set_positional_configuration(PyObject* inDict, mvComboConfig& outConfig)
{
	if (!VerifyPositionalArguments(GetParsers()[GetEntityCommand(mvAppItemType::mvCombo)], inDict))
		return;

	for (int i = 0; i < PyTuple_Size(inDict); i++)
	{
		PyObject* item = PyTuple_GetItem(inDict, i);
		switch (i)
		{
		case 0:
			outConfig.items = ToStringVect(item);
			break;

		default:
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// [SECTION] data sources
//-----------------------------------------------------------------------------
void
DearPyGui::set_data_source(mvAppItem& item, mvUUID dataSource, mvComboConfig& outConfig)
{
	if (dataSource == item.config.source) return;
	item.config.source = dataSource;

	mvAppItem* srcItem = GetItem((*GContext->itemRegistry), dataSource);
	if (!srcItem)
	{
		mvThrowPythonError(mvErrorCode::mvSourceNotFound, "set_value",
			"Source item not found: " + std::to_string(dataSource), &item);
		return;
	}
	if (DearPyGui::GetEntityValueType(srcItem->type) != DearPyGui::GetEntityValueType(item.type))
	{
		mvThrowPythonError(mvErrorCode::mvSourceNotCompatible, "set_value",
			"Values types do not match: " + std::to_string(dataSource), &item);
		return;
	}
	outConfig.value = *static_cast<std::shared_ptr<std::string>*>(srcItem->getValue());
}

void
DearPyGui::set_data_source(mvAppItem& item, mvUUID dataSource, mvCheckboxConfig& outConfig)
{
	if (dataSource == item.config.source) return;
	item.config.source = dataSource;

	mvAppItem* srcItem = GetItem((*GContext->itemRegistry), dataSource);
	if (!srcItem)
	{
		mvThrowPythonError(mvErrorCode::mvSourceNotFound, "set_value",
			"Source item not found: " + std::to_string(dataSource), &item);
		return;
	}
	if (DearPyGui::GetEntityValueType(srcItem->type) != DearPyGui::GetEntityValueType(item.type))
	{
		mvThrowPythonError(mvErrorCode::mvSourceNotCompatible, "set_value",
			"Values types do not match: " + std::to_string(dataSource), &item);
		return;
	}
	outConfig.value = *static_cast<std::shared_ptr<bool>*>(srcItem->getValue());
}

//-----------------------------------------------------------------------------
// [SECTION] template specifics
//-----------------------------------------------------------------------------
void
DearPyGui::apply_template(const mvButtonConfig& sourceConfig, mvButtonConfig& dstConfig)
{
	dstConfig.direction = sourceConfig.direction;
	dstConfig.small_button = sourceConfig.small_button;
	dstConfig.arrow = sourceConfig.arrow;
}

void
DearPyGui::apply_template(const mvComboConfig& sourceConfig, mvComboConfig& dstConfig)
{
	dstConfig.value = sourceConfig.value;
	dstConfig.disabled_value = sourceConfig.disabled_value;
	dstConfig.flags = sourceConfig.flags;
	dstConfig.items = sourceConfig.items;
	dstConfig.popup_align_left = sourceConfig.popup_align_left;
	dstConfig.no_preview = sourceConfig.no_preview;
}

void
DearPyGui::apply_template(const mvCheckboxConfig& sourceConfig, mvCheckboxConfig& dstConfig)
{
	dstConfig.value = sourceConfig.value;
	dstConfig.disabled_value = sourceConfig.disabled_value;
}

//-----------------------------------------------------------------------------
// [SECTION] draw commands
//-----------------------------------------------------------------------------
void
DearPyGui::draw_button(ImDrawList* drawlist, mvAppItem& item, const mvButtonConfig& config)
{
	//-----------------------------------------------------------------------------
	// pre draw
	//-----------------------------------------------------------------------------

	// show/hide
	if (!item.config.show)
		return;

	// focusing
	if (item.info.focusNextFrame)
	{
		ImGui::SetKeyboardFocusHere();
		item.info.focusNextFrame = false;
	}

	// cache old cursor position
	ImVec2 previousCursorPos = ImGui::GetCursorPos();

	// set cursor position if user set
	if (item.info.dirtyPos)
		ImGui::SetCursorPos(item.state.pos);

	// update widget's position state
	item.state.pos = { ImGui::GetCursorPosX(), ImGui::GetCursorPosY() };

	// set item width
	if (item.config.width != 0)
		ImGui::SetNextItemWidth((float)item.config.width);

	// set indent
	if (item.config.indent > 0.0f)
		ImGui::Indent(item.config.indent);

	// push font if a font object is attached
	if (item.font)
	{
		ImFont* fontptr = static_cast<mvFont*>(item.font.get())->getFontPtr();
		ImGui::PushFont(fontptr);
	}

	// themes
	apply_local_theming(&item);

	//-----------------------------------------------------------------------------
	// draw
	//-----------------------------------------------------------------------------
	{
		// push imgui id to prevent name collisions
		ScopedID id(item.uuid);

		bool activated = false;

		if (config.small_button)
			activated = ImGui::SmallButton(item.info.internalLabel.c_str());

		else if (config.arrow)
			activated = ImGui::ArrowButton(item.info.internalLabel.c_str(), config.direction);

		else
			activated = ImGui::Button(item.info.internalLabel.c_str(), ImVec2((float)item.config.width, (float)item.config.height));

		if (activated)
		{
			if (item.config.alias.empty())
				mvAddCallback(item.getCallback(false), item.uuid, nullptr, item.config.user_data);
			else
				mvAddCallback(item.getCallback(false), item.config.alias, nullptr, item.config.user_data);
		}
	}

	//-----------------------------------------------------------------------------
	// update state
	//-----------------------------------------------------------------------------
	UpdateAppItemState(item.state);

	//-----------------------------------------------------------------------------
	// post draw
	//-----------------------------------------------------------------------------

	// set cursor position to cached position
	if (item.info.dirtyPos)
		ImGui::SetCursorPos(previousCursorPos);

	if (item.config.indent > 0.0f)
		ImGui::Unindent(item.config.indent);

	// pop font off stack
	if (item.font)
		ImGui::PopFont();

	// handle popping themes
	cleanup_local_theming(&item);

	if (item.handlerRegistry)
		item.handlerRegistry->checkEvents(&item.state);

	// handle drag & drop if used
	apply_drag_drop(&item);
}

void
DearPyGui::draw_combo(ImDrawList* drawlist, mvAppItem& item, mvComboConfig& config)
{
	//-----------------------------------------------------------------------------
	// pre draw
	//-----------------------------------------------------------------------------

	// show/hide
	if (!item.config.show)
		return;

	// focusing
	if (item.info.focusNextFrame)
	{
		ImGui::SetKeyboardFocusHere();
		item.info.focusNextFrame = false;
	}

	// cache old cursor position
	ImVec2 previousCursorPos = ImGui::GetCursorPos();

	// set cursor position if user set
	if (item.info.dirtyPos)
		ImGui::SetCursorPos(item.state.pos);

	// update widget's position state
	item.state.pos = { ImGui::GetCursorPosX(), ImGui::GetCursorPosY() };

	// set item width
	if (item.config.width != 0)
		ImGui::SetNextItemWidth((float)item.config.width);

	// set indent
	if (item.config.indent > 0.0f)
		ImGui::Indent(item.config.indent);

	// push font if a font object is attached
	if (item.font)
	{
		ImFont* fontptr = static_cast<mvFont*>(item.font.get())->getFontPtr();
		ImGui::PushFont(fontptr);
	}

	// themes
	apply_local_theming(&item);

	//-----------------------------------------------------------------------------
	// draw
	//-----------------------------------------------------------------------------
	{
		ScopedID id(item.uuid);

		static std::vector<std::string> disabled_items{};

		// The second parameter is the label previewed before opening the combo.
		bool activated = ImGui::BeginCombo(item.info.internalLabel.c_str(), config.value->c_str(), config.flags);
		UpdateAppItemState(item.state);

		if (activated)
		{

			for (const auto& name : item.config.enabled ? config.items : disabled_items)
			{
				bool is_selected = (*config.value == name);
				if (ImGui::Selectable((name).c_str(), is_selected))
				{
					if (item.config.enabled) { *config.value = name; }

					auto value = *config.value;

					if (item.config.alias.empty())
						mvSubmitCallback([&]() {
						mvAddCallback(item.getCallback(false), item.uuid, ToPyString(value), item.config.user_data);
							});
					else
						mvSubmitCallback([&]() {
						mvAddCallback(item.getCallback(false), item.config.alias, ToPyString(value), item.config.user_data);
							});


				}

				item.state.edited = ImGui::IsItemEdited();
				item.state.deactivated = ImGui::IsItemDeactivated();
				item.state.deactivatedAfterEdit = ImGui::IsItemDeactivatedAfterEdit();

				// Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}

	//-----------------------------------------------------------------------------
	// post draw
	//-----------------------------------------------------------------------------

	// set cursor position to cached position
	if (item.info.dirtyPos)
		ImGui::SetCursorPos(previousCursorPos);

	if (item.config.indent > 0.0f)
		ImGui::Unindent(item.config.indent);

	// pop font off stack
	if (item.font)
		ImGui::PopFont();

	// handle popping themes
	cleanup_local_theming(&item);

	if (item.handlerRegistry)
		item.handlerRegistry->checkEvents(&item.state);

	// handle drag & drop if used
	apply_drag_drop(&item);
}

void
DearPyGui::draw_checkbox(ImDrawList* drawlist, mvAppItem& item, mvCheckboxConfig& config)
{
	//-----------------------------------------------------------------------------
	// pre draw
	//-----------------------------------------------------------------------------

	// show/hide
	if (!item.config.show)
		return;

	// focusing
	if (item.info.focusNextFrame)
	{
		ImGui::SetKeyboardFocusHere();
		item.info.focusNextFrame = false;
	}

	// cache old cursor position
	ImVec2 previousCursorPos = ImGui::GetCursorPos();

	// set cursor position if user set
	if (item.info.dirtyPos)
		ImGui::SetCursorPos(item.state.pos);

	// update widget's position state
	item.state.pos = { ImGui::GetCursorPosX(), ImGui::GetCursorPosY() };

	// set item width
	if (item.config.width != 0)
		ImGui::SetNextItemWidth((float)item.config.width);

	// set indent
	if (item.config.indent > 0.0f)
		ImGui::Indent(item.config.indent);

	// push font if a font object is attached
	if (item.font)
	{
		ImFont* fontptr = static_cast<mvFont*>(item.font.get())->getFontPtr();
		ImGui::PushFont(fontptr);
	}

	apply_local_theming(&item);

	//-----------------------------------------------------------------------------
	// draw
	//-----------------------------------------------------------------------------
	{
		// push imgui id to prevent name collisions
		ScopedID id(item.uuid);

		if (!item.config.enabled) config.disabled_value = *config.value;

		if (ImGui::Checkbox(item.info.internalLabel.c_str(), item.config.enabled ? config.value.get() : &config.disabled_value))
		{
			bool value = *config.value;

			if (item.config.alias.empty())
				mvSubmitCallback([&]() {
				mvAddCallback(item.getCallback(false), item.uuid, ToPyBool(value), item.config.user_data);
					});
			else
				mvSubmitCallback([&]() {
				mvAddCallback(item.getCallback(false), item.config.alias, ToPyBool(value), item.config.user_data);
					});
		}
	}

	//-----------------------------------------------------------------------------
	// update state
	//-----------------------------------------------------------------------------
	UpdateAppItemState(item.state);

	//-----------------------------------------------------------------------------
	// post draw
	//-----------------------------------------------------------------------------

	// set cursor position to cached position
	if (item.info.dirtyPos)
		ImGui::SetCursorPos(previousCursorPos);

	if (item.config.indent > 0.0f)
		ImGui::Unindent(item.config.indent);

	// pop font off stack
	if (item.font)
		ImGui::PopFont();

	cleanup_local_theming(&item);

	if (item.handlerRegistry)
		item.handlerRegistry->checkEvents(&item.state);

	// handle drag & drop if used
	apply_drag_drop(&item);
}

void
DearPyGui::draw_separator(ImDrawList* drawlist, mvAppItem& item)
{ 
	ImGui::Separator(); 
}

void
DearPyGui::draw_spacer(ImDrawList* drawlist, mvAppItem& item)
{ 
	if (item.config.width == 0 && item.config.height == 0) 
		ImGui::Spacing(); 
	else 
		ImGui::Dummy({ (float)item.config.width, (float)item.config.height }); 
}

void
DearPyGui::draw_menubar(ImDrawList* drawlist, mvAppItem& item)
{
	if (ImGui::BeginMenuBar())
	{

		item.state.lastFrameUpdate = GContext->frame;
		item.state.visible = true;

		for (auto& item : item.childslots[1])
			item->draw(drawlist, ImGui::GetCursorPosX(), ImGui::GetCursorPosY());

		ImGui::EndMenuBar();
	}
}

void
DearPyGui::draw_clipper(ImDrawList* drawlist, mvAppItem& item)
{
	ScopedID id(item.uuid);

	if (item.config.width != 0)
		ImGui::PushItemWidth((float)item.config.width);


	ImGuiListClipper clipper;
	clipper.Begin((int)item.childslots[1].size());

	while (clipper.Step())
	{
		for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
			item.childslots[1][row_n]->draw(drawlist, ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
	}
	clipper.End();
	if (item.config.width != 0)
		ImGui::PopItemWidth();
}