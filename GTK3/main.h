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

struct menu_st
{
    GtkWidget * menu;
    GtkWidget * fileMenu;
    GtkWidget * editMenu;
    GtkWidget * viewMenu;
    GtkWidget * viewTabMenu;
    GtkWidget * helpMenu;
    GtkWidget * fileMh;
    GtkWidget * editMh;
    GtkWidget * viewMh;
    GtkWidget * helpMh;
    GtkWidget * viewTabMh;
    GtkWidget * tab1;
    GtkWidget * tabH;
    GtkWidget * tabV;
    GtkWidget * findMi;
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

struct call_st
{
    struct menu_st * menu;
    struct tool_st * tool;
    struct webt_st * webv;
    struct find_st * find;
    GtkWidget * twin;
};

struct call_st * G_call;

struct newt_st
{
    WebKitWebView * webv;
    struct call_st * call;
};

void connect_signals (WebKitWebView * wv, struct call_st * c);


static void destroyWindowCb(GtkWidget* widget, GtkWidget* window);
static void c_show_tab(WebKitWebView * wv, void * v);

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na ,struct call_st * c);
