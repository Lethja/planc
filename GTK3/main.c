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

void update_tab(GtkNotebook * nb, GtkWidget * ch, const gchar * str)
{
    GtkWidget * t = gtk_notebook_get_tab_label(nb,ch);
    GtkWidget * l = gtk_bin_get_child(GTK_BIN(t));
    gtk_label_set_text((GtkLabel *)l,str);
}

static gboolean c_notebook_click(GtkWidget * w, GdkEventButton * e, void * v)
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
    gtk_widget_hide(GTK_WIDGET(c->tool->top));
    gtk_notebook_set_show_tabs(c->webv->tabsNb,FALSE);
    return 0;
}

static gboolean c_leave_fullscreen(GtkWidget * widget, void * v)
{
    struct call_st * c = v;
    gtk_widget_show(GTK_WIDGET(c->tool->top));
    gtk_notebook_set_show_tabs(c->webv->tabsNb,TRUE);
    return 0;
}


static gboolean c_policy (WebKitWebView *web_view
    ,WebKitPolicyDecision *decision, WebKitPolicyDecisionType type
    ,void * v)
{
    switch (type) {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
        
        if(webkit_navigation_action_get_mouse_button
            (webkit_navigation_policy_decision_get_navigation_action
            ((WebKitNavigationPolicyDecision *)decision)) == 2)
        {
            WebKitWebView * nt = c_new_tab(web_view
                ,webkit_navigation_policy_decision_get_navigation_action
                ((WebKitNavigationPolicyDecision *) decision),v);
            
            webkit_web_view_load_request(nt
                ,webkit_navigation_action_get_request
                (webkit_navigation_policy_decision_get_navigation_action
                    ((WebKitNavigationPolicyDecision *)decision)));
            
            g_signal_emit_by_name(nt,"ready-to-show");
            webkit_policy_decision_ignore(decision);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void c_refresh(GtkWidget * widget, void * v)
{
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
    const gchar * uri = gtk_entry_get_text(GTK_ENTRY(call->tool->addressEn));
    char * curi = prepAddress(uri);
    webkit_web_view_load_uri(WK_CURRENT_TAB(call->webv->tabsNb),curi);
    free(curi);
}

static void c_switch_tab(GtkNotebook * nb, GtkWidget * page
    ,guint i, void * v)
{
    WebKitWebView * wv = (WebKitWebView *) page;
    struct call_st * call = v;
    update_tab(nb,GTK_WIDGET(wv),webkit_web_view_get_title(wv));

    gtk_entry_set_text(call->tool->addressEn
        ,webkit_web_view_get_uri(wv));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(wv));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(wv));
}

static void c_update_title(WebKitWebView * webv, WebKitLoadEvent evt, void * v)
{
    update_tab((GtkNotebook *) v,GTK_WIDGET(webv)
        ,webkit_web_view_get_title(webv));
    gtk_entry_set_text(G_call->tool->addressEn
        ,webkit_web_view_get_uri(webv));
}


static void c_load(WebKitWebView * webv, WebKitLoadEvent evt ,void * v)
{
    struct call_st * call = (struct call_st *) v;
    switch(evt)
    {
        case WEBKIT_LOAD_STARTED:
        case WEBKIT_LOAD_COMMITTED:
        case WEBKIT_LOAD_REDIRECTED:
            gtk_tool_button_set_icon_name(call->tool->reloadTb
            ,"process-stop");
        break;

        case WEBKIT_LOAD_FINISHED:
            gtk_tool_button_set_icon_name(call->tool->reloadTb
            ,"view-refresh");
        break;
    }

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(call->webv->tabsNb)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB(call->webv->tabsNb)));

    if(webkit_web_view_get_uri(webv))
    {
        gtk_entry_set_text(call->tool->addressEn
            ,webkit_web_view_get_uri(WK_CURRENT_TAB(call->webv->tabsNb)));
    }

    if(!webkit_web_view_get_title(webv))
    {
        if(webkit_web_view_get_uri(webv))
        {
            update_tab(call->webv->tabsNb,GTK_WIDGET(webv)
                ,webkit_web_view_get_uri(webv));
        }
        else if (gtk_entry_get_text_length(call->tool->addressEn))
        {
            update_tab(call->webv->tabsNb,GTK_WIDGET(webv)
                ,gtk_entry_get_text(call->tool->addressEn));
        }
        else
        {
            update_tab(call->webv->tabsNb,GTK_WIDGET(webv),"New Tab");
        }
    }
}

GtkWidget * InitTabLabel(WebKitWebView * wv, gchar * str)
{
    GtkWidget * ebox = gtk_event_box_new();
    gtk_widget_set_has_window(ebox, FALSE);
    g_signal_connect(ebox, "button-press-event", G_CALLBACK(c_notebook_click), wv);
    GtkWidget * label;
    if(str == NULL || strcmp(str,"") == 0)
        label = gtk_label_new("New Tab");
    else
        label = gtk_label_new(str);

    gtk_container_add(GTK_CONTAINER(ebox), label);
    gtk_label_set_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_max_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_ellipsize((GtkLabel *)label,PANGO_ELLIPSIZE_END);
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
    connect_signals(newtab->webv,newtab->call);
    free(newtab);
}

static WebKitWebView * c_new_tab(WebKitWebView * wv, WebKitNavigationAction * na
    ,struct call_st * c)
{
    WebKitWebView * nt = (WebKitWebView *) webkit_web_view_new_with_related_view(wv);
    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->call = c;
    g_signal_connect(nt, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    return nt;
}

void InitToolbar(struct tool_st * tool)
{
    tool->top = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(tool->top), GTK_TOOLBAR_ICONS);

    tool->backTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("go-previous"
            ,GTK_ICON_SIZE_SMALL_TOOLBAR)
        ,"_Back");
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top), (GtkToolItem *) tool->backTb, -1);
    gtk_widget_set_sensitive(GTK_WIDGET(tool->backTb), FALSE);

    tool->forwardTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("go-next"
            ,GTK_ICON_SIZE_SMALL_TOOLBAR)
        ,"_Forward");
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top), (GtkToolItem *) tool->forwardTb, -1);
    gtk_widget_set_sensitive(GTK_WIDGET(tool->forwardTb), FALSE);
    tool->addressTi = (GtkContainer *) gtk_tool_item_new();
    tool->addressEn = (GtkEntry *) gtk_entry_new();
    gtk_entry_set_text(tool->addressEn,"about:blank");
    gtk_container_add(tool->addressTi, GTK_WIDGET(tool->addressEn));
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top), GTK_TOOL_ITEM(tool->addressTi), -1);
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(tool->addressTi), TRUE);
    gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(tool->addressTi), TRUE);

    tool->reloadTb = (GtkToolButton *)gtk_tool_button_new
        (gtk_image_new_from_icon_name("view-refresh"
            ,GTK_ICON_SIZE_SMALL_TOOLBAR)
        ,"_Refresh");
    gtk_toolbar_insert(GTK_TOOLBAR(tool->top), (GtkToolItem *) tool->reloadTb, -1);
}

void InitMenubar(struct menu_st * menu)
{
    menu->menu = gtk_menu_bar_new();
    menu->fileMenu = gtk_menu_new();

    menu->fileMi = gtk_menu_item_new_with_mnemonic("_File");
    menu->quitMi = gtk_menu_item_new_with_mnemonic("_Quit");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->fileMi), menu->fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->fileMi);
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
    WebKitWebView * wv = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GtkWidget * ebox = gtk_event_box_new();
    gtk_widget_set_has_window(ebox, FALSE);
    GtkWidget * label = gtk_label_new("New Tab");
    gtk_container_add(GTK_CONTAINER(ebox), label);
    g_signal_connect(ebox, "button-press-event", G_CALLBACK(c_notebook_click), wv);
    gtk_label_set_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_max_width_chars((GtkLabel *)label,WK_TAB_CHAR_LEN);
    gtk_label_set_ellipsize((GtkLabel *)label,PANGO_ELLIPSIZE_END);
    gtk_widget_show(GTK_WIDGET(label));
    gtk_notebook_append_page(c->webv->tabsNb,GTK_WIDGET(wv),ebox);
    connect_signals(wv,c);
}

void InitCallback(struct call_st * c
    ,struct menu_st * m,struct tool_st * t,struct webt_st * w, GtkWidget * x)
{
    c->menu = m;
    c->tool = t;
    c->webv = w;
    c->twin = x;
}

int main(int argc, char* argv[])
{
    struct menu_st menu;
    struct tool_st tool;
    struct webt_st webk;
    struct call_st call;

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    InitMenubar(&menu);
    InitToolbar(&tool);
    InitNotetab(&webk);
    InitCallback(&call,&menu,&tool,&webk,window);
    InitWebview(&call);
    G_call = &call;
    gtk_box_pack_start(GTK_BOX(vbox), menu.menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tool.top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(webk.tabsNb), TRUE, TRUE, 0);

    g_signal_connect(window, "destroy", G_CALLBACK(destroyWindowCb), NULL);
    g_signal_connect(tool.addressEn, "activate", G_CALLBACK(c_act), &call);
    g_signal_connect(tool.backTb, "clicked", G_CALLBACK(c_go_back), webk.tabsNb);
    g_signal_connect(tool.forwardTb, "clicked", G_CALLBACK(c_go_forward), webk.tabsNb);
    g_signal_connect(tool.reloadTb, "clicked", G_CALLBACK(c_refresh), webk.tabsNb);
    g_signal_connect(webk.tabsNb, "switch-page", G_CALLBACK(c_switch_tab), &call);


    if(argv[1])
    {
        char * uri = prepAddress((const gchar *) argv[1]);
        webkit_web_view_load_uri(WK_CURRENT_TAB(webk.tabsNb), uri);
        free(uri);
    }

    gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(webk.tabsNb));


    gtk_widget_show_all(window);
    gtk_widget_hide(menu.menu);

    gtk_main();

    return 0;
}

void connect_signals (WebKitWebView * wv, struct call_st * c)
{
    g_signal_connect(wv, "create", G_CALLBACK(c_new_tab), c);
    g_signal_connect(wv, "load-changed", G_CALLBACK(c_load), c);
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
