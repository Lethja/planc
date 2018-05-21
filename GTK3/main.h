#include "config.h"
#include <stdlib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <ctype.h>

#define WK_CURRENT_TAB(notebook) \
(WebKitWebView *) gtk_notebook_get_nth_page(notebook \
    ,gtk_notebook_get_current_page(notebook))

#define WK_CURRENT_TAB_WIDGET(notebook) \
(GtkWidget *) gtk_notebook_get_nth_page(notebook \
    ,gtk_notebook_get_current_page(notebook))

#define WK_TAB_CHAR_LEN 40

extern GtkApplication * G_APP;
extern GSettings * G_SETTINGS;
extern GtkWindow * G_HISTORY;
extern GtkWindow * G_DOWNLOAD;
extern WebKitSettings * G_WKC_SETTINGS;
extern WebKitWebContext * G_WKC; //Global WebKit Context

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
    GtkWidget * tabH;
    GtkWidget * tabV;
    GtkWidget * tabM;
    GtkWidget * findMi;
    GtkWidget * setwMi;
    GtkWidget * cTabMi;
    GtkWidget * nTabMi;
    GtkWidget * nWinMi;
    GtkWidget * quitMi;
    GtkWidget * histMi;
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

struct find_st
{
    GtkWidget * top;
    GtkSearchEntry * findSb;
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
    struct menu_st 	* menu;
    struct tool_st 	* tool;
    struct find_st 	* find;
    struct sign_st 	* sign;
    GtkNotebook 	* tabs;
    GtkWidget 		* twin;
};

struct dpco_st //Dual Pointer (Call and Other)
{
	struct call_st * call;
	void * other;
};

struct newt_st
{
    WebKitWebView * webv;
    struct call_st * call;
    gboolean show;
};

void c_free_docp(gpointer data, GClosure *closure);

void connect_signals (WebKitWebView * wv, struct call_st * c);

void c_notebook_tabs_autohide(GtkToggleButton * cbmi
	,struct call_st * c);

static void c_show_tab(WebKitWebView * wv, struct newt_st * newtab);

void c_notebook_tabs_changed(GtkNotebook * nb, GtkWidget * w
	,guint n, struct call_st * c);

void c_onrelease_tabsMh(GtkMenuItem * mi, struct call_st * c);

void InitWindow(GApplication * app, gchar ** argv, int argc);

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na ,struct call_st * c);

extern void * new_tab_ext(char * url, struct call_st * c
	,gboolean show);
