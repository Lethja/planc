#include "main.h"
#include "settings.h"
#include "plan.h"
#include "libdatabase.h"

GtkWindow * G_WIN_SETTINGS = NULL;

enum
{
	KEY_COLUMN,
	TITLE_COLUMN,
	URL_COLUMN,
	N_COLUMNS
};

static int searchTreeIter(void * store, int count, char **data
	,char **columns)
{
	GtkTreeIter iter;
	if(count == 3)
	{
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,KEY_COLUMN, data[0]
			,URL_COLUMN, data[1], TITLE_COLUMN, data[2], -1);
	}
	return 0;
}

void c_notebook_tabs_autohide(GtkToggleButton * cbmi
	,void * v)
{
	gboolean setting = gtk_toggle_button_get_active(cbmi);
	GList * wins = gtk_application_get_windows(G_APP);
	while(wins)
	{
		struct call_st * c = planc_window_get_call
			(PLANC_WINDOW(wins->data));
#ifdef PLANC_FEATURE_DMENU
		if(c->menu->tabsMh)
		{
			wins = wins->next;
			continue;
		}
#endif
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
		{
			gtk_widget_show(GTK_WIDGET(c->menu->menu));
			gtk_application_window_set_show_menubar
					(GTK_APPLICATION_WINDOW(wins->data), FALSE);
		}
		else
		{
			gtk_widget_hide(GTK_WIDGET(c->menu->menu));
			gtk_application_window_set_show_menubar
					(GTK_APPLICATION_WINDOW(wins->data), TRUE);
		}
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

static void attachLabeledWidget(GtkGrid * grid, const gchar * l
	,GtkWidget * c, gint row)
{
	GtkWidget * cbl = gtk_label_new(l);
	gtk_label_set_xalign((GtkLabel *) cbl, 0.0);
	gtk_widget_set_hexpand(cbl, TRUE);
	gtk_widget_set_margin_start(cbl, 2);
	gtk_widget_set_margin_end(cbl, 2);
	gtk_grid_attach(grid,cbl,0,row,1,1);
	gtk_widget_set_hexpand(c, TRUE);
	gtk_widget_set_margin_start(c, 2);
	gtk_widget_set_margin_end(c, 2);
	gtk_widget_set_margin_bottom(c, 1);
	gtk_widget_set_margin_top(c, 1);
	gtk_grid_attach(grid,c,1,row,1,1);
}

static GtkWidget * InitSettingTab_search_tree()
{
	GtkTreeStore *store;
	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	/* Create a model.  We are using the store model for now, though we
	* could use any other GtkTreeModel */
	store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING
		,G_TYPE_STRING, G_TYPE_STRING);

	/* custom function to fill the model with data */
	sql_search_read_to_tree(store, &searchTreeIter);

	/* Create a view */
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

	/* The view now holds a reference.  We can get rid of our own
    * reference */
	g_object_unref (G_OBJECT (store));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes ("Key"
		,renderer, "text", KEY_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 50);
	gtk_tree_view_column_set_resizable(column, TRUE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes ("Provider"
		,renderer, "text", TITLE_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 150);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(tree),TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(tree),TRUE);
	return tree;
}

static GtkDialog * SettingTab_search_ae(GtkTreeSelection * s
	,gboolean edit, GtkWindow * p)
{
	GtkTreeIter iter;
	GtkTreeModel * model;
	GtkDialog * dialog = (GtkDialog *) gtk_dialog_new_with_buttons
		("Search Provider", p
		,GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
		,edit ? _("_Update") : _("_Add"), GTK_RESPONSE_ACCEPT
		,_("_Cancel"), GTK_RESPONSE_REJECT, NULL);

	GtkWidget * dbox = gtk_dialog_get_content_area(dialog);
	GtkWidget * grid = gtk_grid_new();

	GtkWidget * en = gtk_entry_new();
	GtkWidget * ek = gtk_entry_new();
	GtkWidget * eu = gtk_entry_new();

	if(s)
	{
		if (gtk_tree_selection_get_selected(s, &model, &iter))
		{
			if(edit)
			{
				gchar * key  = NULL;
				gchar * name = NULL;
				gchar * uri  = NULL;
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0
					,&key, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1
					,&name, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 2
					,&uri, -1);
				gtk_entry_set_text(GTK_ENTRY(en),name);
				gtk_entry_set_text(GTK_ENTRY(ek),key);
				gtk_entry_set_text(GTK_ENTRY(eu),uri);
				g_free(key);
				g_free(name);
				g_free(uri);
			}
		}
	}

	attachLabeledWidget(GTK_GRID(grid),"Provider",en,0);
	attachLabeledWidget(GTK_GRID(grid),"Key",ek,1);
	attachLabeledWidget(GTK_GRID(grid),"URL",eu,2);

    gtk_box_pack_start(GTK_BOX(dbox),grid,0,1,0);
    gtk_widget_show_all(dbox);
	int result = gtk_dialog_run(dialog);
	if(result == GTK_RESPONSE_ACCEPT)
	{
		if(sql_search_write(gtk_entry_get_text(GTK_ENTRY(ek))
			,gtk_entry_get_text(GTK_ENTRY(eu))
			,gtk_entry_get_text(GTK_ENTRY(en))))
		{
			if(s)
			{
				if(!edit)
				{
					gtk_tree_store_append(GTK_TREE_STORE(model), &iter
						,NULL);
				}

				gtk_tree_store_set(GTK_TREE_STORE(model), &iter
					,0, gtk_entry_get_text(GTK_ENTRY(ek))
					,2, gtk_entry_get_text(GTK_ENTRY(eu))
					,1, gtk_entry_get_text(GTK_ENTRY(en)) ,-1);
			}
		}
	}
	return dialog;
}

static void c_settings_search_add(GtkWidget * w, GtkTreeView * tree)
{
	GtkTreeSelection * s = gtk_tree_view_get_selection(tree);
	GtkDialog * d = SettingTab_search_ae(s, FALSE, G_WIN_SETTINGS);
	gtk_widget_destroy(GTK_WIDGET(d));
}

static void c_settings_search_edit(GtkWidget * w, GtkTreeView * tree)
{
	GtkTreeSelection * s = gtk_tree_view_get_selection(tree);
	GtkDialog * d = SettingTab_search_ae(s, TRUE, G_WIN_SETTINGS);
	gtk_widget_destroy(GTK_WIDGET(d));
}

static void c_settings_search_drop(GtkWidget * w, GtkTreeView * tree)
{
	GtkTreeIter iter;
	GtkTreeModel * model;
	GtkTreeSelection * s = gtk_tree_view_get_selection(tree);
	gchar * key  = NULL;
	if (gtk_tree_selection_get_selected(s, &model, &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &key, -1);
		if(key)
		{
			if(sql_search_drop(key))
				gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
			g_free(key);
		}
	}
}

static void c_settings_set_implicit(GtkWidget * w, void * v)
{
	GtkTreeIter iter;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(w), &iter);
	gchar * s;
	GtkTreeModel * model = gtk_combo_box_get_model(GTK_COMBO_BOX(w));
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &s, -1);
	g_settings_set_string(G_SETTINGS, "planc-search", s);
	g_free(s);
}

static gboolean setupSearchTree(GtkTreeModel * model
	,GtkTreePath * path, GtkTreeIter * iter, void * tree)
{
	gchar * settingK = g_settings_get_string(G_SETTINGS,"planc-search");
	gchar * iterK;
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, 0, &iterK, -1);
	if(strcmp(iterK, settingK))
	{
		g_free(iterK);
		return FALSE;
	}
	else
	{
		g_free(iterK);
		GtkTreeSelection * selection
			= gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
		gtk_tree_selection_select_iter(selection, iter);
		return TRUE;
	}
}

static GtkWidget * InitSettingTab_history()
{
	GtkWidget * scrl = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_min_content_width
		(GTK_SCROLLED_WINDOW(scrl),320);
	gtk_scrolled_window_set_min_content_height
		(GTK_SCROLLED_WINDOW(scrl),240);
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget * IsFrame = gtk_frame_new("History Cleaning");
	GtkWidget * IsGrid = gtk_grid_new();
	gtk_widget_set_margin_start(GTK_WIDGET(IsGrid), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(IsGrid), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(IsGrid), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(IsGrid), 2);
	GtkWidget * achBox = gtk_combo_box_text_new();
	attachLabeledWidget(GTK_GRID(IsGrid), "Clean History Automatically"
		,GTK_WIDGET(achBox),0);
	gtk_combo_box_text_append_text((GtkComboBoxText *)achBox
		,"Never");
	gtk_combo_box_text_append_text((GtkComboBoxText *)achBox
		,"Every Exit");
	gtk_combo_box_text_append_text((GtkComboBoxText *)achBox
		,"Visited Over A Week Ago");
	gtk_combo_box_text_append_text((GtkComboBoxText *)achBox
		,"Visited Over A Fortnight Ago");
	gtk_combo_box_text_append_text((GtkComboBoxText *)achBox
		,"Visited Over A Month Ago");
	gtk_combo_box_text_append_text((GtkComboBoxText *)achBox
		,"Visited Over A Year Ago");
	gtk_container_add(GTK_CONTAINER(IsFrame),IsGrid);
	gtk_container_add(GTK_CONTAINER(vbox),IsFrame);

	gtk_widget_set_margin_start(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox), 2);
	gtk_container_add(GTK_CONTAINER(scrl),vbox);

	g_settings_bind (G_SETTINGS, "planc-history-clean", achBox, "active"
		,G_SETTINGS_BIND_DEFAULT);

	return scrl;
}

static GtkWidget * InitSettingTab_search()
{
	GtkWidget * scrl = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_min_content_width
		(GTK_SCROLLED_WINDOW(scrl),320);
	gtk_scrolled_window_set_min_content_height
		(GTK_SCROLLED_WINDOW(scrl),240);
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget * IsFrame = gtk_frame_new("Implicit Search");
	GtkWidget * IsGrid = gtk_grid_new();
	gtk_widget_set_margin_start(GTK_WIDGET(IsGrid), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(IsGrid), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(IsGrid), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(IsGrid), 2);
	GtkWidget * is = gtk_check_button_new_with_label
		("Allow implicit searching");
	gtk_grid_attach(GTK_GRID(IsGrid),is,0,0,2,1);
	GtkWidget * ispBox = gtk_combo_box_text_new();
	attachLabeledWidget(GTK_GRID(IsGrid), "Implicit search key"
		,GTK_WIDGET(ispBox),1);
	gtk_container_add(GTK_CONTAINER(IsFrame),IsGrid);
	gtk_container_add(GTK_CONTAINER(vbox),IsFrame);

	GtkWidget * SpFrame = gtk_frame_new("Search Providers");
	GtkWidget * SpBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget * tool = gtk_toolbar_new();
	GtkWidget * tree = InitSettingTab_search_tree();
	GtkToolItem * sa = gtk_tool_button_new
		(gtk_image_new_from_icon_name ("list-add"
		,GTK_ICON_SIZE_SMALL_TOOLBAR), "Add");
	GtkToolItem * se = gtk_tool_button_new
		(gtk_image_new_from_icon_name ("document-properties"
		,GTK_ICON_SIZE_SMALL_TOOLBAR)
		,"Edit");
	GtkToolItem * sr = gtk_tool_button_new
		(gtk_image_new_from_icon_name ("list-remove"
		,GTK_ICON_SIZE_SMALL_TOOLBAR)
		,"Remove");

	g_signal_connect(GTK_WIDGET(sa), "clicked"
		,G_CALLBACK(c_settings_search_add), tree);
	g_signal_connect(GTK_WIDGET(se), "clicked"
		,G_CALLBACK(c_settings_search_edit), tree);
	g_signal_connect(GTK_WIDGET(sr), "clicked"
		,G_CALLBACK(c_settings_search_drop), tree);

	gtk_toolbar_insert(GTK_TOOLBAR(tool), sa, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(tool), se, 1);
	gtk_toolbar_insert(GTK_TOOLBAR(tool), sr, 2);

	gtk_toolbar_set_style(GTK_TOOLBAR(tool), GTK_TOOLBAR_BOTH_HORIZ);

	gtk_box_pack_start(GTK_BOX(SpBox), tree, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(SpBox), tool, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(SpFrame),SpBox);
	gtk_container_add(GTK_CONTAINER(vbox),SpFrame);

	{
		GtkTreeModel * model
			= gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
		gtk_combo_box_set_model(GTK_COMBO_BOX(ispBox), model);

		gtk_tree_model_foreach
			(gtk_tree_view_get_model(GTK_TREE_VIEW(tree))
			,setupSearchTree, tree);

		GtkTreeIter iter;
		GtkTreeSelection * selection
			= gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
		if(gtk_tree_selection_get_selected(selection, NULL, &iter))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ispBox), &iter);
		else
		{
			gtk_tree_model_get_iter_first(model, &iter);
			gtk_tree_selection_select_iter(selection, &iter);
		}
	}

	g_signal_connect(ispBox, "changed"
		,G_CALLBACK(c_settings_set_implicit), NULL);

	g_settings_bind (G_SETTINGS, "planc-search-implicit", is, "active"
		,G_SETTINGS_BIND_DEFAULT);

	gtk_widget_set_margin_start(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox), 2);
	gtk_container_add(GTK_CONTAINER(scrl),vbox);
	return scrl;
}

static GtkWidget * InitSettingTab_general()
{
	GtkWidget * scrl = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_min_content_width
		(GTK_SCROLLED_WINDOW(scrl),320);
	gtk_scrolled_window_set_min_content_height
		(GTK_SCROLLED_WINDOW(scrl),240);
	GtkWidget * PcFrame = gtk_frame_new("Plan C");
	GtkWidget * PcGrid = gtk_grid_new();
	gtk_widget_set_margin_start(GTK_WIDGET(PcGrid), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(PcGrid), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(PcGrid), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(PcGrid), 2);
	gtk_container_add((GtkContainer *)PcFrame, PcGrid);

	GtkWidget * WkFrame = gtk_frame_new("Webkit");
	GtkWidget * WkGrid = gtk_grid_new();
	gtk_widget_set_margin_start(GTK_WIDGET(WkGrid), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(WkGrid), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(WkGrid), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(WkGrid), 2);
	gtk_container_add((GtkContainer *)WkFrame, WkGrid);

	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_end(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_top(GTK_WIDGET(vbox), 2);
	gtk_widget_set_margin_bottom(GTK_WIDGET(vbox), 2);
	gtk_container_add((GtkContainer *)vbox, PcFrame);
	gtk_container_add((GtkContainer *)vbox, WkFrame);
    gtk_container_add((GtkContainer *)scrl, vbox);

    //Init
    GtkWidget * tabBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)tabBox
		,"Horizontal");
	gtk_combo_box_text_append_text((GtkComboBoxText *)tabBox
		,"Vertical");
#ifdef PLANC_FEATURE_DMENU
	gtk_combo_box_text_append_text((GtkComboBoxText *)tabBox
		,"Menu");
#endif
	GtkWidget * mcmBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)mcmBox
		,"None");
	gtk_combo_box_text_append_text((GtkComboBoxText *)mcmBox
		,"Low");
	gtk_combo_box_text_append_text((GtkComboBoxText *)mcmBox
		,"High");

	GtkWidget * hwaBox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)hwaBox
		,"Never");
	gtk_combo_box_text_append_text((GtkComboBoxText *)hwaBox
		,"On Request");
	gtk_combo_box_text_append_text((GtkComboBoxText *)hwaBox
		,"Always");

	#ifdef PLANC_FEATURE_GNOME
	GtkWidget * tm = gtk_check_button_new_with_label
		("Always show menu at top of window");
	#endif
	#ifdef PLANC_FEATURE_DMENU
	GtkWidget * th = gtk_check_button_new_with_label
		("Switch tab when hovering in tab menu");
	#endif
	GtkWidget * dd = gtk_check_button_new_with_label
		("Download files into a domain folder");
	GtkWidget * dv = gtk_check_button_new_with_label
		("Enable Developer Options");
    GtkWidget * ch = gtk_check_button_new_with_label
		("Enable Page Cache");
    GtkWidget * ta = gtk_check_button_new_with_label
		("Autohide tab bar in single tab windows");
	GtkWidget * ii = gtk_check_button_new_with_label
		("Inhibit system idel actions when audio is playing");
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
	#ifdef PLANC_FEATURE_DMENU
	g_settings_bind (G_SETTINGS,"planc-tabs-hover",th,"active",
		 G_SETTINGS_BIND_DEFAULT);
	#endif
	g_settings_bind (G_SETTINGS,"webkit-cache",ch,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"tab-autohide",ta,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"planc-inhibit",ii,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-mse",ms,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-plugins",in,"active",
		 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (G_SETTINGS,"webkit-hw",hwaBox,"active",
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

	g_signal_connect(hwaBox, "changed"
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
	gtk_grid_attach(GTK_GRID(PcGrid),GTK_WIDGET(tm),0,0,2,1);
#endif
#ifdef PLANC_FEATURE_DMENU
	gtk_grid_attach(GTK_GRID(PcGrid),GTK_WIDGET(th),0,1,2,1);
#endif
	gtk_grid_attach(GTK_GRID(PcGrid),GTK_WIDGET(dd),0,2,2,1);
	gtk_grid_attach(GTK_GRID(PcGrid),GTK_WIDGET(ii),0,3,2,1);
	gtk_grid_attach(GTK_GRID(PcGrid),GTK_WIDGET(ta),0,4,2,1);
	attachLabeledWidget(GTK_GRID(PcGrid), "Default Tab Layout"
		,GTK_WIDGET(tabBox),5);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(dv),0,0,2,1);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(jv),0,1,2,1);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(js),0,2,2,1);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(in),0,3,2,1);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(ms),0,4,2,1);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(ch),0,5,2,1);
	gtk_grid_attach(GTK_GRID(WkGrid),GTK_WIDGET(pt),0,6,2,1);
	attachLabeledWidget(GTK_GRID(WkGrid), "Memory Cache Model"
		,GTK_WIDGET(mcmBox),7);
	attachLabeledWidget(GTK_GRID(WkGrid), "Hardware Accelleration"
		,GTK_WIDGET(hwaBox),8);
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
	return scrl;
}

GtkWindow * InitSettingsWindow(PlancWindow * v)
{
	GtkWindow * r = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(r,400,400);
	gtk_window_set_position(r,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(r,"preferences-system-network");
	gtk_window_set_title(r,"Settings - Plan C");

	GtkWidget * nb = gtk_notebook_new();
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), InitSettingTab_general()
		,gtk_label_new("General"));
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), InitSettingTab_search()
		,gtk_label_new("Search"));
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), InitSettingTab_history()
		,gtk_label_new("History"));
	gtk_container_add((GtkContainer *)r, nb);
	gtk_widget_show_all((GtkWidget *)r);
	return r;
}
