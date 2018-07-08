#include "main.h"
#include "gmenu.h"
#include "download.h"
#include "history.h"
#include "settings.h"
#include "plan.h"

static void c_g_new_tab(GSimpleAction * a, GVariant * v, gpointer p)
{
	PlancWindow * win = (PlancWindow *) get_web_view();
	c_new_tab(NULL,win);
}

static void c_new_win(GSimpleAction * a, GVariant * v, gpointer p)
{
	InitWindow(G_APPLICATION(G_APP), NULL, 0);
}

static void c_new_downloads(GSimpleAction * a, GVariant * v, gpointer p)
{
	InitDownloadWindow();
}

static void c_new_history(GSimpleAction * a, GVariant * v, gpointer p)
{
	InitHistoryWindow(NULL);
}

static void c_new_settings(GSimpleAction * a, GVariant * v, gpointer p)
{
	InitSettingsWindow();
}

static void c_close_win(GSimpleAction * a, GVariant * v, gpointer p)
{
	PlancWindow * w 
		= (PlancWindow *) gtk_application_get_active_window(G_APP);
	if(w)
		c_destroy_window_menu(NULL,w);
}

void InitAppMenu()
{
	static const GActionEntry actions[] =
	{
		{ "newtab", c_g_new_tab },
		{ "newwin", c_new_win },
		{ "download", c_new_downloads },
		{ "history", c_new_history },
		{ "settings", c_new_settings },
		{ "close", c_close_win }
	};

	GMenu * menu, * s1, * s2, * s3;
	menu = g_menu_new();

	g_action_map_add_action_entries (G_ACTION_MAP (G_APP), actions
		,G_N_ELEMENTS (actions), G_APP);

	s1 = g_menu_new();
	g_menu_append(s1, "New Tab", "app.newtab");
	g_menu_append(s1, "New Window", "app.newwin");
	g_menu_append_section(menu, NULL, (GMenuModel *) s1);
	s2 = g_menu_new();
	g_menu_append(s2, "Downloads", "app.download");
	g_menu_append(s2, "History", "app.history");
	g_menu_append(s2, "Settings", "app.settings");
	g_menu_append_section(menu, NULL, (GMenuModel *) s2);
	s3 = g_menu_new();
	g_menu_append(s3, "Close Window", "app.close");
	g_menu_append_section(menu, NULL, (GMenuModel *) s3);

	gtk_application_set_app_menu (G_APP, G_MENU_MODEL (menu));
	g_object_unref(menu);
}
