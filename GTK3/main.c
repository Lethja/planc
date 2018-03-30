#include "main.h"

char * prepAddress(const gchar * c)
{
	if(strstr(c,"about:blank"))
		return NULL;
    char * p = strstr(c,"://");
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
    gchar * str;
    str = g_strconcat
        (gtk_label_get_text((GtkLabel *)l), " - Plan C", NULL);
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

static void c_onclick_tabsMi(GtkMenuItem * mi, GdkEvent * e
	,GtkWidget * w)
{
	gtk_notebook_set_current_page(G_call->webv->tabsNb
		,gtk_notebook_page_num(G_call->webv->tabsNb,w));
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
	for(gint i = 0; i < gtk_notebook_get_n_pages(c->webv->tabsNb); i++)
	{
		GtkWidget * l = gtk_bin_get_child(GTK_BIN
			(gtk_notebook_get_tab_label(c->webv->tabsNb
			,gtk_notebook_get_nth_page(c->webv->tabsNb,i))));
		GtkWidget * n = gtk_menu_item_new();
		gtk_menu_item_set_label((GtkMenuItem *)n
			,gtk_label_get_text((GtkLabel* )l));

		g_signal_connect(n,"button-release-event"
			,G_CALLBACK(c_onclick_tabsMi)
			,gtk_notebook_get_nth_page(c->webv->tabsNb,i));
		gtk_menu_shell_append(GTK_MENU_SHELL(c->menu->tabsMenu),n);
	}
	gtk_widget_show_all((GtkWidget *)mi);
}

void t_tabs_menu(struct menu_st * m, gboolean b)
{
	if(b)
	{
		m->tabsMh = gtk_menu_item_new_with_mnemonic("_Tabs");
		gtk_menu_shell_insert(GTK_MENU_SHELL(m->menu)
			,m->tabsMh,3);
		gtk_widget_show_all(m->menu);
		m->tabsMenu = gtk_menu_new();
		gtk_menu_item_set_submenu((GtkMenuItem *) m->tabsMh
			,m->tabsMenu);
		g_signal_connect(m->tabsMh, "select"
			,G_CALLBACK(c_onclick_tabsMh), G_call);
		g_signal_connect_after(m->tabsMh,"deselect"
			,G_CALLBACK(c_onrelease_tabsMh),G_call);
	}
	else
	{
		gtk_container_remove(GTK_CONTAINER(m->menu)
			,m->tabsMh);
		m->tabsMh = NULL;
	}
}

void c_update_tabs_layout(GtkWidget * r, struct call_st * c)
{
	if(r == c->menu->tabM && !c->menu->tabsMh)
	{
		gtk_notebook_set_show_tabs(c->webv->tabsNb,FALSE);
		t_tabs_menu(c->menu,TRUE);
		return;
	}
	else if(c->menu->tabsMh)
		t_tabs_menu(c->menu,FALSE);

	if(r == c->menu->tabH)
		gtk_notebook_set_tab_pos(c->webv->tabsNb,GTK_POS_TOP);
	else if(r == c->menu->tabV)
		gtk_notebook_set_tab_pos(c->webv->tabsNb,GTK_POS_LEFT);

	if(gtk_check_menu_item_get_active(
		(GtkCheckMenuItem *)c->menu->tab1))
		c_notebook_tabs_changed(c->webv->tabsNb,NULL,0,c);
	else
		gtk_notebook_set_show_tabs(c->webv->tabsNb,TRUE);
}

gboolean c_download_name(WebKitDownload * d, gchar * fn, void * v)
{
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Save File" ,NULL
        ,GTK_FILE_CHOOSER_ACTION_SAVE ,("_Cancel")
        ,GTK_RESPONSE_CANCEL ,("_Save") ,GTK_RESPONSE_ACCEPT,NULL);

    chooser = GTK_FILE_CHOOSER (dialog);

    gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

    if (fn == NULL || strcmp(fn,"") == 0)
    {
		WebKitURIResponse * u = webkit_download_get_response(d);
		SoupMessageHeaders * s = NULL;
		char * f = NULL;
		if(u)
			s = webkit_uri_response_get_http_headers(u);
		if(s)
		{
			if(soup_message_headers_get_content_disposition(s,&f,NULL))
			{
				char * a = malloc(strlen(f)+1);
				strncpy(a,f,strlen(f)+1);
				char * b = strrchr (f, '=')+1;
				char * c = strrchr (b, '/');
				if(c)
					b = c+1;
				if (b[0] == '"' || b[0] == '\'')
					b++;
				if((b[strlen(b)-1] == '"') || (b[strlen(b)-1] == '\''))
					b[strlen(b)-1] = '\0';
				gtk_file_chooser_set_current_name (chooser,b);
				free(a);
			}
		}
		else
			gtk_file_chooser_set_current_name(chooser
				,"Untitled download");
		if(f)
			free(f);
	}
    else
        gtk_file_chooser_set_current_name(chooser, fn);

    gtk_file_chooser_set_current_folder(chooser
        ,g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));

    res = gtk_dialog_run (GTK_DIALOG (dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        webkit_download_set_destination(d
            ,gtk_file_chooser_get_uri(chooser));
        gtk_widget_destroy (dialog);
        return TRUE;
    }
    else
    {
        webkit_download_cancel(d);
    }
    gtk_widget_destroy (dialog);
    return FALSE;
}

static void c_download_start(WebKitWebContext * wv
    ,WebKitDownload * dl, void * v)
{
    g_signal_connect(dl, "decide-destination"
        ,G_CALLBACK(c_download_name), v);
}

static gboolean c_notebook_click(GtkWidget * w, GdkEventButton * e
    ,void * v)
{
    if (e->button == 2)
    {
        if(gtk_notebook_get_n_pages(G_call->webv->tabsNb) > 1)
            gtk_notebook_remove_page(G_call->webv->tabsNb
                ,gtk_notebook_page_num(G_call->webv->tabsNb,v));
		return TRUE;
    }
    return FALSE;
}

void c_notebook_close_current(GtkWidget * w, struct call_st * c)
{
	if(gtk_notebook_get_n_pages(c->webv->tabsNb) > 1)
		gtk_notebook_remove_page
			(c->webv->tabsNb,gtk_notebook_get_current_page
			(c->webv->tabsNb));
}

void c_notebook_tabs_autohide(GtkCheckMenuItem * cbmi
	,struct call_st * c)
{
	if(gtk_check_menu_item_get_active(cbmi))
		c_notebook_tabs_changed(c->webv->tabsNb,NULL,0,c);
	else if(!gtk_notebook_get_show_tabs(c->webv->tabsNb))
		gtk_notebook_set_show_tabs(c->webv->tabsNb,TRUE);
}

void c_notebook_tabs_changed(GtkNotebook * nb, GtkWidget * w
	,guint n, struct call_st * c)
{
	if(gtk_check_menu_item_get_active(
		(GtkCheckMenuItem *)c->menu->tab1)
		&& !gtk_check_menu_item_get_active(
		(GtkCheckMenuItem *)c->menu->tabM))
	{
		if(gtk_notebook_get_n_pages(c->webv->tabsNb) == 1)
			gtk_notebook_set_show_tabs(G_call->webv->tabsNb,FALSE);
		else
			gtk_notebook_set_show_tabs(G_call->webv->tabsNb,TRUE);
	}
}

static gboolean c_enter_fullscreen(GtkWidget * widget, void * v)
{
    struct call_st * c = v;
    gtk_widget_hide(GTK_WIDGET(c->menu->menu));
    gtk_widget_hide(GTK_WIDGET(c->tool->top));
    gtk_notebook_set_show_tabs(c->webv->tabsNb,FALSE);
    return 0;
}

static gboolean c_leave_fullscreen(GtkWidget * widget
	,struct call_st * c)
{
    gtk_widget_show(GTK_WIDGET(c->menu->menu));
    gtk_widget_show(GTK_WIDGET(c->tool->top));
    if((gtk_check_menu_item_get_active
		((GtkCheckMenuItem *)c->menu->tab1)))
		c_notebook_tabs_changed(c->webv->tabsNb,NULL,0,c);
	else
		gtk_notebook_set_show_tabs(c->webv->tabsNb,TRUE);
    return 0;
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

static void c_toggleSearch(GtkWidget * w, void * v)
{
    if(gtk_widget_get_visible(G_call->find->top))
    {
        gtk_widget_hide(G_call->find->top);
        webkit_find_controller_search_finish
            (webkit_web_view_get_find_controller
            (WK_CURRENT_TAB(G_call->webv->tabsNb)));
        gtk_widget_grab_focus
            (WK_CURRENT_TAB_WIDGET(G_call->webv->tabsNb));
    }
    else
    {
        gtk_widget_show(G_call->find->top);
        gtk_widget_grab_focus((GtkWidget *) G_call->find->findEn);
    }
}

static void search_page(GtkEditable * w, GtkNotebook * n)
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

static void c_search_ins(GtkEditable * e, gchar* t, gint l, gpointer p,
    GtkNotebook * d)
{
    search_page(e,d);
}

static void c_search_del(GtkEditable* e, gint sp, gint ep
    ,GtkNotebook * d)
{
    search_page(e,d);
}

static char addrEntryState_webView(GtkEditable * e, WebKitWebView * wv
    ,void * v)
{
    struct call_st * c = v;
    if(webkit_web_view_is_loading(wv))
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"process-stop",GTK_ICON_SIZE_SMALL_TOOLBAR);
        return 0;
    }
    if(strcmp(gtk_entry_get_text((GtkEntry *) e)
        ,webkit_web_view_get_uri(wv)) == 0)
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"view-refresh",GTK_ICON_SIZE_SMALL_TOOLBAR);
        return 1;
    }
    else
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"go-last",GTK_ICON_SIZE_SMALL_TOOLBAR);
        return 2;
    }
}

static char addrEntryState(GtkEditable * e, void * v)
{
    struct call_st * c = v;
    return addrEntryState_webView(e, WK_CURRENT_TAB(c->webv->tabsNb),v);
}

static void c_addr_ins(GtkEditable * e, gchar* t, gint l, gpointer p
	,void * v)
{
    addrEntryState(e,v);
}

static void c_addr_del(GtkEditable* e, gint sp, gint ep
    ,void * v)
{
    addrEntryState(e,v);
}

static void c_refresh(GtkWidget * widget, void * v)
{
    if(webkit_web_view_is_loading(WK_CURRENT_TAB(v)))
        webkit_web_view_stop_loading(WK_CURRENT_TAB(v));
    else
        webkit_web_view_reload(WK_CURRENT_TAB(v));
}

static void c_go_back(GtkWidget * widget, void * v)
{
    webkit_web_view_go_back(WK_CURRENT_TAB(v));

    gtk_widget_set_sensitive(GTK_WIDGET(G_call->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(v)));

    gtk_widget_set_sensitive(GTK_WIDGET(G_call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB(v)));
}

static void c_go_forward(GtkWidget * widget, void * v)
{
    webkit_web_view_go_forward(WK_CURRENT_TAB(v));

    gtk_widget_set_sensitive(GTK_WIDGET(G_call->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(v)));

    gtk_widget_set_sensitive(GTK_WIDGET(G_call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB(v)));
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
					gtk_notebook_next_page(c->webv->tabsNb);
			break;
			case GDK_KEY_ISO_Left_Tab:
				if(m & GDK_CONTROL_MASK)
					gtk_notebook_prev_page(c->webv->tabsNb);
			break;
			}
		}
        switch(k)
        {
        case GDK_KEY_F5:
            c_refresh(w,c->webv->tabsNb);
        break;
        case GDK_KEY_F6:
            gtk_widget_grab_focus((GtkWidget *) c->tool->addressEn);
        break;
        }
    }
}

static void c_act(GtkWidget * widget, void * v)
{
    struct call_st * call = (struct call_st *) v;
    char * curi = NULL;// = prepAddress(uri);
    switch(addrEntryState((GtkEditable *) widget,v))
    {
        case 0: //Stop loading current page before requesting new uri
            webkit_web_view_stop_loading
                (WK_CURRENT_TAB(call->webv->tabsNb));

        case 2: //Go
            curi = prepAddress(gtk_entry_get_text
                (GTK_ENTRY(call->tool->addressEn)));
            webkit_web_view_load_uri(WK_CURRENT_TAB(call->webv->tabsNb)
                ,curi);
            if(curi)
                free(curi);
        break;
    }
    gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(call->webv->tabsNb));
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

    addrEntryState_webView((GtkEditable *) call->tool->addressEn, wv
        ,call);

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(wv));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(wv));
}

static void c_update_title(WebKitWebView * webv, WebKitLoadEvent evt
    ,struct call_st * v)
{
    update_tab((GtkNotebook *) v->webv->tabsNb,webv);
    if(webv == WK_CURRENT_TAB(v->webv->tabsNb))
    {
        update_win_label(v->twin,v->webv->tabsNb
            ,WK_CURRENT_TAB_WIDGET(v->webv->tabsNb));

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
        (WK_CURRENT_TAB(call->webv->tabsNb)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward
        (WK_CURRENT_TAB(call->webv->tabsNb)));
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
        break;
    }

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back
        (WK_CURRENT_TAB(call->webv->tabsNb)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward
        (WK_CURRENT_TAB(call->webv->tabsNb)));

    if(webkit_web_view_get_uri(webv))
    {
        if(webv == WK_CURRENT_TAB(call->webv->tabsNb))
        {
            update_addr_entry(call->tool->addressEn
                ,webkit_web_view_get_uri(webv));
        }
    }

    if(!webkit_web_view_get_title(webv))
    {
        if(webkit_web_view_get_uri(webv))
        {
            update_tab(call->webv->tabsNb,webv);
        }
        else if (gtk_entry_get_text_length(call->tool->addressEn))
        {
            update_tab(call->webv->tabsNb,webv);
        }
        else
        {
            update_tab(call->webv->tabsNb,webv);
        }
    }
}

GtkWidget * InitTabLabel(WebKitWebView * wv, gchar * str)
{
    GtkWidget * ebox = gtk_event_box_new();
    gtk_widget_set_has_window(ebox, FALSE);
    g_signal_connect(ebox, "button-press-event"
        ,G_CALLBACK(c_notebook_click), wv);
    GtkWidget * label;
    if(str == NULL || strcmp(str,"") == 0)
        label = gtk_label_new("New Tab");
    else
        label = gtk_label_new(str);

    gtk_container_add(GTK_CONTAINER(ebox), label);
    gtk_label_set_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_max_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_ellipsize((GtkLabel *)label,PANGO_ELLIPSIZE_END);
    gtk_label_set_justify((GtkLabel *)label,GTK_JUSTIFY_LEFT);

    gtk_widget_set_halign(label,GTK_ALIGN_START);
    gtk_widget_set_halign(ebox,GTK_ALIGN_START);
    gtk_widget_show(GTK_WIDGET(label));
    return ebox;
}

static void c_show_tab(WebKitWebView * wv, void * v)
{
    struct newt_st * newtab = v;
    GtkWidget * ebox = InitTabLabel(wv,NULL);
    gtk_notebook_append_page((GtkNotebook *) newtab->call->webv->tabsNb
        ,GTK_WIDGET(newtab->webv),ebox);
    gtk_widget_show_all(GTK_WIDGET(newtab->call->webv->tabsNb));
    gtk_notebook_set_current_page
        ((GtkNotebook *) newtab->call->webv->tabsNb
        ,gtk_notebook_page_num
        (newtab->call->webv->tabsNb,(GtkWidget *) newtab->webv));

    connect_signals(newtab->webv,newtab->call);
    free(newtab);
}

static WebKitWebView * c_new_tab(GtkWidget * gw,struct call_st * c)
{
    WebKitWebView * nt
        = (WebKitWebView *) webkit_web_view_new_with_related_view
        (WK_CURRENT_TAB(c->webv->tabsNb));
    webkit_web_view_load_uri(nt,"about:blank");
    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->call = c;
    g_signal_connect(nt, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    g_signal_emit_by_name(nt,"ready-to-show");
    gtk_widget_grab_focus((GtkWidget *) c->tool->addressEn);
    return nt;
}

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na, struct call_st * c)
{
    WebKitWebView * nt
        = (WebKitWebView *) webkit_web_view_new_with_related_view(wv);
    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->call = c;

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
    gtk_container_add(tool->addressTi, GTK_WIDGET(tool->addressEn));
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top)
        ,GTK_TOOL_ITEM(tool->addressTi), -1);
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(tool->addressTi), TRUE);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(tool->addressTi), TRUE);

    tool->reloadIo = (GtkImage *) gtk_image_new_from_icon_name
        ("view-refresh",GTK_ICON_SIZE_SMALL_TOOLBAR);
    tool->reloadTb = (GtkToolButton *)gtk_tool_button_new
        (GTK_WIDGET(tool->reloadIo),"_Refresh");
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top)
        ,(GtkToolItem *) tool->reloadTb, -1);
}

void InitMenubar(struct menu_st * menu, GtkAccelGroup * accel_group)
{
	//Top Menu
    menu->menu = gtk_menu_bar_new();
    menu->fileMenu = gtk_menu_new();
    menu->editMenu = gtk_menu_new();
    menu->viewMenu = gtk_menu_new();
    menu->viewTabMenu = gtk_menu_new();
    /*menu->helpMenu = gtk_menu_new();*/

    menu->fileMh = gtk_menu_item_new_with_mnemonic("_File");
    menu->editMh = gtk_menu_item_new_with_mnemonic("_Edit");
    menu->viewMh = gtk_menu_item_new_with_mnemonic("_View");
    menu->tabsMh = NULL;
    menu->viewTabMh = gtk_menu_item_new_with_mnemonic("_Tabs");
    menu->tab1 = gtk_check_menu_item_new_with_mnemonic("_Autohide");
    menu->tabH = gtk_radio_menu_item_new_with_mnemonic(NULL
		,"_Horizontal");
    menu->tabV = gtk_radio_menu_item_new_with_mnemonic_from_widget
		((GtkRadioMenuItem *)menu->tabH, "_Vertical");
	menu->tabM = gtk_radio_menu_item_new_with_mnemonic_from_widget
		((GtkRadioMenuItem *)menu->tabH, "_Menu");
    /*menu->helpMh = gtk_menu_item_new_with_mnemonic("_Help");*/
    menu->nTabMi = gtk_menu_item_new_with_mnemonic("New _Tab");
    menu->cTabMi = gtk_menu_item_new_with_mnemonic("_Close Tab");
    menu->findMi = gtk_menu_item_new_with_mnemonic("_Find");
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
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tab1);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu)
		,gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabH);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabV);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabM);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->editMenu), menu->findMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->nTabMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu)
		,gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->cTabMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->fileMh);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->editMh);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->viewMh);
    /*gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewMh)
		,menu->viewTabMh);*/

	gtk_widget_add_accelerator(menu->cTabMi, "activate", accel_group
		,GDK_KEY_W, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(menu->findMi, "activate", accel_group
		,GDK_KEY_F, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(menu->nTabMi, "activate", accel_group
		,GDK_KEY_T, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    g_signal_connect(G_OBJECT(menu->cTabMi), "activate"
		,G_CALLBACK(c_notebook_close_current), G_call);

    g_signal_connect(G_OBJECT(menu->findMi), "activate"
		,G_CALLBACK(c_toggleSearch), G_call);

    g_signal_connect(G_OBJECT(menu->nTabMi), "activate"
		,G_CALLBACK(c_new_tab), G_call);

    g_signal_connect(G_OBJECT(menu->quitMi), "activate"
		,G_CALLBACK(destroyWindowCb), G_call);

	gint g = g_settings_get_int(G_SETTINGS,"tab-layout");
	switch(g)
	{
		case 2:
			gtk_check_menu_item_set_active
				((GtkCheckMenuItem *)menu->tabM,TRUE);
			t_tabs_menu(menu,TRUE);
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
	gboolean b = g_settings_get_boolean(G_SETTINGS,"tab-autohide");
	if(b)
		gtk_check_menu_item_set_active
			((GtkCheckMenuItem *)menu->tab1,TRUE);
}

void InitNotetab(struct webt_st * webv)
{
    webv->tabsNb = (GtkNotebook *) gtk_notebook_new();
    gtk_notebook_set_show_border(webv->tabsNb,FALSE);
    gint g = g_settings_get_int(G_SETTINGS,"tab-layout");
    switch(g)
    {
		case 2:
			gtk_notebook_set_show_tabs(webv->tabsNb,FALSE);
			break;
		case 1:
			gtk_notebook_set_tab_pos(webv->tabsNb,GTK_POS_LEFT);
			break;
		default:
			gtk_notebook_set_tab_pos(webv->tabsNb,GTK_POS_TOP);
	}
	gboolean b = g_settings_get_boolean(G_SETTINGS,"tab-autohide");
	if(b)
		gtk_notebook_set_show_tabs(webv->tabsNb,FALSE);
    gtk_notebook_set_scrollable(webv->tabsNb,TRUE);
}

void InitWebview(struct call_st * c)
{
    char * cd;
    char * dd;
    char * cs;

    cd = g_build_filename(g_get_user_data_dir(),g_get_prgname(),NULL);
    dd = g_build_filename(g_get_user_cache_dir(),g_get_prgname(),NULL);
    cs = g_build_filename(g_get_user_data_dir(),g_get_prgname()
        ,"cookies",NULL);
    WebKitWebsiteDataManager * d = webkit_website_data_manager_new
        ("base-data-directory", dd, "base-cache-directory", cd, NULL);

    g_free(cd);
    g_free(dd);

    c->webv->webc = webkit_web_context_new_with_website_data_manager(d);
    webkit_web_context_set_cache_model(c->webv->webc
        ,WEBKIT_CACHE_MODEL_WEB_BROWSER);

    webkit_web_context_set_spell_checking_enabled(c->webv->webc,TRUE);
    webkit_web_context_set_spell_checking_languages(c->webv->webc
        ,g_get_language_names());

    WebKitCookieManager * cm
        = webkit_web_context_get_cookie_manager(c->webv->webc);

    webkit_cookie_manager_set_persistent_storage(cm, cs
        ,WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    webkit_cookie_manager_set_accept_policy(cm
        ,WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);

    g_free(cs);

    c->webv->webs = webkit_settings_new();
    webkit_settings_set_enable_mediasource(c->webv->webs, TRUE);

    WebKitWebView * wv = WEBKIT_WEB_VIEW
        (webkit_web_view_new_with_context(c->webv->webc));

    GtkWidget * ebox = gtk_event_box_new();
    gtk_widget_set_has_window(ebox, FALSE);
    GtkWidget * label = gtk_label_new("New Tab");
    gtk_container_add(GTK_CONTAINER(ebox), label);
    g_signal_connect(ebox, "button-press-event"
        ,G_CALLBACK(c_notebook_click), wv);
    gtk_label_set_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_max_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_ellipsize((GtkLabel *)label,PANGO_ELLIPSIZE_END);
    gtk_widget_show(GTK_WIDGET(label));
    gtk_notebook_append_page(c->webv->tabsNb,GTK_WIDGET(wv),ebox);
    connect_signals(wv,c);
}

void InitFindBar(struct find_st * f, GtkNotebook * w)
{
    f->top = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(f->top), GTK_TOOLBAR_ICONS);

    f->findSb = (GtkSearchBar *)gtk_search_bar_new();
    gtk_search_bar_set_show_close_button(f->findSb,TRUE);
    f->findEn = (GtkEntry *) gtk_entry_new();
    gtk_widget_set_hexpand ((GtkWidget *) f->findEn, TRUE);

    gtk_search_bar_connect_entry (GTK_SEARCH_BAR (f->findSb)
        ,GTK_ENTRY (f->findEn));
    GtkContainer * c = (GtkContainer *) gtk_tool_item_new();

    gtk_container_add(c, GTK_WIDGET(f->findEn));
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
        ,G_CALLBACK(c_search_prv), w);
    g_signal_connect(f->forwardTb, "clicked"
        ,G_CALLBACK(c_search_nxt), w);
	g_signal_connect(f->closeTb, "clicked"
        ,G_CALLBACK(c_toggleSearch), w);
    g_signal_connect_after(f->findEn, "insert-text"
        ,G_CALLBACK(c_search_ins), w);
    g_signal_connect_after(f->findEn, "delete-text"
        ,G_CALLBACK(c_search_del), w);
}

void InitCallback(struct call_st * c, struct find_st * f
    ,struct menu_st * m, struct tool_st * t, struct webt_st * w
    ,struct sign_st * s, GtkWidget * x, GtkApplication * a
    ,GSettings * g)
{
    c->menu = m;
    c->find = f;
    c->sign = s;
    c->tool = t;
    c->webv = w;
    c->twin = x;
    c->gApp = a;
    c->gSet = g;
}

int main(int argc, char* argv[])
{
    struct menu_st menu;
    struct tool_st tool;
    struct webt_st webk;
    struct find_st find;
    struct sign_st sign;
    struct call_st call;

    G_call = &call;

    gtk_init(&argc, &argv);
	G_APP = gtk_application_new("priv.dis.planc"
		,G_APPLICATION_FLAGS_NONE);

	G_SETTINGS = g_settings_new("priv.dis.planc");

	gtk_window_set_default_icon_name("web-browser");
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GtkAccelGroup * accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    GdkRectangle *res = malloc(sizeof(GdkRectangle));
    gdk_monitor_get_geometry
		(gdk_display_get_primary_monitor
		(gdk_display_get_default()),res);
    gtk_window_set_default_size(GTK_WINDOW(window)
		,res->width*0.5,res->height*0.5);
	free(res);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    InitMenubar(&menu, accel_group);
    InitToolbar(&tool, accel_group);
    InitNotetab(&webk);
    InitFindBar(&find, webk.tabsNb);
    InitCallback(&call,&find,&menu,&tool,&webk,&sign
		,window,G_APP,G_SETTINGS);
    InitWebview(&call);

    gtk_box_pack_start(GTK_BOX(vbox), menu.menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tool.top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(webk.tabsNb)
        , TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), find.top, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy"
        ,G_CALLBACK(destroyWindowCb), &call);
    g_signal_connect(tool.addressEn, "activate"
        ,G_CALLBACK(c_act), &call);
    g_signal_connect(tool.backTb, "clicked"
        ,G_CALLBACK(c_go_back), webk.tabsNb);
    g_signal_connect(tool.forwardTb, "clicked"
        ,G_CALLBACK(c_go_forward), webk.tabsNb);
    g_signal_connect(tool.reloadTb, "clicked"
        ,G_CALLBACK(c_refresh), webk.tabsNb);
    g_signal_connect(webk.tabsNb, "switch-page"
        ,G_CALLBACK(c_switch_tab), &call);
    g_signal_connect(webk.webc, "download-started"
        ,G_CALLBACK(c_download_start), &call);
	g_signal_connect(webk.tabsNb, "page-added"
		,G_CALLBACK(c_notebook_tabs_changed), &call);
	sign.nb_changed = g_signal_connect(webk.tabsNb, "page-removed"
		,G_CALLBACK(c_notebook_tabs_changed), &call);
    g_signal_connect_after(tool.addressEn, "insert-text"
        ,G_CALLBACK(c_addr_ins), &call);
    g_signal_connect_after(tool.addressEn, "delete-text"
        ,G_CALLBACK(c_addr_del), &call);
    g_signal_connect_after(window, "key-release-event"
        ,G_CALLBACK(c_accl_rels), &call);
	g_signal_connect(G_OBJECT(call.menu->tabV), "activate"
        ,G_CALLBACK(c_update_tabs_layout), &call);
	g_signal_connect(G_OBJECT(call.menu->tabH), "activate"
        ,G_CALLBACK(c_update_tabs_layout), &call);
	g_signal_connect(G_OBJECT(call.menu->tabM), "activate"
        ,G_CALLBACK(c_update_tabs_layout), &call);
	g_signal_connect(call.menu->tab1, "activate"
		,G_CALLBACK(c_notebook_tabs_autohide), &call);

    if(argc > 1)
    {
        gchar * uri = prepAddress((const gchar *) argv[1]);
        webkit_web_view_load_uri(WK_CURRENT_TAB(webk.tabsNb), uri);
        free(uri);
    }
    else
    {
		gchar * uri = prepAddress(g_settings_get_string
			(G_SETTINGS,"start-page"));
		if(uri)
		{
			webkit_web_view_load_uri(WK_CURRENT_TAB(webk.tabsNb),uri);
			free(uri);
		}
		else
			webkit_web_view_load_uri(WK_CURRENT_TAB(webk.tabsNb)
				,"about:blank");
    }

    gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(webk.tabsNb));

    gtk_widget_show_all(window);
    gtk_widget_hide(GTK_WIDGET(find.top));
    gtk_main();

    return 0;
}

void connect_signals (WebKitWebView * wv, struct call_st * c)
{
    g_signal_connect(wv, "create", G_CALLBACK(c_new_tab_url), c);
    g_signal_connect(wv, "load-changed", G_CALLBACK(c_load), c);
    g_signal_connect(wv, "resource-load-started", G_CALLBACK(c_loads)
        ,c);
    g_signal_connect(wv, "notify::title", G_CALLBACK(c_update_title)
        ,c);
    g_signal_connect(wv, "ready-to-show", G_CALLBACK(c_show_tab)
        ,c->webv->tabsNb);
    g_signal_connect(wv, "enter-fullscreen"
        ,G_CALLBACK(c_enter_fullscreen), c);
    g_signal_connect(wv, "leave-fullscreen"
        ,G_CALLBACK(c_leave_fullscreen), c);
    g_signal_connect(wv, "decide-policy", G_CALLBACK(c_policy), c);
    webkit_web_view_set_settings(wv,c->webv->webs);
}

static void destroyWindowCb(GtkWidget* widget, struct call_st * c)
{
	g_signal_handler_disconnect(c->webv->tabsNb,c->sign->nb_changed);
    gtk_main_quit();
}
