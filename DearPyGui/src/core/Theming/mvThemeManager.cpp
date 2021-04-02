#include "mvThemeManager.h"
#include "imgui.h"
#include "mvAppItems.h"
#include "mvCore.h"
#include "mvItemRegistry.h"

namespace Marvel {

	std::vector<std::tuple<std::string, long, mvColor*, mvColor*>>      mvThemeManager::s_acolors;
	std::vector<std::tuple<std::string, long, float*, float>>			mvThemeManager::s_astyles;
	std::unordered_map<mvAppItemType, mvThemeColors>					mvThemeManager::s_colors;
	std::unordered_map<mvAppItemType, mvThemeStyles>					mvThemeManager::s_styles;

	void mvThemeManager::InsertParser(std::map<std::string, mvPythonParser>* parsers)
	{

		parsers->insert({ "set_theme_color", mvPythonParser({
			{mvPythonDataType::Integer, "constant", "mvThemeCol_* constants"},
			{mvPythonDataType::FloatList, "color"},
			{mvPythonDataType::Optional},
			{mvPythonDataType::String, "item", "", "''"}
		}, "Sets a color of a theme item for when the item is enabled.", "None", "Themes and Styles") });

		parsers->insert({ "set_theme_color_disabled", mvPythonParser({
			{mvPythonDataType::Integer, "constant", "mvThemeCol_* constants"},
			{mvPythonDataType::FloatList, "color"},
			{mvPythonDataType::Optional},
			{mvPythonDataType::String, "item", "", "''"}
		}, "Sets a color of a theme item for when the item is disabled.", "None", "Themes and Styles") });

		parsers->insert({ "set_theme_style", mvPythonParser({
			{mvPythonDataType::Integer, "constant", "mvThemeStyle_* constants"},
			{mvPythonDataType::Float, "style"},
			{mvPythonDataType::Optional},
			{mvPythonDataType::String, "item", "", "''"}
		}, "Sets a style of a theme item.", "None", "Themes and Styles") });

	}

	void mvThemeManager::InValidateColorTheme()
	{
		auto& frontWindows = mvApp::GetApp()->getItemRegistry().getFrontWindows();
		auto& backWindows = mvApp::GetApp()->getItemRegistry().getBackWindows();

		for (auto& window : frontWindows)
			window->inValidateThemeColorCache();

		for (auto& window : backWindows)
			window->inValidateThemeColorCache();
	}

	void mvThemeManager::InValidateStyleTheme()
	{
		auto& frontWindows = mvApp::GetApp()->getItemRegistry().getFrontWindows();
		auto& backWindows = mvApp::GetApp()->getItemRegistry().getBackWindows();

		for (auto& window : frontWindows)
			window->inValidateThemeStyleCache();

		for (auto& window : backWindows)
			window->inValidateThemeStyleCache();
	}

	mvThemeManager::mvThemeManager()
	{
		mvEventBus::Subscribe(this, SID("color_change"), mvEVT_CATEGORY_THEMES);
		mvEventBus::Subscribe(this, SID("style_change"), mvEVT_CATEGORY_THEMES);
	}

	mvThemeManager::~mvThemeManager()
	{
		mvEventBus::UnSubscribe(this);
	}
 
	bool mvThemeManager::onEvent(mvEvent& event)
	{
		mvEventDispatcher dispatcher(event);
		dispatcher.dispatch(BIND_EVENT_METH(mvThemeManager::add_style), SID("style_change"));
		dispatcher.dispatch(BIND_EVENT_METH(mvThemeManager::add_color), SID("color_change"));
		return event.handled;
	};

	bool mvThemeManager::add_color(mvEvent& event)
	{
		static mvAppItemType type;
		long mvThemeConstant = GetELong(event, "ID");
		DecodeType(mvThemeConstant, &type);
		mvColor color = GetEColor(event, "COLOR");
		const std::string& widget = GetEString(event, "WIDGET");
		bool enabled = GetEBool(event, "ENABLED");

		//fills out the app's root theme if no item was given
		if (widget.empty())
		{
			if (enabled) GetColors()[type][mvThemeConstant].first = color;
			else GetColors()[type][mvThemeConstant].second = color;
			InValidateColorTheme();
			return true;
		}

		//check widget can take color and apply
		mvRef<mvAppItem> item = mvApp::GetApp()->getItemRegistry().getItem(widget);
		if (item)
		{
			if (mvAppItem::DoesItemHaveFlag(item.get(), MV_ITEM_DESC_CONTAINER) || item->getType() == type)
			{
				if (enabled) item->getColors()[type][mvThemeConstant].first = color;
				else item->getColors()[type][mvThemeConstant].second = color;

				item->inValidateThemeColorCache();
			}
			else
			{
				mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
					{
						ThrowPythonException("Item type does not except this color constant.");
					});
			}
		}
		else
		{
			mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
				{
					ThrowPythonException("Item can not be found");
				});
		}
		return true;
	}

	bool mvThemeManager::add_style(mvEvent& event)
	{
		static mvAppItemType type;
		long mvThemeConstant = GetELong(event, "ID");
		DecodeType(mvThemeConstant, &type);
		float style = GetEFloat(event, "STYLE");
		const std::string& widget = GetEString(event, "WIDGET");

		//fills out the app's root theme if no item was given
		if (widget.empty())
		{
			GetStyles()[type][mvThemeConstant] = style;
			InValidateStyleTheme();
			return true;
		}

		//check widget can take style and apply
		mvRef<mvAppItem> item = mvApp::GetApp()->getItemRegistry().getItem(widget);
		if (item)
		{
			if (mvAppItem::DoesItemHaveFlag(item.get(), MV_ITEM_DESC_CONTAINER) || item->getType() == type)
			{
				item->getStyles()[type][mvThemeConstant] = style;
				item->inValidateThemeStyleCache();
			}
			else
			{
				mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
					{
						ThrowPythonException("Item type does not except this style constant.");
					});
			}
		}
		else
		{
			mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
				{
					ThrowPythonException("Item can not be found");
				});
		}
		return true;
	}

	PyObject* mvThemeManager::set_theme_color(PyObject* self, PyObject* args, PyObject* kwargs)
	{
		long constant;
		PyObject* color;
		const char* item = "";

		if (!(mvApp::GetApp()->getParsers())["set_theme_color"].parse(args, kwargs, __FUNCTION__, &constant, &color, &item))
			return GetPyNone();

		Py_XINCREF(color);
		std::lock_guard<std::mutex> lk(mvApp::GetApp()->getMutex());

		mvEventBus::Publish
		(
			mvEVT_CATEGORY_THEMES,
			SID("color_change"),
			{
				CreateEventArgument("WIDGET", std::string(item)),
				CreateEventArgument("ID", constant),
				CreateEventArgument("COLOR", ToColor(color)),
				CreateEventArgument("ENABLED", true)
			}
		);

		Py_XDECREF(color);


		return GetPyNone();
	}

	PyObject* mvThemeManager::set_theme_color_disabled(PyObject* self, PyObject* args, PyObject* kwargs)
	{
		long constant;
		PyObject* color;
		const char* item = "";

		if (!(mvApp::GetApp()->getParsers())["set_theme_color_disabled"].parse(args, kwargs, __FUNCTION__, &constant, &color, &item))
			return GetPyNone();

		Py_XINCREF(color);
		std::lock_guard<std::mutex> lk(mvApp::GetApp()->getMutex());
		mvApp::GetApp()->getCallbackRegistry().submit([=]()
			{
				mvEventBus::Publish
				(
					mvEVT_CATEGORY_THEMES,
					SID("color_change"),
					{
						CreateEventArgument("WIDGET", std::string(item)),
						CreateEventArgument("ID", constant),
						CreateEventArgument("COLOR", ToColor(color)),
						CreateEventArgument("ENABLED", false)
					}
				);

				// to ensure the decrement happens on the python thread
				mvApp::GetApp()->getCallbackRegistry().submitCallback([=]()
					{
						Py_XDECREF(color);
					});

			});

		return GetPyNone();
	}

	PyObject* mvThemeManager::set_theme_style(PyObject* self, PyObject* args, PyObject* kwargs)
	{
		long constant;
		float style;
		const char* item = "";

		if (!(mvApp::GetApp()->getParsers())["set_theme_style"].parse(args, kwargs, __FUNCTION__, &constant, &style, &item))
			return GetPyNone();

		std::lock_guard<std::mutex> lk(mvApp::GetApp()->getMutex());
		mvApp::GetApp()->getCallbackRegistry().submit([=]()
			{
				mvEventBus::Publish
				(
					mvEVT_CATEGORY_THEMES,
					SID("style_change"),
					{
						CreateEventArgument("WIDGET", std::string(item)),
						CreateEventArgument("ID", constant),
						CreateEventArgument("STYLE", style)
					}
				);
			});

		return GetPyNone();
	}
}