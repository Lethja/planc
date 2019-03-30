#define _GNU_SOURCE
#include "main.h"
#include "history.h"
#include "libdatabase.h"

enum
{
   ADDRESS_COLUMN,
   TITLE_COLUMN,
   VISITED_COLUMN,
   N_COLUMNS
};

static const gchar * G_search;
static GtkWidget * G_scrollWin;
static GtkTreeStore * store;
static GtkWidget * tree;

static int treeIter(void * store, int count, char **data
	,char **columns)
{
	GtkTreeIter iter;
	if(count == 3)
	{
		gtk_tree_store_append(store, &iter, NULL);
		GDateTime * d = g_date_time_new_from_unix_utc(atoll(data[2]));
		gchar * s = g_date_time_format(d,"%F %T");
		gtk_tree_store_set(store, &iter, 0
		,data[0], 1, data[1]
		,2, s ? s : "Unknown", -1);
		if(s)
			g_free(s);
		g_date_time_unref(d);
	}
	return 0;
}

void c_history_url(GtkTreeView * tree_view, GtkTreePath * path
	,GtkTreeViewColumn *column, void * v)
{
	gchar *str_data;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		GtkNotebook * nb = get_web_view_notebook();
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 0
			,&str_data, -1);
		webkit_web_view_load_uri(WK_CURRENT_TAB(nb), str_data);
	}
}

gboolean c_history_url_tab(GtkTreeView * tree, GdkEventButton * event
	,void * v)
{
	if(event->type == GDK_BUTTON_RELEASE)
    {
		if(event->button == 2) //Middle click
		{
			gchar *str_data;
			GtkTreePath * path;

			if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree)
				,(gint) event->x, (gint) event->y
				,&path, NULL, NULL, NULL))
			{
				GtkTreeIter iter;
				GtkTreeModel *model = gtk_tree_view_get_model(tree);

				if (gtk_tree_model_get_iter(model, &iter, path))
				{
					PlancWindow * w = (PlancWindow *) get_web_view();
					gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 0
						,&str_data, -1);
					new_tab_ext(str_data,w,TRUE);
				}
			}
		}
	}
    return false;
}

static void search_entry_change(GtkWidget * e, GtkTreeModelFilter * f)
{
	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(G_HISTORY))
		,gdk_cursor_new_from_name(gdk_display_get_default(), "wait"));
	while (gtk_events_pending ())
		gtk_main_iteration ();
	G_search = gtk_entry_get_text((GtkEntry *) e);
	gtk_tree_model_filter_refilter(f);
	gtk_adjustment_set_value(gtk_scrolled_window_get_vadjustment
		(GTK_SCROLLED_WINDOW(G_scrollWin)), 0.0);
	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(G_HISTORY))
		,gdk_cursor_new_from_name(gdk_display_get_default()
		,"default"));
}

static gboolean hisstrstr(GtkTreeModel * model, GtkTreeIter * iter
	,void * v)
{
	//Keep the program responsive while filtering big lists
	while (gtk_events_pending ())
		gtk_main_iteration ();

	if(*G_search == '\0')
		return TRUE;
	gchar *str;
	for(size_t x = 0; x < 2; x++)
	{
		gtk_tree_model_get(model, iter, x, &str, -1);
		if(str && strcasestr(str,G_search))
		{
			g_free(str);
			return TRUE;
		}
		g_free(str);
	}
	return FALSE;
}

static void c_destroy_window(GtkWindow * w, void * v)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW(tree), NULL);
	G_HISTORY = NULL;
}

/* The function to use as a parameter of g_task_run_in_thread */
static void load_data_thread (GTask         *task,
                  gpointer       source_object,
                  gpointer       store,
                  GCancellable  *cancellable)
{
	/* Actual function here*/
	sql_history_read_to_tree(store, &treeIter);
	GError *error = NULL;
	if (store)
		g_task_return_pointer (task, store, g_object_unref);
	else
		g_task_return_error (task, error);
}

static void load_data_free (GtkTreeStore * store)
{
	g_object_unref(store);
}

/* Normal function you call from main thread */
void history_load_data_async (void * self, GCancellable * cancellable
	,GAsyncReadyCallback  callback)
{
	GTask *task;
	GtkTreeStore * store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING
		,G_TYPE_STRING, G_TYPE_STRING);
	task = g_task_new (self, cancellable, callback, store);
	g_task_set_task_data (task, store, NULL);
	g_task_run_in_thread (task, load_data_thread);
	g_object_unref (task);
}

/* Just needs to exist to signal or could return a value if non-void */
GtkTreeStore * history_load_data_finish (void * self
	,GAsyncResult * result, GError **error)
{
	g_return_val_if_fail (g_task_is_valid (result, self), NULL);

	return g_task_propagate_pointer (G_TASK (result), error);
}

static void history_load_data_result (GObject * obj, GAsyncResult * res
	,gpointer user_data)
{
	gboolean success = FALSE;

	store = history_load_data_finish (obj, res, NULL);
}

extern void InitHistoryWindow(void * v)
{
	if(G_HISTORY)
	{
		gtk_window_present(G_HISTORY);
		return;
	}
	store = NULL;
	G_search = NULL;
    G_HISTORY =
		(GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(G_HISTORY,600,400);
    gtk_window_set_position(G_HISTORY,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(G_HISTORY,"accessories-text-editor");
	gtk_window_set_title(G_HISTORY,"History - Plan C");
	GtkWidget * Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget * searchEntry = gtk_search_entry_new();
	G_scrollWin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_min_content_width
		(GTK_SCROLLED_WINDOW(G_scrollWin),320);
	gtk_scrolled_window_set_min_content_height
		(GTK_SCROLLED_WINDOW(G_scrollWin),240);
	gtk_box_pack_start(GTK_BOX(Vbox),searchEntry,0,1,0);
	gtk_box_pack_start(GTK_BOX(Vbox),G_scrollWin,1,1,0);
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeModelSort * sort;
	GtkTreeSortable *sortable;
	GtkTreeModelFilter * filtered;

	tree = gtk_tree_view_new();
	renderer = gtk_cell_renderer_text_new();

	column = gtk_tree_view_column_new_with_attributes ("URL"
		,renderer, "text", ADDRESS_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 200);
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Title"
		,renderer, "text", TITLE_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 300);
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Last Visited"
		,renderer, "text", VISITED_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,2);
	gtk_tree_view_column_set_fixed_width (column, 100);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	gtk_container_add(GTK_CONTAINER (G_scrollWin), tree);
	gtk_container_add(GTK_CONTAINER(G_HISTORY), Vbox);
    gtk_widget_show_all((GtkWidget *) G_HISTORY);

	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(G_HISTORY))
		,gdk_cursor_new_from_name(gdk_display_get_default(), "wait"));

	/* Create a model.  We are using the store model for now, though we
	* could use any other GtkTreeModel */

	history_load_data_async(NULL, NULL, history_load_data_result);

	while(!store)
	{
		while (gtk_events_pending ())
			gtk_main_iteration ();
	}

	filtered = GTK_TREE_MODEL_FILTER
		(gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL));

	sort = GTK_TREE_MODEL_SORT
		(gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(filtered)));
	sortable = GTK_TREE_SORTABLE(sort);

	gtk_tree_sortable_set_sort_column_id(sortable, VISITED_COLUMN
		,GTK_SORT_DESCENDING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree)
		,GTK_TREE_MODEL(sortable));

	g_object_unref(G_OBJECT (store));
	g_object_unref(G_OBJECT (filtered));
	g_object_unref(G_OBJECT (sort));

	g_signal_connect(G_HISTORY, "destroy"
        ,G_CALLBACK(c_destroy_window), NULL);
	g_signal_connect(searchEntry, "activate"
        ,G_CALLBACK(search_entry_change), filtered);
	g_signal_connect(tree,"row-activated"
		,G_CALLBACK(c_history_url), v);
	g_signal_connect(tree,"button-release-event"
		,G_CALLBACK(c_history_url_tab), v);

	gtk_tree_model_filter_set_visible_func(filtered,
		(GtkTreeModelFilterVisibleFunc) hisstrstr, NULL, NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);

	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(G_HISTORY))
		,gdk_cursor_new_from_name(gdk_display_get_default()
		,"default"));

    gtk_widget_show_all((GtkWidget *) G_HISTORY);
    return;
}
