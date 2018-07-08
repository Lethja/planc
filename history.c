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

static int treeIter(void * store, int count, char **data
	,char **columns)
{
	GtkTreeIter iter;
	if(count == 3)
	{
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,ADDRESS_COLUMN
		,data[0],	TITLE_COLUMN, data[1]
		,VISITED_COLUMN, data[2],-1);
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
		/*if(event->button == 3) //Right click
		{
			GtkTreeSelection * selection
				= gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
		}*/
	}
    return false;
}

static void search_entry_change(GtkWidget * e, GtkTreeModelFilter * f)
{
	G_search = gtk_entry_get_text((GtkEntry *) e);
	gtk_tree_model_filter_refilter(f);
}

static gboolean hisstrstr(GtkTreeModel * model, GtkTreeIter * iter
	,void * v)
{
	const gchar * data = G_search;
	if(!data || strcmp(data,"") == 0)
		return TRUE;
	gchar *str;
	for(size_t x = 0; x < 2; x++)
	{
		gtk_tree_model_get(model, iter, x, &str, -1);
		if(str && strcasestr(str,data))
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
	G_HISTORY = NULL;
}

extern void InitHistoryWindow(void * v)
{
	if(G_HISTORY)
	{
		gtk_window_present(G_HISTORY);
		return;
	}
	G_search = NULL;
    G_HISTORY =
		(GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(G_HISTORY,600,400);
    gtk_window_set_position(G_HISTORY,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(G_HISTORY,"accessories-text-editor");
	gtk_window_set_title(G_HISTORY,"History - Plan C");
	GtkWidget * Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget * searchEntry = gtk_search_entry_new();
	GtkWidget * scrollWin = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start(GTK_BOX(Vbox),searchEntry,0,1,0);
	gtk_box_pack_start(GTK_BOX(Vbox),scrollWin,1,1,0);
	GtkTreeStore *store;
	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeModelSort * sort;
	GtkTreeSortable *sortable;
	GtkTreeModelFilter * filtered;

	/* Create a model.  We are using the store model for now, though we
	* could use any other GtkTreeModel */
	store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING
		,G_TYPE_STRING, G_TYPE_STRING);

	/* custom function to fill the model with data */
	sql_history_read_to_tree(store, &treeIter);

	/* Create the filter modal */
	filtered = GTK_TREE_MODEL_FILTER
		(gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL));

	gtk_tree_model_filter_set_visible_func(filtered,
		(GtkTreeModelFilterVisibleFunc) hisstrstr, NULL, NULL);

	sort = GTK_TREE_MODEL_SORT
		(gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(filtered)));
	sortable = GTK_TREE_SORTABLE(sort);

	gtk_tree_sortable_set_sort_column_id(sortable, VISITED_COLUMN
		,GTK_SORT_DESCENDING);

	/* Create a view */
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (sortable));

	/* The view now holds a reference.  We can get rid of our own
    * reference */
	g_object_unref (G_OBJECT (store));

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

	g_signal_connect(G_HISTORY, "destroy"
        ,G_CALLBACK(c_destroy_window), NULL);
	g_signal_connect_after(searchEntry, "search-changed"
        ,G_CALLBACK(search_entry_change), filtered);
	g_signal_connect(tree,"row-activated"
		,G_CALLBACK(c_history_url), v);
	g_signal_connect(tree,"button-release-event"
		,G_CALLBACK(c_history_url_tab), v);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_container_add(GTK_CONTAINER (scrollWin), tree);
	gtk_container_add(GTK_CONTAINER(G_HISTORY), Vbox);
    gtk_widget_show_all((GtkWidget *) G_HISTORY);
    return;
}
