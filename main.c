#include "main.h"
#include "uri.h"
#include "settings.h"
#include "libdatabase.h"
#include "download.h"
#include "history.h"
#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif
#ifdef PLANC_FEATURE_GNOME
#include "gmenu.h"
gboolean G_GMENU; /* Gmenu active this session */
#endif
#ifdef PLANC_FEATURE_DMENU
#include "dmenu.h"
GtkWidget * lasttab = NULL;
#endif
#include <time.h>

GtkApplication * G_APP          = NULL;
GSettings * G_SETTINGS          = NULL;
GtkWindow * G_HISTORY           = NULL;
GtkWindow * G_DOWNLOAD          = NULL;
WebKitSettings * G_WKC_SETTINGS = NULL;
WebKitWebContext * G_WKC        = NULL;
GtkSettings * G_GTK_SETTINGS    = NULL;

static void c_show_tab(WebKitWebView * wv, struct newt_st * newtab);

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na ,PlancWindow * v);

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

void uninhibit_now(struct call_st * c, PlancWindow * v)
{
    if (gtk_window_is_active(GTK_WINDOW(v))
    && webkit_web_view_is_playing_audio(WK_CURRENT_TAB(c->tabs)))
        return;
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
    {
        gchar *cmd;
        gint returncode;
        cmd = g_strdup_printf("xdg-screensaver resume %i"
            ,(int)GDK_WINDOW_XID(gtk_widget_get_window
            (GTK_WIDGET(v))));
        returncode = system(cmd);
        if (returncode)
            g_print("'%s' failed with error %d", cmd
                ,returncode);
#ifndef NDEBUG
        else
            g_debug("Uninhibited: %s", cmd);
#endif
        g_free(cmd);
    }
    else
#endif
    if(c->sign->inhibit)
    {
        gtk_application_uninhibit(G_APP, c->sign->inhibit);
#ifndef NDEBUG
        g_debug("Uninhibited: %d", c->sign->inhibit);
#endif
    }

    c->sign->inhibit = 0;
}

static gboolean uninhibit(PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if (!c->sign->waiter)
        return FALSE;

    uninhibit_now(c, v);
    g_source_remove(c->sign->waiter);
    c->sign->waiter = 0;
    return FALSE;
}

static void update_screensaver_inhibitor(gboolean inhibit
    ,PlancWindow * v)
{
    if(g_settings_get_boolean(G_SETTINGS, "planc-inhibit"))
    {
        struct call_st * c = planc_window_get_call(v);
        if(inhibit && !c->sign->inhibit)
        {
#ifdef GDK_WINDOWING_X11
            if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
            {
                gchar *cmd;
                gint returncode;
                cmd = g_strdup_printf("xdg-screensaver suspend %i"
                    ,(int)GDK_WINDOW_XID(gtk_widget_get_window
                    (GTK_WIDGET(v))));
                returncode = system(cmd);
                if (returncode)
                    g_print("'%s' failed with error %d", cmd
                        ,returncode);
#ifndef NDEBUG
                else
                    g_debug("Inhibiting: %s", cmd);
#endif
                g_free(cmd);
                c->sign->inhibit = 1;
            }
            else
#endif
            {
                c->sign->inhibit = gtk_application_inhibit(G_APP
                    ,GTK_WINDOW(v)
                    ,GTK_APPLICATION_INHIBIT_IDLE, "Audio Playback");
#ifndef NDEBUG
                g_debug("Inhibited: %d", c->sign->inhibit);
#endif
            }
        }
        else if (c->sign->inhibit && !c->sign->waiter)
#ifdef GDK_WINDOWING_X11
            c->sign->waiter = g_timeout_add_seconds_full(G_PRIORITY_LOW
                ,GDK_IS_X11_DISPLAY(gdk_display_get_default()) ? 5 : 10
                ,(GSourceFunc) uninhibit, v, NULL);
#else
            c->sign->waiter = g_timeout_add_seconds_full(G_PRIORITY_LOW
                ,10, (GSourceFunc) uninhibit, v, NULL);
#endif
    }
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

void c_update_tabs_layout(GtkWidget * r, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
#ifdef PLANC_FEATURE_DMENU
    if(r == c->menu->tabM && !c->menu->tabsMh)
    {
        gtk_notebook_set_show_tabs(c->tabs,FALSE);
        t_tabs_menu(v,TRUE);
        return;
    }
    else if(c->menu->tabsMh)
        t_tabs_menu(v,FALSE);
#endif
    if(r == c->menu->tabH)
        gtk_notebook_set_tab_pos(c->tabs,GTK_POS_TOP);
    else if(r == c->menu->tabV)
        gtk_notebook_set_tab_pos(c->tabs,GTK_POS_LEFT);

    if(g_settings_get_boolean(G_SETTINGS,"tab-autohide"))
        c_notebook_tabs_changed(c->tabs,NULL,0,v);
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

void c_notebook_close_current(GtkWidget * w, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(gtk_notebook_get_n_pages(c->tabs) > 1)
        gtk_notebook_remove_page
            (c->tabs,gtk_notebook_get_current_page
            (c->tabs));
}

void c_notebook_tabs_changed(GtkNotebook * nb, GtkWidget * w
    ,guint n, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(g_settings_get_boolean(G_SETTINGS,"tab-autohide")
#ifdef PLANC_FEATURE_DMENU
        && !gtk_check_menu_item_get_active(
        (GtkCheckMenuItem *)c->menu->tabM)
#endif
        )
    {
        if(gtk_notebook_get_n_pages(c->tabs) == 1)
            gtk_notebook_set_show_tabs(c->tabs,FALSE);
        else
            gtk_notebook_set_show_tabs(c->tabs,TRUE);
    }
}

static gboolean c_enter_fullscreen(GtkWidget * widget
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    gtk_widget_hide(GTK_WIDGET(c->menu->menu));
    gtk_widget_hide(GTK_WIDGET(c->tool->top));
    gtk_notebook_set_show_tabs(c->tabs,FALSE);
    return 0;
}

static gboolean c_leave_fullscreen(GtkWidget * widget
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
#ifdef PLANC_FEATURE_GNOME
    if(!G_GMENU
    || g_settings_get_boolean(G_SETTINGS,"planc-traditional"))
#endif
        gtk_widget_show(GTK_WIDGET(c->menu->menu));
    gtk_widget_show(GTK_WIDGET(c->tool->top));
#ifdef PLANC_FEATURE_DMENU
    if(gtk_check_menu_item_get_active(
        (GtkCheckMenuItem *)c->menu->tabM))
        return FALSE;
#endif
    if(g_settings_get_boolean(G_SETTINGS,"tab-autohide"))
        c_notebook_tabs_changed(c->tabs,NULL,0,v);
    else
        gtk_notebook_set_show_tabs(c->tabs,TRUE);
    return FALSE;
}

static gboolean c_policy (WebKitWebView * wv, WebKitPolicyDecision * d
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

void c_toggleSearch(GtkWidget * w, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
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
    ,PlancWindow * v)
{
    c_toggleSearch(w,v);
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
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(webkit_web_view_is_loading(wv))
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"process-stop",GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_entry_set_progress_fraction(c->tool->addressEn
            ,webkit_web_view_get_estimated_load_progress(wv));
        c->tool->usrmod = FALSE;
        return 0;
    }
    if(strcmp(gtk_entry_get_text((GtkEntry *) e)
        ,webkit_web_view_get_uri(wv)) == 0)
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"view-refresh",GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_entry_set_progress_fraction(c->tool->addressEn, 0);
        c->tool->usrmod = FALSE;
        return 1;
    }
    else
    {
        gtk_image_set_from_icon_name(c->tool->reloadIo
            ,"go-last",GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_entry_set_progress_fraction(c->tool->addressEn, 0);
        c->tool->usrmod = TRUE;
        return 2;
    }
}

static char addrEntryState(GtkEditable * e, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    return addrEntryState_webView(e, WK_CURRENT_TAB(c->tabs),v);
}

static void addrWebviewState(WebKitWebView * wv, WebKitLoadEvent evt
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(WK_CURRENT_TAB(c->tabs) == wv)
        addrEntryState_webView((GtkEditable *) c->tool->addressEn
            ,wv, v);
}

static void c_addr_ins(GtkEditable * e, gchar* t, gint l, gpointer p
    ,PlancWindow * v)
{
    addrEntryState(e,v);
}

static void c_addr_del(GtkEditable* e, gint sp, gint ep
    ,PlancWindow * v)
{
    addrEntryState(e,v);
}

static void c_addr_changed(GtkEditable* e, PlancWindow * v)
{
    const char * uri = gtk_entry_get_text((GtkEntry *) e);
    if(!strcmp(uri, "about:blank"))
        gtk_entry_set_text((GtkEntry *) e, "");
}

static void c_refresh(GtkWidget * widget, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(webkit_web_view_is_loading(WK_CURRENT_TAB(c->tabs)))
        webkit_web_view_stop_loading(WK_CURRENT_TAB(c->tabs));
    else
        webkit_web_view_reload(WK_CURRENT_TAB(c->tabs));
}

static void c_go_back(GtkWidget * widget, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    webkit_web_view_go_back(WK_CURRENT_TAB(c->tabs));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(c->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB
        (c->tabs)));
}

static void c_go_forward(GtkWidget * widget, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    webkit_web_view_go_forward(WK_CURRENT_TAB(c->tabs));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(c->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(c->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB
        (c->tabs)));
}

static void c_open_settings(GtkWidget * w, PlancWindow * v)
{
    InitSettingsWindow(v);
}

static void c_open_history(GtkWidget * w, PlancWindow * v)
{
    InitHistoryWindow(v);
}

static void c_open_download(GtkWidget * w, PlancWindow * v)
{
    InitDownloadWindow();
}

void c_zoom_in(GtkWidget * w, void * v)
{
    WebKitWebView * wv
        = (WebKitWebView *) WK_CURRENT_TAB(get_web_view_notebook());
    webkit_web_view_set_zoom_level(wv,
        webkit_web_view_get_zoom_level(wv)+0.05);
}

void c_zoom_out(GtkWidget * w, void * v)
{
    WebKitWebView * wv
        = (WebKitWebView *) WK_CURRENT_TAB(get_web_view_notebook());
    webkit_web_view_set_zoom_level(wv,
            webkit_web_view_get_zoom_level(wv)-0.05);
}

void c_zoom_reset(GtkWidget * w, void * v)
{
    WebKitWebView * wv
        = (WebKitWebView *) WK_CURRENT_TAB(get_web_view_notebook());
    webkit_web_view_set_zoom_level(wv,1);
}

static gboolean c_accl_rels(GtkWidget * w, GdkEvent * e
    ,PlancWindow * v)
{
    guint k;
    guint m;
    if(gdk_event_get_keyval(e,&k))
    {
        struct call_st * c = planc_window_get_call(v);
        if(gdk_event_get_state(e,&m))
        {
            if(m & GDK_CONTROL_MASK) //Control keys
            {
                switch(k)
                {
                case GDK_KEY_Tab:
                        gtk_notebook_next_page(c->tabs);
                        gtk_widget_grab_focus
                            (WK_CURRENT_TAB_WIDGET(c->tabs));
                return TRUE;
                case GDK_KEY_ISO_Left_Tab:
                        gtk_notebook_prev_page(c->tabs);
                        gtk_widget_grab_focus
                            (WK_CURRENT_TAB_WIDGET(c->tabs));
                return TRUE;
                case GDK_KEY_plus:
                case GDK_KEY_KP_Add:
                    c_zoom_in(NULL,NULL);
                return TRUE;
                case GDK_KEY_minus:
                case GDK_KEY_KP_Subtract:
                    c_zoom_out(NULL,NULL);
                return TRUE;
                case GDK_KEY_0:
                case GDK_KEY_KP_0:
                    c_zoom_reset(NULL,NULL);
                return TRUE;
                }
            }
        }
        switch(k)
        {
        case GDK_KEY_F5:
            c_refresh(w,v);
        return TRUE;
        case GDK_KEY_F6:
            if(gtk_widget_is_focus((GtkWidget *) c->tool->addressEn))
                gtk_widget_grab_focus(WK_CURRENT_TAB_WIDGET(c->tabs));
            else
                gtk_widget_grab_focus((GtkWidget *) c->tool->addressEn);
        return TRUE;
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
        return TRUE;
#ifdef PLANC_FEATURE_GNOME
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        if((G_GMENU)
        &&(!g_settings_get_boolean(G_SETTINGS, "planc-traditional"))
        &&(!gtk_menu_shell_get_selected_item(
            GTK_MENU_SHELL(c->menu->menu))))
            gtk_widget_hide(c->menu->menu);
        return FALSE;
#endif
        }
    }
}

void c_active_window(GtkWidget * widget, void * p
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    gboolean b = gtk_window_is_active(GTK_WINDOW(v))
        && webkit_web_view_is_playing_audio(WK_CURRENT_TAB(c->tabs));
    if(b)
        update_screensaver_inhibitor(b, v);
    else if (c->sign->inhibit)
        uninhibit_now(c, v);
}

static void c_act(GtkWidget * widget, PlancWindow * v)
{
    struct call_st * call = planc_window_get_call(v);
    char * curi = NULL;
    switch(addrEntryState((GtkEditable *) call->tool->addressEn, v))
    {
        case 0: //Stop loading current page before requesting new uri
            webkit_web_view_stop_loading
                (WK_CURRENT_TAB(call->tabs));
        break;

        case 1: //Refresh
            webkit_web_view_reload(WK_CURRENT_TAB(call->tabs));
        break;

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

gboolean c_wv_script_dialog (WebKitWebView * wv, WebKitScriptDialog *d
	,PlancWindow * v)
{
	if(webkit_script_dialog_get_dialog_type(d)
	== WEBKIT_SCRIPT_DIALOG_BEFORE_UNLOAD_CONFIRM)
	{
		GtkWidget * w = gtk_widget_get_toplevel(GTK_WIDGET(wv));
		gtk_window_present(GTK_WINDOW(w));
	}
	return false;
}

gboolean c_wv_zoom_wheel(WebKitWebView * wv, GdkEventScroll * e
    ,void * v)
{
    if(e->state == GDK_CONTROL_MASK)
    {
        switch(e->direction)
        {
            case GDK_SCROLL_UP:
            webkit_web_view_set_zoom_level(wv,
                webkit_web_view_get_zoom_level(wv)+0.05);
            return TRUE;
            case GDK_SCROLL_DOWN:
            webkit_web_view_set_zoom_level(wv,
                webkit_web_view_get_zoom_level(wv)-0.05);
            return TRUE;
            case GDK_SCROLL_SMOOTH:
            {
                gdouble x, y;
                if(gdk_event_get_scroll_deltas((GdkEvent *) e,&x,&y))
                {
                    if(x > -1 && x < 1)
                    {
                        if(y > 0)
                            webkit_web_view_set_zoom_level(wv
                                ,webkit_web_view_get_zoom_level
                                (wv)-0.05);
                        else if(y < 0)
                            webkit_web_view_set_zoom_level(wv
                                ,webkit_web_view_get_zoom_level
                                (wv)+0.05);
                        return TRUE;
                    }
                }
            }
            default:
                break;
        }
    }
    return FALSE;
}

static void c_switch_tab(GtkNotebook * nb, GtkWidget * page
    ,guint i, PlancWindow * v)
{
    WebKitWebView * wv = (WebKitWebView *) page;
    struct call_st * call = planc_window_get_call(v);
    update_tab(nb,wv);

    update_win_label((GtkWidget *) v, nb, (GtkWidget *) wv);

    const gchar * url = webkit_web_view_get_uri(wv);
    gtk_entry_set_text(call->tool->addressEn, url);

    if(strcmp(url,"") == 0 || strcmp(url,"about:blank") == 0)
        gtk_widget_grab_focus(GTK_WIDGET(call->tool->addressEn));
    else
        gtk_widget_grab_focus(GTK_WIDGET(wv));

    addrEntryState_webView((GtkEditable *) call->tool->addressEn, wv
        ,v);

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(wv));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(wv));

    update_screensaver_inhibitor
        (webkit_web_view_is_playing_audio(wv), v);
#ifdef PLANC_FEATURE_DMENU
	if(g_settings_get_int(G_SETTINGS, "tab-layout") == 2
	&& (page == lasttab))
	{
		g_application_withdraw_notification(G_APPLICATION(G_APP), "nt");
		lasttab = NULL;
	}
#endif
}

static void c_update_title(WebKitWebView * webv, WebKitLoadEvent evt
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    update_tab((GtkNotebook *) c->tabs,webv);
    if(webv == WK_CURRENT_TAB(c->tabs))
    {
        update_win_label(GTK_WIDGET(v),c->tabs
            ,WK_CURRENT_TAB_WIDGET(c->tabs));

        update_addr_entry(c->tool->addressEn
            ,webkit_web_view_get_uri(webv));
    }
}

static void c_update_audio(WebKitWebView * webv, void * p
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(webv == WK_CURRENT_TAB(c->tabs)
    && gtk_window_is_active(GTK_WINDOW(v)))
    {
        update_screensaver_inhibitor
            (webkit_web_view_is_playing_audio(webv), v);
    }
}

static void c_update_progress(WebKitWebView * webv, void * p
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(webv == WK_CURRENT_TAB(c->tabs))
        gtk_entry_set_progress_fraction(c->tool->addressEn
            ,webkit_web_view_get_estimated_load_progress(webv));
}

static void c_loads(WebKitWebView * wv, WebKitWebResource * res
    ,WebKitURIRequest  *req, gpointer v)
{
    struct call_st * call = planc_window_get_call(v);
    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back
        (WK_CURRENT_TAB(call->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward
        (WK_CURRENT_TAB(call->tabs)));
}


static void c_load(WebKitWebView * webv, WebKitLoadEvent evt
    ,PlancWindow * v)
{
    struct call_st * call = planc_window_get_call(v);
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
            gtk_entry_set_progress_fraction(call->tool->addressEn, 0);
            if(!webkit_web_view_is_ephemeral(webv)
            && strcmp("about:blank",webkit_web_view_get_uri(webv)) != 0
            && strcmp("",webkit_web_view_get_uri(webv)) != 0)
                sql_history_write(webkit_web_view_get_uri(webv)
                    ,webkit_web_view_get_title(webv));
        break;
    }

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->backTb)
        ,webkit_web_view_can_go_back(WK_CURRENT_TAB(call->tabs)));

    gtk_widget_set_sensitive(GTK_WIDGET(call->tool->forwardTb)
        ,webkit_web_view_can_go_forward(WK_CURRENT_TAB(call->tabs)));

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
            update_tab(call->tabs, webv);
        }
        else if (gtk_entry_get_text_length(call->tool->addressEn))
        {
            update_tab(call->tabs, webv);
        }
        else
        {
            update_tab(call->tabs, webv);
        }
    }
}

GtkWidget * InitTabLabel(WebKitWebView * wv, gchar * str
    ,PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
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

#ifdef PLANC_FEATURE_DMENU
static void c_g_close_tab(GSimpleAction * a, GVariant * v, gpointer p)
{
	if(lasttab == NULL)
		return;
	gtk_widget_destroy(lasttab);
	g_application_withdraw_notification(G_APPLICATION(G_APP), "nt");
	lasttab = NULL;
}

static void c_g_show_tab(GSimpleAction * a, GVariant * v, gpointer p)
{
	if(lasttab == NULL)
		return;
	GtkWidget * top = gtk_widget_get_toplevel(lasttab);
	struct call_st * call = planc_window_get_call(PLANC_WINDOW(top));
	gtk_window_present(GTK_WINDOW(top));
	gint i = gtk_notebook_page_num (call->tabs, lasttab);
	if(i != -1)
		gtk_notebook_set_current_page(call->tabs, i);
	g_application_withdraw_notification(G_APPLICATION(G_APP), "nt");
	lasttab = NULL;
}

static void newTabNotify()
{
	GNotification * boop = g_notification_new("New Tab - Plan C");
	g_notification_set_body(boop, "A tab has been opened");
	g_notification_set_icon(boop, g_icon_new_for_string("tab-new"
		,NULL));
	g_notification_add_button(boop, "View Page", "app.switchtab");
	g_notification_add_button(boop, "Close Tab", "app.closetab");
	g_application_send_notification(G_APPLICATION(G_APP), "nt", boop);
}
#endif

static void c_show_tab(WebKitWebView * wv, struct newt_st * newtab)
{
    struct call_st * call = planc_window_get_call(newtab->plan);
    GtkWidget * ebox = InitTabLabel(wv,NULL,newtab->plan);
    gtk_notebook_append_page((GtkNotebook *) call->tabs
        ,GTK_WIDGET(newtab->webv),ebox);
    gtk_widget_show_all(GTK_WIDGET(call->tabs));
    if(newtab->show)
    {
        gtk_notebook_set_current_page
            ((GtkNotebook *) call->tabs
            ,gtk_notebook_page_num
            (call->tabs,(GtkWidget *) newtab->webv));
        if(g_strcmp0
            (webkit_web_view_get_uri(newtab->webv), "about:blank"))
        {
            gtk_widget_grab_focus((GtkWidget *) newtab->webv);
        }
    }

    connect_signals(newtab->webv,newtab->plan);
#ifdef PLANC_FEATURE_DMENU
	if(g_settings_get_int(G_SETTINGS, "tab-layout") == 2
	&& (WK_CURRENT_TAB(call->tabs) != newtab->webv))
	{
		lasttab = (GtkWidget *) newtab->webv;
		newTabNotify(call->tabs, newtab->webv);
	}
#endif
    free(newtab);
}

extern void * new_tab_ext(char * url, PlancWindow * v, gboolean show)
{
    struct call_st * c = planc_window_get_call(v);
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
    else
        return NULL;

    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = wv;
    newtab->plan = v;
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

static gboolean closePrompt(PlancWindow * v)
{
    struct call_st * call = planc_window_get_call(v);
    gint g = gtk_notebook_get_n_pages(call->tabs);
    if(g > 1)
    {
        GtkWidget * dialog = gtk_message_dialog_new
            ((GtkWindow *) v
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
    else
    {
        webkit_web_view_try_close(WK_CURRENT_TAB(call->tabs));
        return TRUE;
    }
    return FALSE;
}

static gboolean c_destroy_window_request(GtkWidget * widget
    ,GdkEvent * e, PlancWindow * v)
{
    return closePrompt(v);
}

static void c_destroy_window(GtkWidget* widget, void * v)
{
    struct call_st * c = planc_window_get_call((PlancWindow *) widget);
    g_signal_handler_disconnect(c->tabs,c->sign->nb_changed);
    gtk_widget_destroy(GTK_WIDGET(c->tabs));
    if(c->sign->waiter)
    {
        g_source_remove(c->sign->waiter);
        c->sign->waiter = 0;
    }
    free(c->menu);
    free(c->find);
    free(c->sign);
    free(c->tool);
    free(c);
}

void c_destroy_window_menu(GtkWidget * widget
    ,PlancWindow * v)
{
    gtk_window_close((GtkWindow *) v);
}

WebKitWebView * c_new_tab(GtkWidget * gw, PlancWindow * v)
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
    newtab->plan = v;
    newtab->show = TRUE;
    g_signal_connect(nt, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    g_signal_emit_by_name(nt,"ready-to-show");
    return nt;
}

static WebKitWebView * c_new_tab_url(WebKitWebView * wv
    ,WebKitNavigationAction * na, PlancWindow * v)
{
    WebKitWebView * nt
        = (WebKitWebView *) webkit_web_view_new_with_context(G_WKC);
    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->plan = v;
    newtab->show = FALSE;

    webkit_web_view_load_request(nt
        ,webkit_navigation_action_get_request(na));

    g_signal_connect(nt, "ready-to-show", G_CALLBACK(c_show_tab)
        ,newtab);
    return nt;
}

static WebKitWebView * c_new_tab_related(WebKitWebView * wv
    ,WebKitNavigationAction * na, PlancWindow * v)
{
    WebKitWebView * nt
        = (WebKitWebView *) webkit_web_view_new_with_related_view(wv);
    struct newt_st * newtab = malloc(sizeof(struct newt_st));
    newtab->webv = nt;
    newtab->plan = v;
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

void InitMenubar(struct menu_st * menu, PlancWindow * v
    ,GtkAccelGroup * accel_group)
{
    struct call_st * c = planc_window_get_call(v);
    //Top Menu
    menu->menu = gtk_menu_bar_new();
    menu->fileMenu = gtk_menu_new();
    menu->editMenu = gtk_menu_new();
    menu->viewMenu = gtk_menu_new();
#ifdef PLANC_FEATURE_DMENU
    menu->gotoMenu = gtk_menu_new();
#endif
    menu->viewTabMenu = gtk_menu_new();
    menu->viewZoomMenu = gtk_menu_new();
    menu->helpMenu = gtk_menu_new();

    menu->fileMh = gtk_menu_item_new_with_mnemonic("_File");
    menu->editMh = gtk_menu_item_new_with_mnemonic("_Edit");
    menu->viewMh = gtk_menu_item_new_with_mnemonic("_View");
#ifdef PLANC_FEATURE_DMENU
    menu->gotoMh = gtk_menu_item_new_with_mnemonic("_Go");
    menu->tabsMh = NULL;
#endif
    menu->helpMh = gtk_menu_item_new_with_mnemonic("_Help");
    menu->tabH = gtk_radio_menu_item_new_with_mnemonic(NULL
        ,"_Horizontal");
    menu->tabV = gtk_radio_menu_item_new_with_mnemonic_from_widget
        ((GtkRadioMenuItem *)menu->tabH, "_Vertical");
#ifdef PLANC_FEATURE_DMENU
    menu->tabM = gtk_radio_menu_item_new_with_mnemonic_from_widget
        ((GtkRadioMenuItem *)menu->tabH, "_Menu");
#endif

    menu->zin = gtk_menu_item_new_with_mnemonic("Zoom _in");
    menu->zout = gtk_menu_item_new_with_mnemonic("Zoom _out");
    menu->znorm = gtk_menu_item_new_with_mnemonic("_Normal size");

    menu->nWinMi = gtk_menu_item_new_with_mnemonic("New _Window");
    menu->nTabMi = gtk_menu_item_new_with_mnemonic("New _Tab");
    menu->cTabMi = gtk_menu_item_new_with_mnemonic("_Close Tab");
    menu->findMi = gtk_menu_item_new_with_mnemonic("_Find");
    menu->setwMi = gtk_menu_item_new_with_mnemonic("_Settings");
    menu->viewZoomMh = gtk_menu_item_new_with_mnemonic("_Zoom");
    menu->viewTabMh = gtk_menu_item_new_with_mnemonic("_Tabs");
    menu->histMi = gtk_menu_item_new_with_mnemonic("_History");
    menu->downMi = gtk_menu_item_new_with_mnemonic("_Downloads");
    menu->quitMi = gtk_menu_item_new_with_mnemonic("_Quit");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->fileMh)
        ,menu->fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->editMh)
        ,menu->editMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->viewMh)
        ,menu->viewMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->viewZoomMh)
        ,menu->viewZoomMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->viewTabMh)
        ,menu->viewTabMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewMenu)
        ,menu->viewZoomMh);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewMenu)
        ,menu->viewTabMh);
#ifdef PLANC_FEATURE_DMENU
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(c->menu->gotoMh)
        ,c->menu->gotoMenu);
#endif

    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabH);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabV);
#ifdef PLANC_FEATURE_DMENU
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewTabMenu),menu->tabM);
#endif
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewZoomMenu)
        ,gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewZoomMenu),menu->zin);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewZoomMenu)
        ,menu->zout);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->viewZoomMenu)
        ,menu->znorm);
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
#ifdef PLANC_FEATURE_DMENU
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->gotoMh);
#endif
    gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->helpMh);
#ifdef PLANC_FEATURE_DMENU
    g_signal_connect(G_OBJECT(menu->gotoMenu), "show"
        ,G_CALLBACK(c_show_gotoMenu), v);

    g_signal_connect(G_OBJECT(menu->gotoMenu), "hide"
        ,G_CALLBACK(c_hide_gotoMenu), v);
#endif
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
        ,G_CALLBACK(c_notebook_close_current), v);

    g_signal_connect(G_OBJECT(menu->findMi), "activate"
        ,G_CALLBACK(c_toggleSearch), v);

    g_signal_connect(G_OBJECT(menu->setwMi), "activate"
        ,G_CALLBACK(c_open_settings), v);

    g_signal_connect(G_OBJECT(menu->nTabMi), "activate"
        ,G_CALLBACK(c_new_tab), v);

    g_signal_connect(G_OBJECT(menu->nWinMi), "activate"
        ,G_CALLBACK(c_new_win), NULL);

    g_signal_connect(G_OBJECT(menu->histMi), "activate"
        ,G_CALLBACK(c_open_history), v);

    g_signal_connect(G_OBJECT(menu->downMi), "activate"
        ,G_CALLBACK(c_open_download), v);

    g_signal_connect(G_OBJECT(menu->quitMi), "activate"
        ,G_CALLBACK(c_destroy_window_menu), v);

    g_signal_connect(G_OBJECT(menu->zin), "activate"
        ,G_CALLBACK(c_zoom_in), NULL);

    g_signal_connect(G_OBJECT(menu->zout), "activate"
        ,G_CALLBACK(c_zoom_out), NULL);

    g_signal_connect(G_OBJECT(menu->znorm), "activate"
        ,G_CALLBACK(c_zoom_reset), NULL);

    c->menu = menu;
    gint g = g_settings_get_int(G_SETTINGS,"tab-layout");
    switch(g)
    {
#ifdef PLANC_FEATURE_DMENU
        case 2:
            gtk_check_menu_item_set_active
                ((GtkCheckMenuItem *)menu->tabM,TRUE);
            t_tabs_menu(v,TRUE);
        break;
#endif
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
    gtk_widget_set_size_request(GTK_WIDGET(tabs),320,240);
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

#ifdef PLANC_FEATURE_DPOLC
static void c_wk_ext_init(WebKitWebContext *context, gpointer user_data)
{
    webkit_web_context_set_web_extensions_directory(context, WKED);
}
#endif

static void ErrorMsg(gchar * title, gchar * message)
{
	GtkWidget * d = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR
		,GTK_BUTTONS_CLOSE, "");
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG (d), message);
	gtk_window_set_title(GTK_WINDOW (d), title);
	gtk_dialog_run(GTK_DIALOG (d));
	gtk_widget_destroy(d);
}

char wkVersionOk(guint major, guint minor, guint micro)
{
	guint j = webkit_get_major_version()
		,n = webkit_get_minor_version()
		,c = webkit_get_micro_version();
	if(j >= major && n >= minor && c >= micro)
		return 1;

	return 0;
}

static void InitWebContext(GtkApplication * app)
{
	char * config = g_build_filename(g_get_user_config_dir()
        ,PACKAGE_NAME, NULL);
    if(!g_file_test(config, G_FILE_TEST_IS_DIR))
    {
        if(g_mkdir(config, S_IRWXU))
		{
			gchar * str = g_strconcat
				("Unable to create configuration directory <b>'"
				,config, "</b>'. Plan C cannot continue.", NULL);
			g_free(config);
			ErrorMsg("Startup Error - Plan C", str);
			g_free(str);
			g_application_quit(G_APPLICATION(app));
			return;
		}
    }
    g_free(config);

    char * datadir = g_build_filename(g_get_user_data_dir()
		,PACKAGE_NAME, NULL);
    char * cachedir = g_build_filename(g_get_user_cache_dir()
		,PACKAGE_NAME, NULL);
    char * cookiedir = g_build_filename(g_get_user_data_dir()
		,PACKAGE_NAME, "cookies",NULL);
    WebKitWebsiteDataManager * d = webkit_website_data_manager_new
        ("base-data-directory", cachedir, "base-cache-directory"
        ,datadir, NULL);

    g_free(datadir);
    g_free(cachedir);

    G_WKC = webkit_web_context_new_with_website_data_manager(d);
#ifdef PLANC_FEATURE_DPOLC
    g_signal_connect(G_WKC, "initialize-web-extensions"
        ,G_CALLBACK(c_wk_ext_init), NULL);
#endif
    if(wkVersionOk(2,26,0) //Shared model deprecated above this version
    || g_settings_get_boolean(G_SETTINGS,"webkit-ppt"))
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

    switch(g_settings_get_int(G_SETTINGS,"webkit-hw"))
    {
        case 1:
            webkit_settings_set_hardware_acceleration_policy
                (G_WKC_SETTINGS
                ,WEBKIT_HARDWARE_ACCELERATION_POLICY_ON_DEMAND);
        break;
        default:
            webkit_settings_set_hardware_acceleration_policy
                (G_WKC_SETTINGS
                ,WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
        break;
        case 2:
            webkit_settings_set_hardware_acceleration_policy
                (G_WKC_SETTINGS
                ,WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
        break;
    }

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

void InitWebview(PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
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
    connect_signals(wv,v);
}

void InitFindBar(struct find_st * f, PlancWindow * v)
{
    struct call_st * call = planc_window_get_call(v);
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
        ,G_CALLBACK(c_toggleSearch), v);
    g_signal_connect(f->findSb, "focus-out-event"
        ,G_CALLBACK(c_toggleSearchEvent), v);
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
}

#ifdef PLANC_FEATURE_GNOME
gboolean preferGmenu()
{
    gboolean g = gtk_application_prefers_app_menu(G_APP);
    if(!g)
    {
#ifdef GDK_WINDOWING_X11
        GdkDisplay * d = gdk_display_get_default();
        if (GDK_IS_X11_DISPLAY(d))
        {
            const gchar * w = gdk_x11_screen_get_window_manager_name
                (gdk_display_get_default_screen(d));
            //Only MutterWM is known to support appmenu properly
            if(w)
            {
                if(strcmp(w, "muttur"))
                    return g;
            }
            GtkSettings * gtksettings = gtk_settings_get_default();
            if(gtksettings)
            {
                gchar * dec;
                g_object_get(gtksettings, "gtk-decoration-layout"
                    ,&dec, NULL);
                gchar * m = strstr(dec, "menu");
                if(m)
                {
                    if(m > dec)
                    {
                        gchar * rm = m-1;
                        if(rm[0] != ':' && rm[0] != ',')
                            goto preferGmenu_end;
                            //Return false but free dec
                    }
                    switch(m[4])
                    {
                        case ':':
                        case ',':
                        case '\0':
                            g = TRUE;
                    }
                }
preferGmenu_end:
                g_free(dec);
            }
        } else //Check for Wayland Gnome3
#endif
        {
            gchar * e = getenv("DESKTOP_SESSION");
            if(e)
            {
                if (!strcmp(e, "gnome"))
                    g = TRUE;
            }
        }
    }
    return g;
}

gboolean c_key_press (GtkWidget * w, GdkEventKey * e
    ,PlancWindow * v)
{
    switch(e->keyval)
    {
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_F10:
        {
            struct call_st * c = planc_window_get_call(v);
            if(!gtk_widget_is_visible(c->menu->menu))
                gtk_widget_show(c->menu->menu);
            return FALSE;
        }
    }
    return FALSE;
}

static void c_tmenu_deactivate(GtkWidget * w, void * v)
{
    if((G_GMENU)
    &&(!g_settings_get_boolean(G_SETTINGS, "planc-traditional")))
        gtk_widget_hide(w);
}
#endif

static gboolean c_addr_click(GtkEditable * w, GdkEventButton * e
    ,void * v)
{
    if(e->button == 1)
    {
        if(e->type == GDK_2BUTTON_PRESS)
        {
            gtk_widget_grab_focus((GtkWidget *) w);
            return TRUE;
        }
    }
    return FALSE;
}

static GdkRectangle * getMonitorResolution(GtkWidget * win)
{
        GdkWindow * gdk = gtk_widget_get_window(win);
        GdkRectangle * res = g_malloc(sizeof(GdkRectangle));
        if(gdk)
                gdk_monitor_get_geometry(gdk_display_get_monitor_at_window
                        (gtk_widget_get_display(win), gdk), res);
        else
        {
                gint x, y;
                GdkDisplay * di = gdk_display_get_default();
                GdkDevice * de = gdk_seat_get_pointer
                        (gdk_display_get_default_seat(di));
                gdk_device_get_position(de, NULL, &x, &y);
                GdkMonitor * m = gdk_display_get_monitor_at_point(di, x, y);
                gdk_monitor_get_geometry(m, res);
        }
        return res;
}

GtkWidget * InitWindow(GApplication * app, gchar ** argv, int argc)
{
    //There's probably a better way to do this with Glib
    struct menu_st * menu = malloc(sizeof(struct menu_st));
    struct tool_st * tool = malloc(sizeof(struct tool_st));
    struct find_st * find = malloc(sizeof(struct find_st));
    struct sign_st * sign = malloc(sizeof(struct sign_st));
    struct call_st * call = malloc(sizeof(struct call_st));

    InitCallback(call,find,menu,tool,sign,NULL);

    PlancWindow * window = planc_window_new((GtkApplication *) app);
    planc_window_set_call(window, call);

    GtkAccelGroup * accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    GdkRectangle * res = getMonitorResolution((GtkWidget *) window);
    gtk_window_set_default_size(GTK_WINDOW(window)
        ,res->width*0.5, res->height*0.5);
    free(res);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_window_maximize(GTK_WINDOW(window));

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    InitMenubar(menu, window, accel_group);
    InitToolbar(tool, accel_group);
    call->tabs = InitNotetab(call);
    InitWebview(window);
    InitFindBar(find, window);

    gtk_box_pack_start(GTK_BOX(vbox), menu->menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tool->top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(call->tabs)
        ,TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), find->top, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy"
        ,G_CALLBACK(c_destroy_window), NULL);
    g_signal_connect(window, "delete-event"
        ,G_CALLBACK(c_destroy_window_request), window);
    g_signal_connect(window, "notify::is-active"
        ,G_CALLBACK(c_active_window), window);
    g_signal_connect(tool->addressEn, "activate"
        ,G_CALLBACK(c_act), window);
    g_signal_connect(tool->addressEn, "button-press-event"
        ,G_CALLBACK(c_addr_click), NULL);
    g_signal_connect(tool->backTb, "clicked"
        ,G_CALLBACK(c_go_back), window);
    g_signal_connect(tool->forwardTb, "clicked"
        ,G_CALLBACK(c_go_forward), window);
    g_signal_connect(tool->reloadTb, "clicked"
        ,G_CALLBACK(c_act), window);
    g_signal_connect(call->tabs, "switch-page"
        ,G_CALLBACK(c_switch_tab), window);
    g_signal_connect(call->tabs, "page-added"
        ,G_CALLBACK(c_notebook_tabs_changed), window);
    sign->nb_changed = g_signal_connect(call->tabs, "page-removed"
        ,G_CALLBACK(c_notebook_tabs_changed), window);
    g_signal_connect_after(tool->addressEn, "insert-text"
        ,G_CALLBACK(c_addr_ins), window);
    g_signal_connect_after(tool->addressEn, "delete-text"
        ,G_CALLBACK(c_addr_del), window);
    g_signal_connect(tool->addressEn, "changed"
        ,G_CALLBACK(c_addr_changed), window);
#ifdef PLANC_FEATURE_GNOME
    if(G_GMENU)
    {
        g_signal_connect(window, "key-press-event"
            ,G_CALLBACK(c_key_press), window);
        g_signal_connect(menu->menu, "deactivate"
            ,G_CALLBACK(c_tmenu_deactivate), window);
    }
#endif
    g_signal_connect(window, "key-release-event"
        ,G_CALLBACK(c_accl_rels), window);
    g_signal_connect(G_OBJECT(call->menu->tabV), "activate"
        ,G_CALLBACK(c_update_tabs_layout), window);
    g_signal_connect(G_OBJECT(call->menu->tabH), "activate"
        ,G_CALLBACK(c_update_tabs_layout), window);
#ifdef PLANC_FEATURE_DMENU
    g_signal_connect(G_OBJECT(call->menu->tabM), "activate"
        ,G_CALLBACK(c_update_tabs_layout), window);
#endif
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
                    new_tab_ext((gchar *) argv[s], window, FALSE);
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
    gtk_widget_show_all(GTK_WIDGET(window));
#ifdef PLANC_FEATURE_GNOME
    if(G_GMENU)
    {
        if(!g_settings_get_boolean(G_SETTINGS, "planc-traditional"))
            gtk_widget_hide(menu->menu);
        else
            gtk_application_window_set_show_menubar
                (GTK_APPLICATION_WINDOW(window) ,FALSE);
    }
#endif
    gtk_widget_hide(GTK_WIDGET(find->top));
    return (GtkWidget *) window;
}

static void c_app_act(GApplication * app, GApplicationCommandLine * cmd
    ,void * v)
{
    gchar **argv;
    gint argc;
    argv = g_application_command_line_get_arguments (cmd, &argc);

    InitWindow(app, argv, argc);
}

static void c_app_exit(GApplication * app, void * v)
{
	if(!G_SETTINGS)
		return;
    time_t t = time(NULL);
    if(t > 31557600)
    {
        gint s = g_settings_get_int(G_SETTINGS, "planc-history-clean");
        switch(s)
        {
            case 1: //Every session
                sql_history_clean(t);
            break;
            case 2: //Every week
                sql_history_clean(t-604800);
            break;
            case 3: //Every fortnight
                sql_history_clean(t-1209600);
            break;
            case 4: //Every month
                sql_history_clean(t-2592000);
            break;
            case 5: //Every year
                sql_history_clean(t-31557600);
            break;
            default:
            break;
        }
    }
}

static void c_wv_hit(WebKitWebView * wv, WebKitHitTestResult * h
    ,guint m, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(c->tool->usrmod
    || gtk_editable_get_selection_bounds
    ((GtkEditable *) c->tool->addressEn,NULL,NULL))
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

static void c_wv_close(WebKitWebView * wv, PlancWindow * v)
{
    struct call_st * c = planc_window_get_call(v);
    if(gtk_notebook_get_n_pages(c->tabs) > 1)
        gtk_widget_destroy(GTK_WIDGET(wv));
    else
        gtk_widget_destroy(GTK_WIDGET(v));
}

static gboolean c_wv_focus(GtkWidget * wv, GdkEvent  * e
    ,PlancWindow * v)
{
    struct call_st * call = planc_window_get_call(v);

    const gchar * url = webkit_web_view_get_uri((WebKitWebView *) wv);
    if(strcmp(url,"") == 0 || strcmp(url,"about:blank") == 0)
    {
        gtk_widget_grab_focus(GTK_WIDGET(call->tool->addressEn));
        return TRUE;
    }
    else
        gtk_widget_grab_focus(GTK_WIDGET(wv));
    return FALSE;
}

#ifdef PLANC_FEATURE_DPOLC
static void initialSetup()
{
    GtkWidget * SetupDialog = gtk_dialog_new_with_buttons
        ("No domain policy rules setup for this user - Plan C"
        ,NULL
        ,0
        ,_("_Whitelist"),   GTK_RESPONSE_OK
        ,_("_Blacklist"),   GTK_RESPONSE_CANCEL
        ,NULL);
    GtkWidget * dbox = gtk_dialog_get_content_area
        (GTK_DIALOG(SetupDialog));
    GtkWidget * l = gtk_label_new(
        "Please select a starting policy:\n\n"
        "Whilelist is ideal for casual use. All requests from the page "
        "will be granted.\n"
        "This is how most web browsers work by default."
        "\n\nBlacklist is ideal for machines where security/privacy "
        "outweighs conivence.\nAll requests from a page "
        "outside the original domain will be denied.\nThis is "
        "very secure but may interfere with loading complex webpages.");
    gtk_box_pack_start(GTK_BOX(dbox),l,0,1,0);
    gtk_widget_show_all(dbox);

    gint res = gtk_dialog_run(GTK_DIALOG(SetupDialog));
    switch(res)
    {
        case GTK_RESPONSE_OK:
            createPolicyDatabase(1);
        break;
        default:
            createPolicyDatabase(0);
        break;
    }
    gtk_widget_destroy(SetupDialog);
}
#endif

GtkWidget * get_web_view()
{
    GtkWidget * w = (GtkWidget *) gtk_application_get_active_window
        (GTK_APPLICATION(G_APP));
    if(!w) //No web view windows are open, make one
        w = InitWindow(G_APPLICATION(G_APP), (gchar**)"about:blank", 1);
    return w;
}

GtkNotebook * get_web_view_notebook()
{
    GtkWidget * w = get_web_view();
    struct call_st * c = planc_window_get_call((PlancWindow *) w);
    return GTK_NOTEBOOK(c->tabs);
}

static void c_app_init(GtkApplication * app, void * v)
{
    G_SETTINGS = g_settings_new("priv.dis.planc");
    G_GTK_SETTINGS = gtk_settings_get_default();
#ifdef PLANC_FEATURE_GNOME
    G_GMENU = preferGmenu();
    if(G_GMENU)
        InitAppMenu();
#endif
#ifdef PLANC_FEATURE_DPOLC
    POLICYDIR(p);
    if(!g_file_test(p, G_FILE_TEST_IS_REGULAR))
    {
        initialSetup();
    }
    g_free(p);
#endif
#ifdef PLANC_FEATURE_DMENU
	static const GActionEntry actions[] =
	{
		{ "switchtab", c_g_show_tab },
		{ "closetab", c_g_close_tab }
	};
	g_action_map_add_action_entries (G_ACTION_MAP (G_APP), actions
		,G_N_ELEMENTS (actions), G_APP);
#endif
    InitWebContext(app);
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

    g_signal_connect(G_APP, "shutdown", G_CALLBACK(c_app_exit)
        ,NULL);

    status = g_application_run(G_APPLICATION(G_APP), argc, argv);

    return status;
}

void connect_signals (WebKitWebView * wv, PlancWindow * v)
{
    g_signal_connect(wv, "create", G_CALLBACK(c_new_tab_related), v);
    g_signal_connect(wv, "load-changed", G_CALLBACK(c_load), v);
    g_signal_connect(wv, "resource-load-started", G_CALLBACK(c_loads)
        ,v);
    g_signal_connect(wv, "notify::title", G_CALLBACK(c_update_title)
        ,v);
    g_signal_connect(wv, "notify::estimated-load-progress"
        ,G_CALLBACK(c_update_progress), v);
    g_signal_connect(wv, "notify::uri", G_CALLBACK(addrWebviewState)
        ,v);
    g_signal_connect(wv, "notify::is-playing-audio"
        ,G_CALLBACK(c_update_audio), v);
    g_signal_connect(wv, "ready-to-show", G_CALLBACK(c_show_tab)
        ,v);
    g_signal_connect(wv, "enter-fullscreen"
        ,G_CALLBACK(c_enter_fullscreen), v);
    g_signal_connect(wv, "leave-fullscreen"
        ,G_CALLBACK(c_leave_fullscreen), v);
    g_signal_connect(wv, "decide-policy", G_CALLBACK(c_policy), v);
    g_signal_connect(wv, "mouse-target-changed"
        ,G_CALLBACK(c_wv_hit), v);
    g_signal_connect(wv, "focus-in-event"
        ,G_CALLBACK(c_wv_focus), v);
    g_signal_connect(wv, "script-dialog"
        ,G_CALLBACK(c_wv_script_dialog), v);
    g_signal_connect(wv, "scroll-event"
        ,G_CALLBACK(c_wv_zoom_wheel), NULL);
    g_signal_connect(wv, "close", G_CALLBACK(c_wv_close), v);
    webkit_web_view_set_settings(wv,G_WKC_SETTINGS);
}
