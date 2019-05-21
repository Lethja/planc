#include "main.h"
#include "libdatabase.h"
#include <stdint.h>

static void c_goto_dial(GtkMenuItem * mi, GdkEventButton * e, void * v)
{
	WebKitWebView * wk = WK_CURRENT_TAB(get_web_view_notebook());
	char * url = sql_speed_dial_get_by_name
		(gtk_menu_item_get_label(mi));
	if(!url)
		return;
	char * purl = prepAddress(url);
	free(url);
	if (e->button == 2)
		new_tab_ext(purl, (PlancWindow *) get_web_view(), TRUE);
	else
		webkit_web_view_load_uri(wk,purl);
	free(purl);
}

static int gotoIter(void * store, int count, char **data
	,char **columns)
{
	if(count == 3)
	{
		GtkWidget * mi;
		if(data[2])
			mi = gtk_menu_item_new_with_label(data[2]);
		else
			mi = gtk_menu_item_new_with_label(data[1]);
		uintptr_t id;
		memcpy(&id,data[0],sizeof(uintptr_t));
		g_signal_connect(mi,"button-release-event"
			,G_CALLBACK(c_goto_dial), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(store), mi);
		gtk_widget_show_all((GtkWidget *)store);
	}
	return 0;
}

void c_show_gotoMenu(GtkMenuItem * mi, PlancWindow * v)
{
    //Make a menu of all dials on the fly then display it
	struct call_st * c = planc_window_get_call(v);
	sql_speed_dial_read_to_menu(c->menu->gotoMenu, &gotoIter);
}

void c_hide_gotoMenu(GtkMenuItem * mi, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    GList * list = gtk_container_get_children(
        (GtkContainer *)c->menu->gotoMenu);
    for (GList * l = list; l != NULL; l = l->next)
    {
        gtk_widget_destroy(l->data);
    }
}

void c_select_tabsMi(GtkWidget * w, struct dpco_st * dp)
{
    gtk_notebook_set_current_page(dp->call->tabs
        ,gtk_notebook_page_num(dp->call->tabs,dp->other));
}

gboolean c_onclick_tabsMi(GtkMenuItem * mi, GdkEventButton * e
    ,struct dpco_st * dp)
{
    if (e->button == 2)
    {
        if(gtk_notebook_get_n_pages(dp->call->tabs) > 1)
        {
            gtk_notebook_remove_page(dp->call->tabs
                ,gtk_notebook_page_num(dp->call->tabs
                ,dp->other));
            gtk_widget_set_sensitive(GTK_WIDGET(mi),FALSE);
            return TRUE;
        }
    }
    gtk_notebook_set_current_page(dp->call->tabs
        ,gtk_notebook_page_num(dp->call->tabs,dp->other));
    return FALSE;
}

void c_destroy_tabsMi(GtkMenuItem * mi, struct dpco_st * dp)
{
    free(dp);
}

void c_hide_tabsMenu(GtkMenuItem * mi, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    GList * list = gtk_container_get_children(
        (GtkContainer *)c->menu->tabsMenu);
    for (GList * l = g_list_last(list); l != NULL; l = l->prev)
    {
        gtk_widget_destroy(l->data);
    }
}

static GtkRadioMenuItem * add_tabMi(gint it, GSList * list
	,struct call_st * c)
{
	GtkWidget * l = gtk_bin_get_child(GTK_BIN
		(gtk_notebook_get_tab_label(c->tabs
		,gtk_notebook_get_nth_page(c->tabs,it))));
	GtkRadioMenuItem * n
		= (GtkRadioMenuItem *)gtk_radio_menu_item_new(list);
	gtk_menu_item_set_label((GtkMenuItem *) n
		,gtk_label_get_text((GtkLabel *)l));
	if(gtk_notebook_get_nth_page(c->tabs,it)
		== WK_CURRENT_TAB_WIDGET(c->tabs))
	{
		gtk_check_menu_item_set_active((GtkCheckMenuItem *) n, TRUE);
	}
	struct dpco_st * dp = malloc(sizeof(struct dpco_st));
	dp->call = c;
	dp->other= gtk_notebook_get_nth_page(c->tabs,it);
	gtk_menu_shell_append(GTK_MENU_SHELL(c->menu->tabsMenu)
		,(GtkWidget *) n);

	g_signal_connect(n,"button-release-event"
		,G_CALLBACK(c_onclick_tabsMi), dp);

	if(g_settings_get_boolean(G_SETTINGS, "planc-tabs-hover"))
		g_signal_connect(n, "select", G_CALLBACK(c_select_tabsMi), dp);

	g_signal_connect(n, "destroy", G_CALLBACK(c_destroy_tabsMi), dp);
	return n;
}

void c_show_tabsMenu(GtkMenuItem * mi, PlancWindow * v)
{
    //Make a menu of all tabs on the fly then display it
    struct call_st * c = planc_window_get_call(v);

	GtkRadioMenuItem * f = add_tabMi(0,NULL,c);

	GSList * r = gtk_radio_menu_item_get_group(f);

	if(gtk_notebook_get_n_pages(c->tabs) > 1)
	{
		for(gint i = 1; i < gtk_notebook_get_n_pages(c->tabs); i++)
		{
			add_tabMi(i,r,c);
		}
    }
    gtk_widget_show_all((GtkWidget *)mi);
}

void t_tabs_menu(PlancWindow * v, gboolean b)
{
    struct call_st * c = planc_window_get_call(v);
    if(b)
    {
        c->menu->tabsMh = gtk_menu_item_new_with_mnemonic("_Tabs");
        gtk_menu_shell_insert(GTK_MENU_SHELL(c->menu->menu)
            ,c->menu->tabsMh,4);
        gtk_widget_show_all(c->menu->menu);
        c->menu->tabsMenu = gtk_menu_new();
        gtk_widget_add_events(c->menu->tabsMenu,GDK_KEY_PRESS_MASK);
        gtk_menu_item_set_submenu((GtkMenuItem *) c->menu->tabsMh
            ,c->menu->tabsMenu);
        g_signal_connect(c->menu->tabsMenu, "show"
            ,G_CALLBACK(c_show_tabsMenu), v);
        g_signal_connect_after(c->menu->tabsMenu,"hide"
            ,G_CALLBACK(c_hide_tabsMenu),v);
    }
    else
    {
        gtk_container_remove(GTK_CONTAINER(c->menu->menu)
            ,c->menu->tabsMh);
        c->menu->tabsMh = NULL;
    }
}
