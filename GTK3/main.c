#include "main.h"

char * prepAddress(const gchar * c)
{
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

void update_win_label(GtkWidget * win, WebKitWebView * wv)
{
    gchar * str;
    str = g_strconcat
        (webkit_web_view_get_title(wv), " - Browser", NULL);
    gtk_window_set_title((GtkWindow *) win, str);
    if(str)
        g_free(str);
}

void update_tab(GtkNotebook * nb, WebKitWebView * ch)
{
    GtkWidget * t = gtk_notebook_get_tab_label(nb,(GtkWidget *) ch);
    GtkWidget * l = gtk_bin_get_child(GTK_BIN(t));
    const gchar * c = webkit_web_view_get_title(ch);
    if(c && strcmp(c,"") != 0)
        gtk_label_set_text((GtkLabel *)l,c);
    else
        gtk_label_set_text((GtkLabel *)l,webkit_web_view_get_uri(ch));
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
        gtk_file_chooser_set_current_name (chooser
            ,("Untitled download"));
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
        return FALSE;
    }
    else
    {
        webkit_download_cancel(d);
    }
    gtk_widget_destroy (dialog);
    return TRUE;
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
        gtk_notebook_remove_page(G_call->webv->tabsNb
            ,gtk_notebook_page_num(G_call->webv->tabsNb,v));

        return TRUE;
    }
    return FALSE;
}

static gboolean c_enter_fullscreen(GtkWidget * widget, void * v)
{
    struct call_st * c = v;
    gtk_widget_hide(GTK_WIDGET(c->menu->menu));
    gtk_widget_hide(GTK_WIDGET(c->tool->top));
    gtk_notebook_set_show_tabs(c->webv->tabsNb,FALSE);
    return 0;
}

static gboolean c_leave_fullscreen(GtkWidget * widget, void * v)
{
    struct call_st * c = v;
    gtk_widget_show(GTK_WIDGET(c->menu->menu));
    gtk_widget_show(GTK_WIDGET(c->tool->top));
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
    return addrEntryState_webView(e, WK_CURRENT_TAB(c->webv->tabsNb)
        ,v);
}

static void c_addr_ins(GtkEditable * e, gchar* t, gint l, gpointer p,
    void * v)
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

static void c_act(GtkWidget * widget, void * v)
{
    struct call_st * call = (struct call_st *) v;
    const gchar * uri = gtk_entry_get_text
        (GTK_ENTRY(call->tool->addressEn));
    char * curi = prepAddress(uri);
    webkit_web_view_load_uri(WK_CURRENT_TAB(call->webv->tabsNb),curi);
    free(curi);
}

static void c_switch_tab(GtkNotebook * nb, GtkWidget * page
    ,guint i, void * v)
{
    WebKitWebView * wv = (WebKitWebView *) page;
    struct call_st * call = v;
    update_tab(nb,wv);

    update_win_label(G_call->twin,(WebKitWebView*) page);

    gtk_entry_set_text(call->tool->addressEn
        ,webkit_web_view_get_uri(wv));

    addrEntryState_webView((GtkEditable *) call->tool->addressEn, wv
        ,call);

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(wv));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(wv));
}

static void c_update_title(WebKitWebView * webv, WebKitLoadEvent evt
    ,void * v)
{
    update_tab((GtkNotebook *) v,webv);
    if(webv == WK_CURRENT_TAB(v))
    {
        update_win_label(G_call->twin,webv);
    }
    gtk_entry_set_text(G_call->tool->addressEn
        ,webkit_web_view_get_uri(webv));
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
        gtk_entry_set_text(call->tool->addressEn
            ,webkit_web_view_get_uri
            (WK_CURRENT_TAB(call->webv->tabsNb)));
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
    gtk_widget_add_accelerator((GtkWidget *) tool->reloadTb
        ,"clicked", accel_group, GDK_KEY_F5, 0, 0);
}

void InitMenubar(struct menu_st * menu, GtkAccelGroup * accel_group)
{
    menu->menu = gtk_menu_bar_new();

    menu->fileMenu = gtk_menu_new();
    menu->editMenu = gtk_menu_new();
    /*menu->viewMenu = gtk_menu_new();
    menu->helpMenu = gtk_menu_new();*/

    menu->fileMh = gtk_menu_item_new_with_mnemonic("_File");
    menu->editMh = gtk_menu_item_new_with_mnemonic("_Edit");
    /*menu->viewMh = gtk_menu_item_new_with_mnemonic("_View");
    menu->helpMh = gtk_menu_item_new_with_mnemonic("_Help");*/
    menu->nTabMi = gtk_menu_item_new_with_mnemonic("New _Tab");
    menu->findMi = gtk_menu_item_new_with_mnemonic("_Search Page");
    menu->quitMi = gtk_menu_item_new_with_mnemonic("_Quit");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->fileMh)
        ,menu->fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->editMh)
        ,menu->editMenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu->editMenu), menu->findMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->nTabMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->fileMh);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->editMh);

    gtk_widget_add_accelerator(menu->findMi, "activate", accel_group,
      GDK_KEY_F, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(menu->nTabMi, "activate", accel_group,
      GDK_KEY_T, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    g_signal_connect(G_OBJECT(menu->findMi), "activate",
        G_CALLBACK(c_toggleSearch), NULL);

    g_signal_connect(G_OBJECT(menu->nTabMi), "activate",
        G_CALLBACK(c_new_tab), G_call);

    g_signal_connect(G_OBJECT(menu->quitMi), "activate",
        G_CALLBACK(destroyWindowCb), NULL);
}

void InitNotetab(struct webt_st * webv)
{
    webv->tabsNb = (GtkNotebook *) gtk_notebook_new();
    gtk_notebook_set_show_border(webv->tabsNb,FALSE);
    gtk_notebook_set_tab_pos(webv->tabsNb,GTK_POS_TOP);
    gtk_notebook_set_scrollable(webv->tabsNb,TRUE);
}

void InitWebview(struct call_st * c)
{
    c->webv->webc = webkit_web_context_get_default();
    webkit_web_context_set_cache_model(c->webv->webc
        ,WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);

    WebKitWebView * wv = WEBKIT_WEB_VIEW(
        webkit_web_view_new_with_context(c->webv->webc));
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

    g_signal_connect(f->backTb, "clicked"
        ,G_CALLBACK(c_search_prv), w);
    g_signal_connect(f->forwardTb, "clicked"
        ,G_CALLBACK(c_search_nxt), w);

    g_signal_connect_after(f->findEn, "insert-text"
        ,G_CALLBACK(c_search_ins), w);
    g_signal_connect_after(f->findEn, "delete-text"
        ,G_CALLBACK(c_search_del), w);
}

void InitCallback(struct call_st * c, struct find_st * f
    ,struct menu_st * m,struct tool_st * t,struct webt_st * w
    ,GtkWidget * x)
{
    c->menu = m;
    c->find = f;
    c->tool = t;
    c->webv = w;
    c->twin = x;
}

int main(int argc, char* argv[])
{
    struct menu_st menu;
    struct tool_st tool;
    struct webt_st webk;
    struct find_st find;
    struct call_st call;

    G_call = &call;

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkAccelGroup * accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    InitMenubar(&menu, accel_group);
    InitToolbar(&tool, accel_group);
    InitNotetab(&webk);
    InitFindBar(&find, webk.tabsNb);
    InitCallback(&call,&find,&menu,&tool,&webk,window);
    InitWebview(&call);

    gtk_box_pack_start(GTK_BOX(vbox), menu.menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tool.top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(webk.tabsNb)
        , TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), find.top, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy"
        ,G_CALLBACK(destroyWindowCb), NULL);
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

    g_signal_connect_after(tool.addressEn, "insert-text"
        ,G_CALLBACK(c_addr_ins), &call);
    g_signal_connect_after(tool.addressEn, "delete-text"
        ,G_CALLBACK(c_addr_del), &call);


    if(argc > 1)
    {
        char * uri = prepAddress((const gchar *) argv[1]);
        webkit_web_view_load_uri(WK_CURRENT_TAB(webk.tabsNb), uri);
        free(uri);
    }
    else
    {
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
        ,c->webv->tabsNb);
    g_signal_connect(wv, "ready-to-show", G_CALLBACK(c_show_tab)
        ,c->webv->tabsNb);
    g_signal_connect(wv, "enter-fullscreen"
        ,G_CALLBACK(c_enter_fullscreen), c);
    g_signal_connect(wv, "leave-fullscreen"
        ,G_CALLBACK(c_leave_fullscreen), c);
    g_signal_connect(wv, "decide-policy", G_CALLBACK(c_policy), c);
}

static void destroyWindowCb(GtkWidget* widget, GtkWidget* window)
{
    gtk_main_quit();
}
