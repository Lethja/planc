#include "main.h"
#include "settings.h"
#include "plan.h"

GtkWindow * G_WIN_SETTINGS = NULL;

void c_notebook_tabs_autohide(GtkToggleButton * cbmi
	,void * v)
{
	gboolean setting = gtk_toggle_button_get_active(cbmi);
	GList * wins = gtk_application_get_windows(G_APP);
	while(wins)
	{
		struct call_st * c = planc_window_get_call
			(PLANC_WINDOW(wins->data));
		if(setting)
		{
			if(gtk_notebook_get_n_pages(c->tabs) == 1)
				gtk_notebook_set_show_tabs(c->tabs,FALSE);
			else
				gtk_notebook_set_show_tabs(c->tabs,TRUE);
		}
		else if(!gtk_notebook_get_show_tabs(c->tabs))
			gtk_notebook_set_show_tabs(c->tabs,TRUE);
		wins = wins->next;
	}
}
#ifdef PLANC_FEATURE_GNOME
void c_traditional_menu_hide(GtkToggleButton * cbmi
	,void * v)
{
	gboolean setting = gtk_toggle_button_get_active(cbmi)
		|| !preferGmenu();
	GList * wins = gtk_application_get_windows(G_APP);
	while(wins)
	{
		struct call_st * c = planc_window_get_call
			(PLANC_WINDOW(wins->data));
		if(setting)
			gtk_widget_show(GTK_WIDGET(c->menu->menu));
		else
			gtk_widget_hide(GTK_WIDGET(c->menu->menu));
		wins = wins->next;
	}
}
#endif
void c_settings_jv(GtkToggleButton * w, void * v)
{
	webkit_settings_set_enable_java(G_WKC_SETTINGS
		,gtk_toggle_button_get_active(w));
}

void c_settings_js(GtkToggleButton * w, void * v)
{
	webkit_settings_set_enable_javascript(G_WKC_SETTINGS
		,gtk_toggle_button_get_active(w));
}

void c_settings_mse(GtkToggleButton * w, void * v)
{
	webkit_settings_set_enable_mediasource(G_WKC_SETTINGS
		,gtk_toggle_button_get_active(w));
}

void c_settings_in(GtkToggleButton * w, void * v)
{
	webkit_settings_set_enable_plugins(G_WKC_SETTINGS
		,gtk_toggle_button_get_active(w));
}

void c_settings_ch(GtkToggleButton * w, void * v)
{
	webkit_settings_set_enable_page_cache(G_WKC_SETTINGS
		,gtk_toggle_button_get_active(w));
}

void c_settings_dv(GtkToggleButton * w, void * v)
{
	g_object_set(G_OBJECT(G_WKC_SETTINGS), "enable-developer-extras"
		,gtk_toggle_button_get_active(w), NULL);
}

void c_settings_cm(GtkComboBox * w, void * v)
{
	switch(gtk_combo_box_get_active(w))
	{
		case 0:
			webkit_web_context_set_cache_model(G_WKC
				,WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
			g_settings_set_int(G_SETTINGS,"webkit-mcm",0);
		break;
		default:
			webkit_web_context_set_cache_model(G_WKC
				,WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER);
			g_settings_set_int(G_SETTINGS,"webkit-mcm",1);
		break;
		case 2:
			webkit_web_context_set_cache_model(G_WKC
				,WEBKIT_CACHE_MODEL_WEB_BROWSER);
			g_settings_set_int(G_SETTINGS,"webkit-mcm",2);
		break;
	}
}

void c_settings_hw(GtkComboBox * w, void * v)
{
	switch(gtk_combo_box_get_active(w))
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
}

GtkWidget * InitComboBoxLabel(const gchar * l, GtkWidget * c)
{
	GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget * cbl = gtk_label_new(l);
	gtk_container_add((GtkContainer *)hbox, cbl);
	if(!c)
		c = gtk_combo_box_new();
	gtk_container_add((GtkContainer *)hbox, c);
	return hbox;
}

GtkWindow * InitSettingsWindow(PlancWindow * v)
{
	GtkWindow * r = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(r,400,400);
	gtk_window_set_position(r,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(r,"preferences-system-network");
	gtk_window_set_title(r,"Settings - Plan C");
	GtkWidget * scrl = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_min_content_width
		(GTK_SCROLLED_WINDOW(scrl),320);
	gtk_scrolled_window_set_min_content_height
		(GTK_SCROLLED_WINDOW(scrl),240);
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add((GtkContainer *)r, scrl);
    gtk_container_add((GtkContainer *)scrl, vbox);

    //Init
    GtkWidget * tabBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)tabBox
		,"Horizontal");
	gtk_combo_box_text_append_text((GtkComboBoxText *)tabBox
		,"Vertical");
	gtk_combo_box_text_append_text((GtkComboBoxText *)tabBox
		,"Menu");

	GtkWidget * mcmBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)mcmBox
		,"None");
	gtk_combo_box_text_append_text((GtkComboBoxText *)mcmBox
		,"Low");
	gtk_combo_box_text_append_text((GtkComboBoxText *)mcmBox
		,"High");

	GtkWidget * hwBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)hwBox
		,"Never");
	gtk_combo_box_text_append_text((GtkComboBoxText *)hwBox
		,"On Request");
	gtk_combo_box_text_append_text((GtkComboBoxText *)hwBox
		,"Always");

	#ifdef PLANC_FEATURE_GNOME
	GtkWidget * tm = gtk_check_button_new_with_label
		("Always show menu at top of window");
	#endif
	GtkWidget * dd = gtk_check_button_new_with_label
		("Download files into a domain folder");
	GtkWidget * dv = gtk_check_button_new_with_label
		("Enable Developer Options");
    GtkWidget * ch = gtk_check_button_new_with_label
		("Enable Page Cache");
    GtkWidget * ta = gtk_check_button_new_with_label
		("Autohide tab bar in single tab windows");
	GtkWidget * tt = (GtkWidget *) InitComboBoxLabel
		("Default Tab Layout",tabBox);
	GtkWidget * cm = (GtkWidget *) InitComboBoxLabel
		("Memory Cache Model",mcmBox);
	GtkWidget * hw = (GtkWidget *) InitComboBoxLabel
		("Hardware Accelleration",hwBox);
	GtkWidget * js = gtk_check_button_new_with_label
		("Enable JavaScript");
	GtkWidget * jv = gtk_check_button_new_with_label
		("Enable Java");
	GtkWidget * ms = gtk_check_button_new_with_label
		("Enable Media Source Extensions");
	GtkWidget * in = gtk_check_button_new_with_label
		("Enable Plugins");
	GtkWidget * pt = gtk_check_button_new_with_label
		("Enable Process Per Tab (Requires Restart)");

	//Bind to setting
	g_settings_bind (G_SETTINGS,"download-domain",dd,"active",
		 G_SETTINGS_BIND_DEFAULT);
	#ifdef PLANC_FEATURE_GNOME
	g_settings_bind (G_SETTINGS,"planc-traditional",tm,"active",
		 G_SETTINGS_BIND_DEFAULT);
	#endif
	g_settings_bind (G_SETTINGS,"webkit-cache",ch,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"tab-autohide",ta,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-mse",ms,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-plugins",in,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-hw",hwBox,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-js",js,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-java",jv,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-ppt",pt,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-dev",dv,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"tab-layout",tabBox,"active",
		 G_SETTINGS_BIND_DEFAULT);



	//Connect events
	g_signal_connect(mcmBox, "changed"
		,G_CALLBACK(c_settings_cm), NULL);

	g_signal_connect(hwBox, "changed"
		,G_CALLBACK(c_settings_cm), NULL);

	g_signal_connect(ta, "toggled"
		,G_CALLBACK(c_notebook_tabs_autohide), NULL);
	#ifdef PLANC_FEATURE_GNOME
	g_signal_connect(tm, "toggled"
		,G_CALLBACK(c_traditional_menu_hide), NULL);
	#endif
	g_signal_connect(ms, "toggled"
		,G_CALLBACK(c_settings_mse), NULL);

	g_signal_connect(jv, "toggled"
		,G_CALLBACK(c_settings_jv), NULL);

	g_signal_connect(js, "toggled"
		,G_CALLBACK(c_settings_js), NULL);

	g_signal_connect(in, "toggled"
		,G_CALLBACK(c_settings_in), NULL);

	g_signal_connect(ch, "toggled"
		,G_CALLBACK(c_settings_ch), NULL);

	g_signal_connect(dv, "toggled"
		,G_CALLBACK(c_settings_dv), NULL);

	//Parent
	#ifdef PLANC_FEATURE_GNOME
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(tm),FALSE,FALSE,0);
	#endif
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(dd),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(dv),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(ta),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(jv),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(js),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(in),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(ms),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(ch),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(pt),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(cm),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(hw),FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(tt),FALSE,FALSE,0);

	switch(webkit_web_context_get_cache_model(G_WKC))
	{
		case WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER:
			gtk_combo_box_set_active((GtkComboBox *) mcmBox,0);
		break;
		default:
			gtk_combo_box_set_active((GtkComboBox *) mcmBox,1);
		break;
		case WEBKIT_CACHE_MODEL_WEB_BROWSER:
			gtk_combo_box_set_active((GtkComboBox *) mcmBox,2);
		break;
	}

	gtk_widget_show_all((GtkWidget *)r);
	return r;
}
