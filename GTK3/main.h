#include "config.h"
#include <stdlib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>

#define WK_CURRENT_TAB(notebook) \
(WebKitWebView *) gtk_notebook_get_nth_page(notebook \
    ,gtk_notebook_get_current_page(notebook))

#define WK_CURRENT_TAB_WIDGET(notebook) \
(GtkWidget *) gtk_notebook_get_nth_page(notebook \
    ,gtk_notebook_get_current_page(notebook))

#define WK_TAB_CHAR_LEN 40

extern GtkApplication * G_APP;
extern GSettings * G_SETTINGS;

struct menu_st
{
    GtkWidget * menu;
    GtkWidget * fileMenu;
    GtkWidget * editMenu;
    GtkWidget * viewMenu;
    GtkWidget * tabsMenu;
    GtkWidget * viewTabMenu;
    GtkWidget * helpMenu;
    GtkWidget * fileMh;
    GtkWidget * editMh;
    GtkWidget * viewMh;
    GtkWidget * tabsMh;
    GtkWidget * helpMh;
    GtkWidget * viewTabMh;
//    GtkWidget * tab1;
    GtkWidget * tabH;
    GtkWidget * tabV;
    GtkWidget * tabM;
    GtkWidget * findMi;
    GtkWidget * setwMi;
    GtkWidget * cTabMi;
    GtkWidget * nTabMi;
    GtkWidget * quitMi;
};

struct tool_st
{
    GtkWidget * top;
    GtkToolButton * backTb;
    GtkToolButton * forwardTb;
    GtkContainer * addressTi;
    GtkEntry * addressEn;
    GtkToolButton * reloadTb;
    GtkImage * reloadIo;
};

struct webt_st
{
    GtkNotebook * tabsNb;
    WebKitWebContext * webc;
    WebKitSettings * webs;
};

struct find_st
{
    GtkWidget * top;
    GtkSearchBar * findSb;
    GtkEntry * findEn;
    GtkToolButton * backTb;
    GtkToolButton * forwardTb;
    GtkToolButton * closeTb;
};

struct sign_st
{
	gulong nb_changed;
};

struct call_st
{
    struct menu_st * menu;
    struct tool_st * tool;
    struct webt_st * webv;
    struct find_st * find;
    struct sign_st * sign;
    GtkWidget * twin;
    GtkApplication * gApp;
    GSettings * gSet;
};

struct dpco_st //Dual Pointer (Call and Other)
{
	struct call_st * call;
	void * other;
};

struct call_st * G_call;

struct newt_st
{
    WebKitWebView * webv;
    struct call_st * call;
};

void connect_signals (WebKitWebView * wv, struct call_st * c);

void c_notebook_tabs_autohide(GtkToggleButton * cbmi
	,struct call_st * c);

static void c_destroy_window(GtkWidget* widget, struct call_st * c);

gboolean c_destroy_window_request(GtkWidget * widget, GdkEvent * e
	,struct call_st * call);

static void c_show_tab(WebKitWebView * wv, void * v);

void c_notebook_tabs_changed(GtkNotebook * nb, GtkWidget * w
	,guint n, struct call_st * c);

void c_onrelease_tabsMh(GtkMenuItem * mi, struct call_st * c);

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na ,struct call_st * c);
