#include "main.h"
#include "settings.h"
#include "database.h"
#include "download.h"
#include "history.h"

GtkApplication * G_APP			= NULL;
GSettings * G_SETTINGS			= NULL;
GtkWindow * G_HISTORY			= NULL;
GtkWindow * G_DOWNLOAD			= NULL;
WebKitSettings * G_WKC_SETTINGS	= NULL;
WebKitWebContext * G_WKC		= NULL;

static void c_show_tab(WebKitWebView * wv, struct newt_st * newtab);

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na ,struct call_st * c);

/** Sanitize the address for webkit
 * Returned value never null and always must be freed after use
 * Will convert numbers to speed dial addresses if avaliable
**/
char * prepAddress(const gchar * c)
{
	char * p;
	if(strcmp(c,"") == 0 || strstr(c,"about:") == c)
	{
		p = malloc(strlen("about:blank")+1);
		strncpy(p,"about:blank",strlen("about:blank")+1);
		return p;
	}
	/*Check if speed dial entry
	 * isdigit() means it's impossible to confuse with ipv4 (127.0.0.1)
	*/
	{
		size_t i = 0;
		for(; i < strlen(c); i++)
		{
			if(!isdigit(c[i]))
				break;
		}
		if(i == strlen(c))
		{
			char * x = sql_speed_dial_get(atoi(c));
			if(x)
				c = (gchar *) x;
		}
	}
	//Check if protocol portion of the url exists add it if it dosen't
    p = strstr(c,"://");
    if(!p)
    {
        size_t s = strlen("http://")+strlen(c)+1;
        p = malloc(s);
        snprintf(p,s,"http://%s",c);
    }
    else
    {
        p = malloc(strlen(c)+1);
        memcpy(p,c,strlen(c)+1);
    }
    return p;
}

GtkWidget * update_tab_label(const gchar * str, GtkWidget * label)
{
    GtkWidget * l = gtk_bin_get_child(GTK_BIN(label));
    gtk_label_set_text((GtkLabel *)l,str);
    return l;
}

void update_addr_entry(GtkEntry * a, const gchar * s)
{
    if(!gtk_widget_has_focus((GtkWidget *) a))
        gtk_entry_set_text(a,s);
}

void update_win_label(GtkWidget * win, GtkNotebook * nb, GtkWidget * e)
{
    GtkWidget * l = gtk_bin_get_child(GTK_BIN
        (gtk_notebook_get_tab_label(nb,e)));
    gchar * str = g_strconcat(gtk_label_get_text((GtkLabel *)l)
		," - Plan C", NULL);
    gtk_window_set_title((GtkWindow *) win, str);
    if(str)
        g_free(str);
}

void update_tab(GtkNotebook * nb, WebKitWebView * ch)
{
    GtkWidget * l = gtk_bin_get_child(GTK_BIN
        (gtk_notebook_get_tab_label(nb,(GtkWidget *) ch)));

    const gchar * c = webkit_web_view_get_title(ch);
    if(c && strcmp(c,"") != 0)
        gtk_label_set_text((GtkLabel *)l,c);
    else
        gtk_label_set_text((GtkLabel *)l,webkit_web_view_get_uri(ch));
}

void c_free_docp(gpointer data, GClosure *closure)
{
	free(data);
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

void c_onrelease_tabsMh(GtkMenuItem * mi, struct call_st * c)
{
	GList * list = gtk_container_get_children(
		(GtkContainer *)c->menu->tabsMenu);
	for (GList * l = list; l != NULL; l = l->next)
	{
		gtk_widget_destroy(l->data);
	}
}

void c_onclick_tabsMh(GtkMenuItem * mi, struct call_st * c)
{
	//Make a menu of all tabs on the fly then display it
	{
		GtkWidget * l = gtk_bin_get_child(GTK_BIN
			(gtk_notebook_get_tab_label(c->tabs
			,WK_CURRENT_TAB_WIDGET(c->tabs))));
		GtkWidget * ct = gtk_menu_item_new();
		gtk_menu_item_set_label((GtkMenuItem *)ct
			,gtk_label_get_text((GtkLabel* )l));
		struct dpco_st * dp = malloc(sizeof(struct dpco_st));
		dp->call = c;
		dp->other = WK_CURRENT_TAB(c->tabs);
		g_signal_connect(ct,"button-release-event"
			,G_CALLBACK(c_onclick_tabsMi), dp);
		g_signal_connect(ct,"select"
			,G_CALLBACK(c_select_tabsMi), dp);
		g_signal_connect(ct,"destroy"
			,G_CALLBACK(c_destroy_tabsMi), dp);

		gtk_menu_shell_append(GTK_MENU_SHELL(c->menu->tabsMenu),ct);
	}

	if(gtk_notebook_get_n_pages(c->tabs) != 1)
	{
		gtk_menu_shell_append(GTK_MENU_SHELL(c->menu->tabsMenu)
			,gtk_separator_menu_item_new());

		for(gint i = 0; i < gtk_notebook_get_n_pages(c->tabs)
			;i++)
		{
			if(gtk_notebook_get_nth_page(c->tabs,i)
				!= WK_CURRENT_TAB_WIDGET(c->tabs))
			{
				GtkWidget * l = gtk_bin_get_child(GTK_BIN
					(gtk_notebook_get_tab_label(c->tabs
					,gtk_notebook_get_nth_page(c->tabs,i))));
				GtkWidget * n = gtk_menu_item_new();
				gtk_menu_item_set_label((GtkMenuItem *)n
					,gtk_label_get_text((GtkLabel* )l));
				struct dpco_st * dp = malloc(sizeof(struct dpco_st));
				dp->call = c;
				dp->other= gtk_notebook_get_nth_page(c->tabs,i);
				g_signal_connect(n,"button-release-event"
					,G_CALLBACK(c_onclick_tabsMi), dp);
				g_signal_connect(n,"select"
					,G_CALLBACK(c_select_tabsMi), dp);
				g_signal_connect(n,"destroy"
					,G_CALLBACK(c_destroy_tabsMi), dp);
				gtk_menu_shell_append(GTK_MENU_SHELL(c->menu->tabsMenu)
					,n);
			}
		}
	}
	gtk_widget_show_all((GtkWidget *)mi);
}

void t_tabs_menu(struct call_st * c, gboolean b)
{
	if(b)
	{
		c->menu->tabsMh = gtk_menu_item_new_with_mnemonic("_Tabs");
		gtk_menu_shell_insert(GTK_MENU_SHELL(c->menu->menu)
			,c->menu->tabsMh,3);
		gtk_widget_show_all(c->menu->menu);
		c->menu->tabsMenu = gtk_menu_new();
		gtk_widget_add_events(c->menu->tabsMenu,GDK_KEY_PRESS_MASK);
		gtk_menu_item_set_submenu((GtkMenuItem *) c->menu->tabsMh
			,c->menu->tabsMenu);
		g_signal_connect(c->menu->tabsMh, "select"
			,G_CALLBACK(c_onclick_tabsMh), c);
		g_signal_connect_after(c->menu->tabsMh,"deselect"
			,G_CALLBACK(c_onrelease_tabsMh),c);
	}
	else
	{
		gtk_container_remove(GTK_CONTAINER(c->menu->menu)
			,c->menu->tabsMh);
		c->menu->tabsMh = NULL;
	}
}

void c_update_tabs_layout(GtkWidget * r, struct call_st * c)
{
	if(r == c->menu->tabM && !c->menu->tabsMh)
	{
		gtk_notebook_set_show_tabs(c->tabs,FALSE);
		t_tabs_menu(c,TRUE);
		return;
	}
	else if(c->menu->tabsMh)
		t_tabs_menu(c,FALSE);

	if(r == c->menu->tabH)
		gtk_notebook_set_tab_pos(c->tabs,GTK_POS_TOP);
	else if(r == c->menu->tabV)
		gtk_notebook_set_tab_pos(c->tabs,GTK_POS_LEFT);

	if(g_settings_get_boolean(G_SETTINGS,"tab-autohide"))
		c_notebook_tabs_changed(c->tabs,NULL,0,c);
	else
		gtk_notebook_set_show_tabs(c->tabs,TRUE);
}

static gboolean c_notebook_scroll(GtkWidget * w, GdkEventScroll * e
	,void * v)
{
	switch(e->direction)
	{
		case GDK_SCROLL_UP:
		case GDK_SCROLL_LEFT:
			gtk_notebook_prev_page((GtkNotebook *) w);
		break;
		case GDK_SCROLL_DOWN:
		case GDK_SCROLL_RIGHT:
			gtk_notebook_next_page((GtkNotebook *) w);
		default:
		break;

	}
	return FALSE;
}

static gboolean c_notebook_click(GtkWidget * w, GdkEventButton * e
    ,struct dpco_st * dp)
{
    if (e->button == 2)
    {
        if(gtk_notebook_get_n_pages(dp->call->tabs) > 1)
            gtk_notebook_remove_page(dp->call->tabs
                ,gtk_notebook_page_num(dp->call->tabs
				,dp->other));
		return TRUE;
    }
    return FALSE;
}

void c_notebook_close_current(GtkWidget * w, struct call_st * c)
{
	if(gtk_notebook_get_n_pages(c->tabs) > 1)
		gtk_notebook_remove_page
			(c->tabs,gtk_notebook_get_current_page
			(c->tabs));
}

void c_notebook_tabs_changed(GtkNotebook * nb, GtkWidget * w
	,guint n, struct call_st * c)
{
	if(g_settings_get_boolean(G_SETTINGS,"tab-autohide")
		&& !gtk_check_menu_item_get_active(
		(GtkCheckMenuItem *)c->menu->tabM))
	{
		if(gtk_notebook_get_n_pages(c->tabs) == 1)
			gtk_notebook_set_show_tabs(c->tabs,FALSE);
		else
			gtk_notebook_set_show_tabs(c->tabs,TRUE);
	}
}

static gboolean c_enter_fullscreen(GtkWidget * widget
	,struct call_st * c)
{
    gtk_widget_hide(GTK_WIDGET(c->menu->menu));
    gtk_widget_hide(GTK_WIDGET(c->tool->top));
    gtk_notebook_set_show_tabs(c->tabs,FALSE);
    return 0;
}

static gboolean c_leave_fullscreen(GtkWidget * widget
	,struct call_st * c)
{
    gtk_widget_show(GTK_WIDGET(c->menu->menu));
    gtk_widget_show(GTK_WIDGET(c->tool->top));
    if(gtk_check_menu_item_get_active(
		(GtkCheckMenuItem *)c->menu->tabM))
		return FALSE;
    if(g_settings_get_boolean(G_SETTINGS,"tab-autohide"))
		c_notebook_tabs_changed(c->tabs,NULL,0,c);
	else
		gtk_notebook_set_show_tabs(c->tabs,TRUE);
    return FALSE;
}

static gboolean c_policy (WebKitWebView *wv ,WebKitPolicyDecision *d
    ,WebKitPolicyDecisionType t, void * v)
{
    switch (t) {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:

        if(webkit_navigation_action_get_mouse_button
            (webkit_navigation_policy_decision_get_navigation_action
            ((WebKitNavigationPolicyDecision *)d)) == 2)
        {
            WebKitWebView * nt = c_new_tab_url(wv
                ,webkit_navigation_policy_decision_get_navigation_action
                ((WebKitNavigationPolicyDecision *) d),v);

            g_signal_emit_by_name(nt,"ready-to-show");
            webkit_policy_decision_ignore(d);
        }
        break;
    case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:

        if(webkit_response_policy_decision_is_mime_type_supported
            ((WebKitResponsePolicyDecision *) d))
        {
            webkit_policy_decision_use(d);
        }
        else
        {
            webkit_policy_decision_download(d);
            return TRUE;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void c_toggleSearch(GtkWidget * w, struct call_st * c)
{
    if(gtk_widget_get_visible(c->find->top))
    {
        gtk_widget_hide(c->find->top);
        webkit_find_controller_search_finish
            (webkit_web_view_get_find_controller
            (WK_CURRENT_TAB(c->tabs)));
        gtk_widget_grab_focus
            (WK_CURRENT_TAB_WIDGET(c->tabs));
    }
    else
    {
        gtk_widget_show(c->find->top);
        gtk_widget_grab_focus((GtkWidget *) c->find->findSb);
    }
}

static int c_toggleSearchEvent(GtkWidget * w, GdkEvent * e
	,struct call_st * c)
{
	c_toggleSearch(w,c);
	return FALSE;
}

static void c_search_page(GtkEditable * w, GtkNotebook * n)
{
    WebKitFindController * f
        = webkit_web_view_get_find_controller(WK_CURRENT_TAB(n));

    webkit_find_controller_search(f,gtk_entry_get_text(GTK_ENTRY(w))
        ,WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE
        | WEBKIT_FIND_OPTIONS_WRAP_AROUND, -1);
}

static void c_search_nxt(GtkButton * b, GtkNotebook * n)
{
    const gchar * s = webkit_find_controller_get_search_text
        (webkit_web_view_get_find_controller(WK_CURRENT_TAB(n)));
    if(s)
        webkit_find_controller_search_next
			(webkit_web_view_get_find_controller(WK_CURRENT_TAB(n)));
}

static void c_search_prv(GtkButton * b, GtkNotebook * n)
{
    const gchar * s = webkit_find_controller_get_search_text
        (webkit_web_view_get_find_controller(WK_CURRENT_TAB(n)));
    if(s)
        webkit_find_controller_search_previous
			(webkit_web_view_get_find_controller(WK_CURRENT_TAB(n)));
}

static char addrEntryState_webView(GtkEditable * e, WebKitWebView * wv
    ,struct call_st * c)
{
    if(webkit_web_view_is_loading(wv))
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"process-stop",GTK_ICON_SIZE_SMALL_TOOLBAR);
		c->tool->usrmod = FALSE;
        return 0;
    }
    if(strcmp(gtk_entry_get_text((GtkEntry *) e)
        ,webkit_web_view_get_uri(wv)) == 0)
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"view-refresh",GTK_ICON_SIZE_SMALL_TOOLBAR);
		c->tool->usrmod = FALSE;
        return 1;
    }
    else
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"go-last",GTK_ICON_SIZE_SMALL_TOOLBAR);
		c->tool->usrmod = TRUE;
        return 2;
    }
}

static char addrEntryState(GtkEditable * e, struct call_st * c)
{
    return addrEntryState_webView(e, WK_CURRENT_TAB(c->tabs),c);
}

static void addrWebviewState(WebKitWebView * wv, WebKitLoadEvent evt
	,struct call_st * c)
{
	if(WK_CURRENT_TAB(c->tabs) == wv)
		addrEntryState_webView((GtkEditable *) c->tool->addressEn
			,wv, c);
}

static void c_addr_ins(GtkEditable * e, gchar* t, gint l, gpointer p
	,struct call_st * c)
{
    addrEntryState(e,c);
}

static void c_addr_del(GtkEditable* e, gint sp, gint ep
    ,struct call_st * c)
{
    addrEntryState(e,c);
}

static void c_refresh(GtkWidget * widget, void * v)
{
    if(webkit_web_view_is_loading(WK_CURRENT_TAB(v)))
        webkit_web_view_stop_loading(WK_CURRENT_TAB(v));
    else
        webkit_web_view_reload(WK_CURRENT_TAB(v));
}

static void c_go_back(GtkWidget * widget, struct call_st * c)
{
    webkit_web_view_go_back(WK_CURRENT_TAB(c->tabs));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(c->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB
		(c->tabs)));
}

static void c_go_forward(GtkWidget * widget, struct call_st * c)
{
    webkit_web_view_go_forward(WK_CURRENT_TAB(c->tabs));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(c->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB
        (c->tabs)));
}

static void c_open_settings(GtkWidget * w, struct call_st * c)
{
	InitSettingsWindow(c);
}

static void c_open_history(GtkWidget * w, struct call_st * c)
{
	InitHistoryWindow(c);
}

static void c_open_download(GtkWidget * w, struct call_st * c)
{
	InitDownloadWindow();
}

static void c_accl_rels(GtkWidget * w, GdkEvent * e, struct call_st * c)
{
    guint k;
    guint m;
    if(gdk_event_get_keyval(e,&k))
    {
		if(gdk_event_get_state(e,&m))
		{
			switch(k)
			{
			case GDK_KEY_Tab:
				if(m & GDK_CONTROL_MASK)
					gtk_notebook_next_page(c->tabs);
			break;
			case GDK_KEY_ISO_Left_Tab:
				if(m & GDK_CONTROL_MASK)
					gtk_notebook_prev_page(c->tabs);
			break;
			}
		}
        switch(k)
        {
        case GDK_KEY_F5:
            c_refresh(w,c->tabs);
        break;
        case GDK_KEY_F6:
			if(gtk_widget_is_focus((GtkWidget *) c->tool->addressEn))
				gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(c->tabs));
			else
				gtk_widget_grab_focus((GtkWidget *) c->tool->addressEn);
        break;
        case GDK_KEY_F12:
			{
				gboolean b;
				g_object_get(G_WKC_SETTINGS
					,"enable-developer-extras",&b,NULL);
				if(b)
				{
					WebKitWebInspector * wi
						= webkit_web_view_get_inspector
						(WK_CURRENT_TAB(c->tabs));
					webkit_web_inspector_show(wi);
				}
			}
		break;
        }
    }
}

static void c_act(GtkWidget * widget, void * v)
{
    struct call_st * call = (struct call_st *) v;
    char * curi = NULL;
    switch(addrEntryState((GtkEditable *) widget,v))
    {
        case 0: //Stop loading current page before requesting new uri
            webkit_web_view_stop_loading
                (WK_CURRENT_TAB(call->tabs));

        case 2: //Go
            curi = prepAddress(gtk_entry_get_text
                (GTK_ENTRY(call->tool->addressEn)));
            webkit_web_view_load_uri(WK_CURRENT_TAB(call->tabs)
                ,curi);
            if(curi)
                free(curi);
        break;
    }
    gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(call->tabs));
}

static void c_switch_tab(GtkNotebook * nb, GtkWidget * page
    ,guint i, void * v)
{
    WebKitWebView * wv = (WebKitWebView *) page;
    struct call_st * call = v;
    update_tab(nb,wv);

    update_win_label(call->twin,nb,(GtkWidget *) wv);

    const gchar * url = webkit_web_view_get_uri(wv);
    gtk_entry_set_text(call->tool->addressEn, url);

    if(strcmp(url,"") == 0 || strcmp(url,"about:blank") == 0)
        gtk_widget_grab_focus(GTK_WIDGET(call->tool->addressEn));
	else
		gtk_widget_grab_focus(GTK_WIDGET(wv));

    addrEntryState_webView((GtkEditable *) call->tool->addressEn, wv
        ,call);

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(wv));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(wv));
}

static gboolean c_update_favicon(WebKitWebView * wv, GParamSpec *pspec,
	void * v)
{
    cairo_surface_t* surface;
    GdkPixbuf * pixbuf;

    /*if ((surface = webkit_web_view_get_favicon (view)) &&
        (pixbuf = gdk_pixbuf_get_from_surface (
            surface, 0, 0,
            cairo_image_surface_get_width (surface),
            cairo_image_surface_get_height (surface)))
        ) {
        gtk_window_set_icon (window, pixbuf);
    }*/

    return TRUE;
}

static void c_update_title(WebKitWebView * webv, WebKitLoadEvent evt
    ,struct call_st * v)
{
    update_tab((GtkNotebook *) v->tabs,webv);
    if(webv == WK_CURRENT_TAB(v->tabs))
    {
        update_win_label(v->twin,v->tabs
            ,WK_CURRENT_TAB_WIDGET(v->tabs));

        update_addr_entry(v->tool->addressEn
            ,webkit_web_view_get_uri(webv));
    }
}

static void c_loads(WebKitWebView * wv, WebKitWebResource * res
    ,WebKitURIRequest  *req, gpointer v)
{
    struct call_st * call = v;
    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back
        (WK_CURRENT_TAB(call->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward
        (WK_CURRENT_TAB(call->tabs)));
}


static void c_load(WebKitWebView * webv, WebKitLoadEvent evt ,void * v)
{
    struct call_st * call = (struct call_st *) v;
    switch(evt)
    {
        case WEBKIT_LOAD_STARTED:
        case WEBKIT_LOAD_COMMITTED:
        case WEBKIT_LOAD_REDIRECTED:
            gtk_image_set_from_icon_name(call->tool->reloadIo
                ,"process-stop",GTK_ICON_SIZE_SMALL_TOOLBAR);
        break;

        case WEBKIT_LOAD_FINISHED:
            gtk_image_set_from_icon_name(call->tool->reloadIo
                ,"view-refresh",GTK_ICON_SIZE_SMALL_TOOLBAR);
			if(!webkit_web_view_is_ephemeral(webv)
			&& strcmp("about:blank",webkit_web_view_get_uri(webv)) != 0
			&& strcmp("",webkit_web_view_get_uri(webv)) != 0)
				sql_history_write(webkit_web_view_get_uri(webv)
					,webkit_web_view_get_title(webv));
        break;
    }

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back
        (WK_CURRENT_TAB(call->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward
        (WK_CURRENT_TAB(call->tabs)));

    if(webkit_web_view_get_uri(webv))
    {
        if(webv == WK_CURRENT_TAB(call->tabs))
        {
            update_addr_entry(call->tool->addressEn
                ,webkit_web_view_get_uri(webv));
        }
    }

    if(!webkit_web_view_get_title(webv))
    {
        if(webkit_web_view_get_uri(webv))
        {
            update_tab(call->tabs,webv);
        }
        else if (gtk_entry_get_text_length(call->tool->addressEn))
        {
            update_tab(call->tabs,webv);
        }
        else
        {
            update_tab(call->tabs,webv);
        }
    }
}

GtkWidget * InitTabLabel(WebKitWebView * wv, gchar * str
	,struct call_st * c)
{
    GtkWidget * ebox = gtk_event_box_new();
    gtk_widget_set_has_window(ebox, FALSE);
    struct dpco_st * dp = malloc(sizeof(struct dpco_st));
    dp->call = c;
    dp->other=wv;
    g_signal_connect_data(ebox, "button-press-event"
        ,G_CALLBACK(c_notebook_click), dp, c_free_docp,0);
	gtk_widget_add_events(ebox, GDK_SCROLL_MASK);
    GtkWidget * label;
    if(str == NULL || strcmp(str,"") == 0)
        label = gtk_label_new("New Tab");
    else
        label = gtk_label_new(str);

    gtk_container_add(GTK_CONTAINER(ebox), label);
    gtk_label_set_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_max_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_ellipsize((GtkLabel *)label,PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign((GtkLabel *)label, 0.0);

    gtk_widget_set_halign(label,GTK_ALIGN_START);
    gtk_widget_set_halign(ebox,GTK_ALIGN_FILL);
    gtk_widget_show(GTK_WIDGET(label));
    return ebox;
}

static void c_show_tab(WebKitWebView * wv, struct newt_st * newtab)
{
    GtkWidget * ebox = InitTabLabel(wv,NULL,newtab->call);
    gtk_notebook_append_page((GtkNotebook *) newtab->call->tabs
        ,GTK_WIDGET(newtab->webv),ebox);
    gtk_widget_show_all(GTK_WIDGET(newtab->call->tabs));
    if(newtab->show)
    {
		gtk_notebook_set_current_page
			((GtkNotebook *) newtab->call->tabs
			,gtk_notebook_page_num
			(newtab->call->tabs,(GtkWidget *) newtab->webv));
		if(g_strcmp0
		(webkit_web_view_get_uri(newtab->webv), "about:blank"))
		{
			gtk_widget_grab_focus((GtkWidget *) newtab->webv);
		}
	}

    connect_signals(newtab->webv,newtab->call);
    free(newtab);
}

extern void * new_tab_ext(char * url, struct call_st * c, gboolean show)
{
	WebKitWebView * wv;
	if(g_strcmp0
		(webkit_web_view_get_uri(WK_CURRENT_TAB(c->tabs))
		,"about:blank"))
		wv = WEBKIT_WEB_VIEW
        (webkit_web_view_new_with_context(G_WKC));
	else //Blank page, use this page instead of new tab
	{
		wv = WK_CURRENT_TAB(c->tabs);
		gchar * u = prepAddress(url);
		if(u)
		{
			webkit_web_view_load_uri(wv,u);
			free(u);
			return wv;
		}
	}
	gchar * u = prepAddress(url);
	if(u)
	{
		webkit_web_view_load_uri(wv,u);
		free(u);
	}
	else return NULL;

    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = wv;
    newtab->call = c;
    newtab->show = show;
    g_signal_connect(wv, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    g_signal_emit_by_name(wv,"ready-to-show");
    return wv;
}

void c_new_win(GtkWidget * w, void * v)
{
	InitWindow(G_APPLICATION(G_APP), NULL, 0);
}

static gboolean closePrompt(struct call_st * call)
{
	gint g = gtk_notebook_get_n_pages(call->tabs);
	if(g > 1)
	{
		GtkWidget * dialog = gtk_message_dialog_new
			((GtkWindow *) call->twin
			,GTK_DIALOG_DESTROY_WITH_PARENT
			,GTK_MESSAGE_QUESTION
			,GTK_BUTTONS_YES_NO
			,"Are you sure you want to close this window with %d tabs?"
			,g);

		g = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		if(g == GTK_RESPONSE_NO)
			return TRUE;
	}
	return FALSE;
}

static gboolean c_destroy_window_request(GtkWidget * widget
	,GdkEvent * e, struct call_st * call)
{
	return closePrompt(call);
}

static void c_destroy_window(GtkWidget* widget, struct call_st * c)
{
	g_signal_handler_disconnect(c->tabs,c->sign->nb_changed);
	free(c->menu);
	free(c->find);
	free(c->sign);
	free(c->tool);
	free(c);
}

static void c_destroy_window_menu(GtkWidget * widget
	,struct call_st * c)
{
	gtk_window_close((GtkWindow *) c->twin);
}

static WebKitWebView * c_new_tab(GtkWidget * gw,struct call_st * c)
{
    WebKitWebView * nt
        = (WebKitWebView *) webkit_web_view_new_with_context(G_WKC);
	gchar * u = prepAddress(g_settings_get_string
		(G_SETTINGS,"tab-newpage"));
	if(u)
	{
		webkit_web_view_load_uri(nt,u);
		free(u);
	}
	else
		webkit_web_view_load_uri(nt,"about:blank");

    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->call = c;
    newtab->show = TRUE;
    g_signal_connect(nt, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    g_signal_emit_by_name(nt,"ready-to-show");
    return nt;
}

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na, struct call_st * c)
{
    WebKitWebView * nt
        = (WebKitWebView *) webkit_web_view_new_with_context(G_WKC);
    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->call = c;
	newtab->show = FALSE;

    webkit_web_view_load_request(nt
        ,webkit_navigation_action_get_request(na));

    g_signal_connect(nt, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    return nt;
}

void InitToolbar(struct tool_st * tool, GtkAccelGroup * accel_group)
{
    tool->top = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(tool->top), GTK_TOOLBAR_ICONS);

    tool->backTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("go-previous"
        ,GTK_ICON_SIZE_SMALL_TOOLBAR), "_Back");

    gtk_toolbar_insert(GTK_TOOLBAR(tool->top)
        ,(GtkToolItem *) tool->backTb, -1);

    gtk_widget_set_sensitive(GTK_WIDGET(tool->backTb), FALSE);

    tool->forwardTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("go-next"
        ,GTK_ICON_SIZE_SMALL_TOOLBAR), "_Forward");

    gtk_toolbar_insert(GTK_TOOLBAR(tool->top)
        ,(GtkToolItem *) tool->forwardTb, -1);
    gtk_widget_set_sensitive(GTK_WIDGET(tool->forwardTb), FALSE);
    tool->addressTi = (GtkContainer *) gtk_tool_item_new();
    tool->addressEn = (GtkEntry *) gtk_entry_new();
    gtk_widget_set_focus_on_click ((GtkWidget *) tool->addressEn, TRUE);
    gtk_widget_add_events((GtkWidget *) tool->addressTi
		,GDK_FOCUS_CHANGE_MASK);
    gtk_container_add(tool->addressTi, GTK_WIDGET(tool->addressEn));
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top)
        ,GTK_TOOL_ITEM(tool->addressTi), -1);
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(tool->addressTi), TRUE);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(tool->addressTi), TRUE);
    tool->usrmod = FALSE;

    tool->reloadIo = (GtkImage *) gtk_image_new_from_icon_name
        ("view-refresh",GTK_ICON_SIZE_SMALL_TOOLBAR);
    tool->reloadTb = (GtkToolButton *)gtk_tool_button_new
        (GTK_WIDGET(tool->reloadIo),"_Refresh");
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top)
        ,(GtkToolItem *) tool->reloadTb, -1);
}

void InitMenubar(struct menu_st * menu, struct call_st * c
	,GtkAccelGroup * accel_group)
{
	//Top Menu
    menu->menu = gtk_menu_bar_new();
    menu->fileMenu = gtk_menu_new();
    menu->editMenu = gtk_menu_new();
    menu->viewMenu = gtk_menu_new();
    menu->viewTabMenu = gtk_menu_new();
    menu->helpMenu = gtk_menu_new();

    menu->fileMh = gtk_menu_item_new_with_mnemonic("_File");
    menu->editMh = gtk_menu_item_new_with_mnemonic("_Edit");
    menu->viewMh = gtk_menu_item_new_with_mnemonic("_View");
    menu->tabsMh = NULL;
    menu->viewTabMh = gtk_menu_item_new_with_mnemonic("_Tabs");
    menu->tabH = gtk_radio_menu_item_new_with_mnemonic(NULL
		,"_Horizontal");
    menu->tabV = gtk_radio_menu_item_new_with_mnemonic_from_widget
		((GtkRadioMenuItem *)menu->tabH, "_Vertical");
	menu->tabM = gtk_radio_menu_item_new_with_mnemonic_from_widget
		((GtkRadioMenuItem *)menu->tabH, "_Menu");
    menu->helpMh = gtk_menu_item_new_with_mnemonic("_Help");
    menu->nWinMi = gtk_menu_item_new_with_mnemonic("New _Window");
    menu->nTabMi = gtk_menu_item_new_with_mnemonic("New _Tab");
    menu->cTabMi = gtk_menu_item_new_with_mnemonic("_Close Tab");
    menu->findMi = gtk_menu_item_new_with_mnemonic("_Find");
    menu->setwMi = gtk_menu_item_new_with_mnemonic("_Settings");
    menu->histMi = gtk_menu_item_new_with_mnemonic("_History");
    menu->downMi = gtk_menu_item_new_with_mnemonic("_Downloads");
    menu->quitMi = gtk_menu_item_new_with_mnemonic("_Quit");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->fileMh)
        ,menu->fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->editMh)
        ,menu->editMenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->viewMh)
        ,menu->viewMenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->viewTabMh)
        ,menu->viewTabMenu);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewMenu)
		,menu->viewTabMh);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabH);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabV);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabM);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewMenu), menu->histMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewMenu), menu->downMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->editMenu), menu->findMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->editMenu), menu->setwMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->nWinMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->nTabMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu)
		,gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->cTabMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->fileMh);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->editMh);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->viewMh);

	gtk_widget_add_accelerator(menu->cTabMi, "activate", accel_group
		,GDK_KEY_W, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(menu->findMi, "activate", accel_group
		,GDK_KEY_F, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(menu->nTabMi, "activate", accel_group
		,GDK_KEY_T, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_widget_add_accelerator(menu->nWinMi, "activate", accel_group
		,GDK_KEY_N, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_widget_add_accelerator(menu->histMi, "activate", accel_group
		,GDK_KEY_H, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_widget_add_accelerator(menu->downMi, "activate", accel_group
		,GDK_KEY_D, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    g_signal_connect(G_OBJECT(menu->cTabMi), "activate"
		,G_CALLBACK(c_notebook_close_current), c);

    g_signal_connect(G_OBJECT(menu->findMi), "activate"
		,G_CALLBACK(c_toggleSearch), c);

	g_signal_connect(G_OBJECT(menu->setwMi), "activate"
		,G_CALLBACK(c_open_settings), c);

    g_signal_connect(G_OBJECT(menu->nTabMi), "activate"
		,G_CALLBACK(c_new_tab), c);

	g_signal_connect(G_OBJECT(menu->nWinMi), "activate"
		,G_CALLBACK(c_new_win), NULL);

	g_signal_connect(G_OBJECT(menu->histMi), "activate"
		,G_CALLBACK(c_open_history), c);

	g_signal_connect(G_OBJECT(menu->downMi), "activate"
		,G_CALLBACK(c_open_download), c);

    g_signal_connect(G_OBJECT(menu->quitMi), "activate"
		,G_CALLBACK(c_destroy_window_menu), c);

	c->menu = menu;
	gint g = g_settings_get_int(G_SETTINGS,"tab-layout");
	switch(g)
	{
		case 2:
			gtk_check_menu_item_set_active
				((GtkCheckMenuItem *)menu->tabM,TRUE);
			t_tabs_menu(c,TRUE);
		break;
		case 1:
			gtk_check_menu_item_set_active
				((GtkCheckMenuItem *)menu->tabV,TRUE);
		break;
		default:
			gtk_check_menu_item_set_active
				((GtkCheckMenuItem *)menu->tabH,TRUE);
		break;
	}
}

static GtkNotebook * InitNotetab(void * v)
{
    GtkNotebook * tabs = (GtkNotebook *) gtk_notebook_new();
    gtk_notebook_set_show_border(tabs,FALSE);
    gint g = g_settings_get_int(G_SETTINGS,"tab-layout");
    switch(g)
    {
		case 2:
			gtk_notebook_set_show_tabs(tabs,FALSE);
			break;
		case 1:
			gtk_notebook_set_tab_pos(tabs,GTK_POS_LEFT);
			break;
		default:
			gtk_notebook_set_tab_pos(tabs,GTK_POS_TOP);
	}
	gboolean b = g_settings_get_boolean(G_SETTINGS,"tab-autohide");
	if(b)
		gtk_notebook_set_show_tabs(tabs,FALSE);
    gtk_notebook_set_scrollable(tabs,TRUE);
	gtk_widget_add_events((GtkWidget *) tabs, GDK_SCROLL_MASK);
    g_signal_connect(tabs, "scroll-event"
		,G_CALLBACK(c_notebook_scroll), NULL);
    return tabs;
}

static void c_wk_ext_init(WebKitWebContext *context, gpointer user_data)
{
	webkit_web_context_set_web_extensions_directory(context, WKED);
}

static void InitWebContext()
{
	char * datadir;
    char * cachedir;
    char * cookiedir;

    datadir = g_build_filename(g_get_user_data_dir(),g_get_prgname()
		,NULL);
    cachedir = g_build_filename(g_get_user_cache_dir(),g_get_prgname()
		,NULL);
    cookiedir = g_build_filename(g_get_user_data_dir(),g_get_prgname()
        ,"cookies",NULL);
    WebKitWebsiteDataManager * d = webkit_website_data_manager_new
        ("base-data-directory", cachedir, "base-cache-directory"
        ,datadir, NULL);

    g_free(datadir);
    g_free(cachedir);

    G_WKC = webkit_web_context_new_with_website_data_manager(d);

    g_signal_connect(G_WKC, "initialize-web-extensions"
		,G_CALLBACK(c_wk_ext_init), NULL);

    if(g_settings_get_boolean(G_SETTINGS,"webkit-ppt"))
		webkit_web_context_set_process_model(G_WKC
			,WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);
	else
		webkit_web_context_set_process_model(G_WKC
			,WEBKIT_PROCESS_MODEL_SHARED_SECONDARY_PROCESS);

	switch(g_settings_get_int(G_SETTINGS,"webkit-mcm"))
	{
		case 0:
			webkit_web_context_set_cache_model(G_WKC
				,WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
		break;
		default:
			webkit_web_context_set_cache_model(G_WKC
				,WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER);
		break;
		case 2:
			webkit_web_context_set_cache_model(G_WKC
				,WEBKIT_CACHE_MODEL_WEB_BROWSER);
		break;
	}

    webkit_web_context_set_spell_checking_enabled(G_WKC,TRUE);
    webkit_web_context_set_spell_checking_languages(G_WKC
        ,g_get_language_names());

    WebKitCookieManager * cm
        = webkit_web_context_get_cookie_manager(G_WKC);

    webkit_cookie_manager_set_persistent_storage(cm, cookiedir
        ,WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    webkit_cookie_manager_set_accept_policy(cm
        ,WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);

    g_free(cookiedir);

    G_WKC_SETTINGS = webkit_settings_new();

	webkit_settings_set_enable_page_cache(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-cache"));
	webkit_settings_set_enable_java(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-java"));
	webkit_settings_set_enable_javascript(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-js"));
	webkit_settings_set_enable_plugins(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-plugins"));
	webkit_settings_set_enable_webaudio(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-webaudio"));
	webkit_settings_set_enable_media_stream(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-rtc"));
	webkit_settings_set_enable_mediasource(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-mse"));
#ifdef WK_MEDIA_ENCRYPT
	webkit_settings_set_enable_encrypted_media(G_WKC_SETTINGS
		,g_settings_get_boolean(G_SETTINGS,"webkit-eme"));
#endif

	g_object_set (G_OBJECT(G_WKC_SETTINGS), "enable-developer-extras"
		,g_settings_get_boolean(G_SETTINGS,"webkit-dev"), NULL);

	g_signal_connect(G_WKC, "download-started"
        ,G_CALLBACK(c_download_start), NULL);
}

void InitWebview(struct call_st * c)
{
    WebKitWebView * wv = WEBKIT_WEB_VIEW
        (webkit_web_view_new_with_context(G_WKC));

    GtkWidget * ebox = gtk_event_box_new();
    gtk_widget_set_has_window(ebox, FALSE);
    GtkWidget * label = gtk_label_new("New Tab");
    gtk_container_add(GTK_CONTAINER(ebox), label);
    struct dpco_st * dp = malloc(sizeof(struct dpco_st));
    dp->call = c;
    dp->other=wv;
    g_signal_connect_data(ebox, "button-press-event"
        ,G_CALLBACK(c_notebook_click), dp, c_free_docp,0);
	gtk_widget_add_events(ebox, GDK_SCROLL_MASK);
    gtk_label_set_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_max_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_ellipsize((GtkLabel *)label,PANGO_ELLIPSIZE_END);
	gtk_label_set_xalign((GtkLabel *)label, 0.0);
	gtk_widget_set_halign(ebox,GTK_ALIGN_FILL);
    gtk_widget_show(GTK_WIDGET(label));
    gtk_notebook_append_page(c->tabs,GTK_WIDGET(wv),ebox);
    connect_signals(wv,c);
}

void InitFindBar(struct find_st * f, struct call_st * call)
{
    f->top = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(f->top), GTK_TOOLBAR_ICONS);

    f->findSb = (GtkSearchEntry *) gtk_search_entry_new();
    gtk_widget_set_hexpand ((GtkWidget *) f->findSb, TRUE);

    GtkContainer * c = (GtkContainer *) gtk_tool_item_new();

    gtk_container_add(c, GTK_WIDGET(f->findSb));
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(c), TRUE);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(c), TRUE);
    gtk_toolbar_insert(GTK_TOOLBAR(f->top), (GtkToolItem *) c, -1);


    f->backTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("go-previous"
        ,GTK_ICON_SIZE_SMALL_TOOLBAR)
        ,"_Previous");

    gtk_toolbar_insert(GTK_TOOLBAR(f->top)
        , (GtkToolItem *) f->backTb, -1);

    f->forwardTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("go-next"
        ,GTK_ICON_SIZE_SMALL_TOOLBAR)
        ,"_Next");

    gtk_toolbar_insert(GTK_TOOLBAR(f->top)
        ,(GtkToolItem *) f->forwardTb, -1);

	f->closeTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("window-close"
        ,GTK_ICON_SIZE_SMALL_TOOLBAR)
        ,"_Close");

    gtk_toolbar_insert(GTK_TOOLBAR(f->top)
        ,(GtkToolItem *) f->closeTb, -1);

    g_signal_connect(f->backTb, "clicked"
        ,G_CALLBACK(c_search_prv), call->tabs);
    g_signal_connect(f->forwardTb, "clicked"
        ,G_CALLBACK(c_search_nxt), call->tabs);
	g_signal_connect(f->closeTb, "clicked"
        ,G_CALLBACK(c_toggleSearch), call);
	g_signal_connect(f->findSb, "focus-out-event"
		,G_CALLBACK(c_toggleSearchEvent), call);
    g_signal_connect(f->findSb, "search-changed"
        ,G_CALLBACK(c_search_page), call->tabs);
}

void InitCallback(struct call_st * c, struct find_st * f
    ,struct menu_st * m, struct tool_st * t, struct sign_st * s
    ,GtkWidget * x)
{
    c->menu = m;
    c->find = f;
    c->sign = s;
    c->tool = t;
    c->twin = x;
}

static gboolean c_addr_unfocus(GtkEditable * w, GdkEventButton * e
    ,void * v)
{
	if (e->type == GDK_FOCUS_CHANGE)
    {
		gtk_editable_select_region(w, 0, 0);
	}
	return FALSE;
}

static gboolean c_addr_click(GtkEditable * w, GdkEventButton * e
	,void * v)
{
	if(e->button == 1)
	{
		if(e->type == GDK_2BUTTON_PRESS)
		{
			gtk_editable_select_region(w, 0, -1);
			return TRUE;
		}
	}
	return FALSE;
}

void InitWindow(GApplication * app, gchar ** argv, int argc)
{
	//There's probably a better way to do this with Glib
	struct menu_st * menu = malloc(sizeof(struct menu_st));
    struct tool_st * tool = malloc(sizeof(struct tool_st));
    struct find_st * find = malloc(sizeof(struct find_st));
    struct sign_st * sign = malloc(sizeof(struct sign_st));
    struct call_st * call = malloc(sizeof(struct call_st));

    GtkWidget * window = gtk_application_window_new
		((GtkApplication *) app);

    GtkAccelGroup * accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    GdkRectangle * res = malloc(sizeof(GdkRectangle));
    gdk_monitor_get_geometry
		(gdk_display_get_primary_monitor
		(gdk_display_get_default()),res);
    gtk_window_set_default_size(GTK_WINDOW(window)
		,res->width*0.5,res->height*0.5);
	free(res);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_window_maximize(GTK_WINDOW(window));

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    InitCallback(call,find,menu,tool,sign,window);
    InitMenubar(menu, call, accel_group);
    InitToolbar(tool, accel_group);
    call->tabs = InitNotetab(call);
    InitWebview(call);
    InitFindBar(find, call);

    gtk_box_pack_start(GTK_BOX(vbox), menu->menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tool->top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(call->tabs)
        , TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), find->top, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy"
        ,G_CALLBACK(c_destroy_window), call);
	g_signal_connect(window, "delete-event"
        ,G_CALLBACK(c_destroy_window_request), call);
    g_signal_connect(tool->addressEn, "activate"
        ,G_CALLBACK(c_act), call);
	g_signal_connect(tool->addressEn, "focus-out-event"
        ,G_CALLBACK(c_addr_unfocus), NULL);
	g_signal_connect(tool->addressEn, "button-press-event"
        ,G_CALLBACK(c_addr_click), NULL);
    g_signal_connect(tool->backTb, "clicked"
        ,G_CALLBACK(c_go_back), call);
    g_signal_connect(tool->forwardTb, "clicked"
        ,G_CALLBACK(c_go_forward), call);
    g_signal_connect(tool->reloadTb, "clicked"
        ,G_CALLBACK(c_refresh), call->tabs);
    g_signal_connect(call->tabs, "switch-page"
        ,G_CALLBACK(c_switch_tab), call);
	g_signal_connect(call->tabs, "page-added"
		,G_CALLBACK(c_notebook_tabs_changed), call);
	sign->nb_changed = g_signal_connect(call->tabs, "page-removed"
		,G_CALLBACK(c_notebook_tabs_changed), call);
    g_signal_connect_after(tool->addressEn, "insert-text"
        ,G_CALLBACK(c_addr_ins), call);
    g_signal_connect_after(tool->addressEn, "delete-text"
        ,G_CALLBACK(c_addr_del), call);
    g_signal_connect_after(window, "key-release-event"
        ,G_CALLBACK(c_accl_rels), call);
	g_signal_connect(G_OBJECT(call->menu->tabV), "activate"
        ,G_CALLBACK(c_update_tabs_layout), call);
	g_signal_connect(G_OBJECT(call->menu->tabH), "activate"
        ,G_CALLBACK(c_update_tabs_layout), call);
	g_signal_connect(G_OBJECT(call->menu->tabM), "activate"
        ,G_CALLBACK(c_update_tabs_layout), call);

	if(argc > 1 && argv)
    {
        gchar * uri = prepAddress((const gchar *) argv[1]);
        if(uri)
        {
			webkit_web_view_load_uri(WK_CURRENT_TAB(call->tabs), uri);
			free(uri);
			if(argc > 2)
			{
				for(int s = 2; s != argc; s++)
				{
					new_tab_ext((gchar *) argv[s], call, FALSE);
				}
			}
		}
		else
			webkit_web_view_load_uri(WK_CURRENT_TAB(call->tabs)
				,"about:blank");
    }
    else
    {
		gchar * uri = prepAddress(g_settings_get_string
			(G_SETTINGS,"planc-startpage"));
		if(uri)
		{
			webkit_web_view_load_uri(WK_CURRENT_TAB(call->tabs),uri);
			free(uri);
		}
		else
			webkit_web_view_load_uri(WK_CURRENT_TAB(call->tabs)
				,"about:blank");
    }

	gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(call->tabs));
	gtk_widget_show_all(window);
    gtk_widget_hide(GTK_WIDGET(find->top));
}

static void c_app_act(GApplication * app, GApplicationCommandLine * cmd
	,void * v)
{
	gchar **argv;
	gint argc;
    argv = g_application_command_line_get_arguments (cmd, &argc);

    InitWindow(app, argv, argc);
}

static void c_wv_hit(WebKitWebView * wv, WebKitHitTestResult * h
	,guint m, struct call_st * c)
{
	if(c->tool->usrmod)
		return;
	if(webkit_hit_test_result_context_is_link(h))
	{
		if(webkit_hit_test_result_get_link_uri(h))
		{
			gtk_entry_set_text(c->tool->addressEn
				,webkit_hit_test_result_get_link_uri(h));
			c->tool->usrmod = FALSE; //This change isn't done by the usr
		}
	}
	else
	{
		gtk_entry_set_text(c->tool->addressEn
			,webkit_web_view_get_uri(wv));
	}
}

static void c_app_init(GtkApplication * app, void * v)
{
	G_SETTINGS = g_settings_new("priv.dis.planc");
	InitWebContext();
}

int main(int argc, char **argv)
{
	int status;
	G_APP = gtk_application_new("priv.dis.planc"
		,G_APPLICATION_HANDLES_COMMAND_LINE);
	gtk_window_set_default_icon_name("web-browser");

	g_signal_connect(G_APP, "startup", G_CALLBACK(c_app_init)
		,NULL);

	g_signal_connect(G_APP, "command-line", G_CALLBACK(c_app_act)
		,NULL);

	status = g_application_run(G_APPLICATION(G_APP), argc, argv);

	return status;
}

void connect_signals (WebKitWebView * wv, struct call_st * c)
{
    g_signal_connect(wv, "create", G_CALLBACK(c_new_tab_url), c);
    g_signal_connect(wv, "load-changed", G_CALLBACK(c_load), c);
    g_signal_connect(wv, "resource-load-started", G_CALLBACK(c_loads)
        ,c);
    g_signal_connect(wv, "notify::title", G_CALLBACK(c_update_title)
        ,c);
	g_signal_connect(wv, "notify::uri", G_CALLBACK(addrWebviewState)
        ,c);
    g_signal_connect(wv, "ready-to-show", G_CALLBACK(c_show_tab)
        ,c->tabs);
    g_signal_connect(wv, "enter-fullscreen"
        ,G_CALLBACK(c_enter_fullscreen), c);
    g_signal_connect(wv, "leave-fullscreen"
        ,G_CALLBACK(c_leave_fullscreen), c);
    g_signal_connect(wv, "decide-policy", G_CALLBACK(c_policy), c);
    g_signal_connect(wv, "mouse-target-changed"
		,G_CALLBACK(c_wv_hit), c);
    webkit_web_view_set_settings(wv,G_WKC_SETTINGS);
}
