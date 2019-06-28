#ifndef __PLANC_WINDOWCONTAINER_H__
#define __PLANC_WINDOWCONTAINER_H__

#include "config.h"

#ifndef NO_INCLUDE_WITHIN_HEADERS
#include <gtk/gtk.h>
#endif

struct menu_st
{
    GtkWidget * menu;
    GtkWidget * fileMenu;
    GtkWidget * editMenu;
    GtkWidget * viewMenu;
    GtkWidget * viewTabMenu;
    GtkWidget * viewZoomMenu;
    GtkWidget * helpMenu;
    GtkWidget * fileMh;
    GtkWidget * editMh;
    GtkWidget * viewMh;
    GtkWidget * helpMh;
    GtkWidget * viewTabMh;
    GtkWidget * tabH;
    GtkWidget * tabV;
    GtkWidget * viewZoomMh;
    GtkWidget * znorm;
    GtkWidget * zin;
    GtkWidget * zout;
    GtkWidget * findMi;
    GtkWidget * setwMi;
    GtkWidget * cTabMi;
    GtkWidget * nTabMi;
    GtkWidget * nWinMi;
    GtkWidget * quitMi;
    GtkWidget * histMi;
    GtkWidget * downMi;
#ifdef PLANC_FEATURE_DMENU
    GtkWidget * tabsMenu;
    GtkWidget * gotoMenu;
    GtkWidget * tabsMh;
    GtkWidget * gotoMh;
    GtkWidget * tabM;
#endif
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
    gboolean usrmod; //Has the user modified the address bar?
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
	guint inhibit;
	guint waiter;
};

struct call_st
{
    struct menu_st 	* menu;
    struct tool_st 	* tool;
    struct find_st 	* find;
    struct sign_st 	* sign;
    GtkNotebook 	* tabs;
};

#define PLANC_TYPE_WINDOW \
	(planc_window_get_type ())
#define PLANC_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANC_TYPE_WINDOW \
	,PlancWindow))
#define PLANC_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST  ((klass), PLANC_TYPE_WINDOW \
	,PlancWindowClass))
#define PLANC_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANC_TYPE_WINDOW))
#define PLANC_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE  ((klass), PLANC_TYPE_WINDOW))
#define PLANC_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS  ((obj), PLANC_TYPE_WINDOW \
	,PlancWindowClass))

typedef struct _PlancWindow      	PlancWindow;
typedef struct _PlancWindowClass	PlancWindowClass;

struct _PlancWindow
{
    GtkApplicationWindow parent_instance;
    struct call_st * call;
};

struct _PlancWindowClass
{
    GtkApplicationWindowClass parent_class;
};

GType planc_window_get_type(void);

PlancWindow * planc_window_new(GtkApplication * app);
void planc_window_set_call(PlancWindow * self, struct call_st * call);
struct call_st * planc_window_get_call(PlancWindow * self);

#endif /* __PLANC_WINDOWCONTAINER_H__ */
