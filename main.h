#include "config.h"
#include <stdlib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <ctype.h>
#include "plan.h"

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

struct dpco_st //Dual Pointer (Call and Other)
{
	struct call_st * call;
	void * other;
};

struct newt_st
{
    WebKitWebView * webv;
	PlancWindow * plan;
	gboolean show;
};

void c_free_docp(gpointer data, GClosure *closure);

void connect_signals (WebKitWebView * wv, PlancWindow * v);

void c_notebook_tabs_autohide(GtkToggleButton * cbmi
	,PlancWindow * v);

void c_notebook_tabs_changed(GtkNotebook * nb, GtkWidget * w
	,guint n, PlancWindow * v);

void c_onrelease_tabsMh(GtkMenuItem * mi, PlancWindow * v);

void InitWindow(GApplication * app, gchar ** argv, int argc);

extern void * new_tab_ext(char * url, PlancWindow * v
	,gboolean show);
