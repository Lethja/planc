#define _GNU_SOURCE
#include <string.h>
#include <gtk/gtk.h>
#include "history.h"
#include "database.h"
#include "main.h"

enum
{
   ADDRESS_COLUMN,
   TITLE_COLUMN,
   VISITED_COLUMN,
   N_COLUMNS
};

static const gchar * G_search;

extern int treeIter(void * store, int count, char **data
	,char **columns)
{
	GtkTreeIter iter;
	if(count == 3)
	{
		gtk_tree_store_append (store, &iter, NULL);
		/* Acquire an iterator */

		gtk_tree_store_set (store, &iter,ADDRESS_COLUMN
		,data[0],	TITLE_COLUMN, data[1]
		,VISITED_COLUMN, data[2],-1);
	}
}

void c_history_url(GtkTreeView * tree_view, GtkTreePath * path
	,GtkTreeViewColumn *column, struct call_st * c)
{
	gint int_data;
	gchar *str_data;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 0
			,&str_data, -1);
		new_tab_ext(str_data,c);
	}
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

extern void * InitHistoryWindow(void * v)
{
	G_search = NULL;
    GtkWindow * window =
		(GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(window,600,400);
    gtk_window_set_position(window,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(window,"accessories-text-editor");
	gtk_window_set_title(window,"History - Plan C");
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
	sql_history_read_to_tree(store);

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

	g_signal_connect_after(searchEntry, "search-changed"
        ,G_CALLBACK(search_entry_change), filtered);
	g_signal_connect(tree,"row-activated"
		,G_CALLBACK(c_history_url), v);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_container_add(GTK_CONTAINER (scrollWin), tree);
	gtk_container_add(GTK_CONTAINER(window), Vbox);
    gtk_widget_show_all((GtkWidget *) window);
    return window;
}
