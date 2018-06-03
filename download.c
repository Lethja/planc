#define _GNU_SOURCE
#include "main.h"
#include "download.h"
#include "database.h"
#include <sys/wait.h>

static const gchar * G_search;
static GtkTreeStore * G_store;

enum
{
	NAME_COLUMN, //File Name
	PAGE_COLUMN, //Download Page
	STAT_COLUMN, //File size/progress
	DURL_COLUMN, //Actual download URL
	FILE_COLUMN, //Actual file URI
	DATE_COLUMN, //Time of download start for ordering
	LMOD_COLUMN, //Modification date for comparing
	N_COLUMNS
};

static int treeIter(void * store, int count, char **data
	,char **columns)
{
	GtkTreeIter iter;
	if(count == 3)
	{
		gtk_tree_store_append (store, &iter, NULL);
		/* Acquire an iterator */

		gchar * file = getFileNameFromPath(data[2]);
		gchar * purl = data[0];
		if(!purl || strcmp(purl,"") == 0)
			purl = data[1];

		gtk_tree_store_set (store, &iter,PAGE_COLUMN
		,purl,	DURL_COLUMN, data[1]
		,FILE_COLUMN, data[2], NAME_COLUMN, file
		,STAT_COLUMN, "Complete"
		,-1);
	}
	return 0;
}

static GtkTreeIter * addDownloadEntry(const gchar * file
	,const gchar * name, const gchar * durl, const gchar * purl
	,const gchar * stat, void * v)
{
	InitDownloadWindow();
	GtkTreeIter * r = malloc(sizeof(GtkTreeIter));
	gtk_tree_store_append (G_store, r, NULL);

	gtk_tree_store_set (G_store, r, PAGE_COLUMN
		,purl, DURL_COLUMN, durl
		,FILE_COLUMN, file, NAME_COLUMN, name
		,STAT_COLUMN, "Downloading"
		,-1);

	return r;
}

static gboolean dlstrstr(GtkTreeModel * model, GtkTreeIter * iter
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

static void openFile(const gchar * file)
{
	pid_t pid = fork();
	if (pid == 0)
	{ //This is the child process
		execlp("xdg-open","xdg-open",file,NULL);
		exit(127); /* only if execlp fails */
	}
	else
	{ //This is the parent process
		waitpid(pid,0,0); //wait for child before continuing
	}
}

static void openFileDirectory(const gchar * file)
{
	gchar * dir = malloc(strlen(file)+1);
	strncpy(dir,file,strlen(file)+1);
	gchar * nullr = strrchr(dir,'/');
	nullr[0] = '\0';
	openFile(dir); //XDG will see a directory path
	free(dir);
}

static void c_download_finished(WebKitDownload * d, GtkTreeIter * iter)
{
	guint64 len = webkit_uri_response_get_content_length
		(webkit_download_get_response(d));
	gchar * l = NULL;
	if(len)
	{
		l = g_strdup_printf
			("Downloaded: %" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d),len,NULL);
		//gtk_progress_bar_set_text((GtkProgressBar *)w,l);
	}
	else
	{
		l = g_strdup_printf
			("Downloaded: %" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d), NULL);
		//gtk_progress_bar_set_text((GtkProgressBar *)w,l);
	}
	gtk_tree_store_set(G_store, iter, STAT_COLUMN, l, -1);
	free(l);
	free(iter);
	/*gtk_progress_bar_set_fraction((GtkProgressBar *)w
		,1.0);
	const gchar * file = webkit_download_get_destination(d);
	if(file)
	{
		openFileDirectory(file);
	}*/
}

static void c_download_progress(WebKitDownload * d, guint pro
	,GtkTreeIter * iter)
{
	guint64 len = webkit_uri_response_get_content_length
		(webkit_download_get_response(d));
	gchar * l = NULL;
	if(len)
	{
		l = g_strdup_printf
			("Downloading: %" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d),len,NULL);
		/*gtk_progress_bar_set_text((GtkProgressBar *)w,l);
		gtk_progress_bar_set_fraction((GtkProgressBar *)w
			,webkit_download_get_estimated_progress(d));*/
	}
	else
	{
		l = g_strdup_printf
			("Downloading: %" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d), NULL);
		/*gtk_progress_bar_set_text((GtkProgressBar *)w,l);
		gtk_progress_bar_pulse((GtkProgressBar *)w);*/
	}
	gtk_tree_store_set(G_store, iter, STAT_COLUMN, l, -1);
	free(l);
}

static void c_download_destination_created(WebKitDownload * d
	,gchar * u, void * v)
{
	gchar * n = getFileNameFromPath(u);
	const gchar * p
		= webkit_web_view_get_uri(webkit_download_get_web_view(d));
	const gchar * a
		= webkit_uri_request_get_uri(webkit_download_get_request(d));

	if(!p || strcmp(p,"") == 0)
		p = a;

	GtkTreeIter * t = addDownloadEntry(u,n,a,p,"Downloading",v);
	g_signal_connect(d,"received-data"
		,G_CALLBACK(c_download_progress),t);
	g_signal_connect(d, "finished"
        ,G_CALLBACK(c_download_finished),t);

	/*GtkWindow * r = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(r,280,40);
	gtk_window_set_position(r,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(r,"document-save");
	char * f = strrchr(fn,'/');
	gchar * t = NULL;
	if(f)
		t = g_strdup_printf("%s - Plan C",f+1);
	else
		t = g_strdup_printf("%s - Plan C",fn);

	gtk_window_set_title(r,t);
	free(t);
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget * lab = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text((GtkProgressBar *)lab,TRUE);
	gtk_progress_bar_set_pulse_step((GtkProgressBar *)lab,0.001);
	gtk_progress_bar_set_text((GtkProgressBar *)lab,"0");
    gtk_container_add((GtkContainer *)r, vbox);
    g_signal_connect(d,"received-data"
		,G_CALLBACK(c_download_progress),lab);
	g_signal_connect(d, "finished"
        ,G_CALLBACK(c_download_finished),lab);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(lab),TRUE,TRUE,0);
	gtk_widget_show_all((GtkWidget *)r);*/

	sql_download_write
		(webkit_web_view_get_uri(webkit_download_get_web_view(d))
		,webkit_uri_request_get_uri(webkit_download_get_request(d))
		,webkit_download_get_destination(d));
}

gchar * getFileNameFromPath(const gchar * path)
{
	char * a = strrchr (path, '.');
	char * b = strrchr (path, '/');
	if(a > b+1)
		return b+1;
	return NULL;
}

gchar * getPathFromFileName(const gchar * path)
{
	char * a = strrchr(path, '/');
	if(a)
	{
		size_t s = strlen(path) - strlen(a);
		char * r = malloc(s+1);
		strncpy(r,path,s);
		r[s+1] = '\0';
		return r;
	}
	return NULL;
}

static gchar * getDisposition(WebKitURIResponse * u)
{
	SoupMessageHeaders * s = NULL;

	if(u)
		s = webkit_uri_response_get_http_headers(u);
	if(s)
	{
		char * f = NULL;
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
			gchar * r = malloc(strlen(b)+1);
			strncpy(r,b,strlen(b)+1);
			free(a);
			free(f);
			return r;
		}
	}
	return NULL;
}

static gboolean c_download_save_as(WebKitDownload * d, gchar * fn
	,struct call_st * v)
{
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    gint res;
    GtkWindow * w = gtk_application_get_active_window(G_APP);
    dialog = gtk_file_chooser_dialog_new ("Save File"
		,(GtkWindow *) w,GTK_FILE_CHOOSER_ACTION_SAVE ,("_Cancel")
        ,GTK_RESPONSE_CANCEL ,("_Save") ,GTK_RESPONSE_ACCEPT,NULL);

    chooser = GTK_FILE_CHOOSER (dialog);

    gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

    if (fn == NULL || strcmp(fn,"") == 0)
    {
		gchar * f = getDisposition(webkit_download_get_response(d));
		if(f)
		{
			gtk_file_chooser_set_current_name(chooser,f);
			free(f);
		}
		else
			gtk_file_chooser_set_current_name(chooser
				,"Untitled download");
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

static gboolean c_download_prompt(WebKitDownload * d, gchar * fn
	,struct call_st * v)
{
	gchar * t = NULL;
	gchar * f = NULL;
	if (fn == NULL || strcmp(fn,"") == 0)
    {
		f = getDisposition(webkit_download_get_response(d));
		if(f)
			t = g_strconcat("Download request '", f, "' - Plan C"
				,NULL);
		else
			t = g_strconcat("Download request - Plan C", NULL);
	}
    else
	{
		t = g_strconcat("Download request '", fn, "' - Plan C", NULL);
		f = fn;
	}

	GtkWidget * DownloadPrompt = gtk_dialog_new_with_buttons(t
		,gtk_application_get_active_window(G_APP)
		,GTK_DIALOG_DESTROY_WITH_PARENT
		,_("_Save"),	GTK_RESPONSE_OK
		,_("Save _As"),	GTK_RESPONSE_APPLY
		,_("_Cancel"),	GTK_RESPONSE_CANCEL
		,NULL);

	g_free(t);

	gchar * absdir = g_build_filename(g_get_user_special_dir
		(G_USER_DIRECTORY_DOWNLOAD), f, NULL);

	GtkWidget * dbox = gtk_dialog_get_content_area
		(GTK_DIALOG(DownloadPrompt));

	gchar * str_name = g_strconcat("Name: ", f, NULL);
	gchar * str_puri = g_strconcat("Page: "
		,webkit_uri_request_get_uri(webkit_download_get_request(d))
		,NULL);
	gchar * str_adir;
	if(g_file_test(absdir, G_FILE_TEST_EXISTS))
	{
		str_adir = g_strconcat("Save to: ", absdir
			," (File exists)", NULL);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(DownloadPrompt)
			,GTK_RESPONSE_OK, FALSE);
	}
	else
		str_adir = g_strconcat("Save to: ", absdir, NULL);

	GtkWidget * name = gtk_label_new(str_name);
	GtkWidget * puri = gtk_label_new(str_puri);
	GtkWidget * adir = gtk_label_new(str_adir);

	g_free(str_name);
	g_free(str_puri);
	g_free(str_adir);

	gtk_label_set_xalign((GtkLabel *) name, 0.0);
	gtk_label_set_xalign((GtkLabel *) puri, 0.0);
	gtk_label_set_xalign((GtkLabel *) adir, 0.0);

	gtk_box_pack_start(GTK_BOX(dbox),name,0,1,0);
	gtk_box_pack_start(GTK_BOX(dbox),puri,0,1,0);
	gtk_box_pack_start(GTK_BOX(dbox),adir,0,1,0);

	gtk_widget_show_all(dbox);

	gint res = gtk_dialog_run(GTK_DIALOG(DownloadPrompt));
	gboolean r;
    switch(res)
    {
		case GTK_RESPONSE_OK:
		{
			gchar * uri = g_filename_to_uri(absdir, NULL, NULL);
			if(uri)
			{
				webkit_download_set_destination(d, uri);
				gtk_widget_destroy(DownloadPrompt);
				g_free(uri);
				r = TRUE;
			}
		}
		break;

        case GTK_RESPONSE_APPLY:
        gtk_widget_destroy(DownloadPrompt);
        r = c_download_save_as(d, f, v);
        break;

        default:
        webkit_download_cancel(d);
        gtk_widget_destroy(DownloadPrompt);
        r = FALSE;
        break;
    }
    g_free(absdir);
    if(f && f != fn)
		g_free(f);
    return r;
}

void c_download_start(WebKitWebContext * wv
    ,WebKitDownload * dl, void * v)
{
	webkit_download_set_allow_overwrite(dl,TRUE);
    g_signal_connect(dl, "decide-destination"
        ,G_CALLBACK(c_download_prompt), NULL);
	g_signal_connect(dl, "created-destination"
        ,G_CALLBACK(c_download_destination_created), NULL);
}

void c_download_dir(GtkTreeView * tree_view, GtkTreePath * path
	,GtkTreeViewColumn *column, void * v)
{
	gchar *str_data;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 4
			,&str_data, -1);
		openFile(str_data);
	}
}

/*static gboolean c_download_prompt(WebKitDownload * d, gchar * fn
	,struct call_st * v)
{
	GtkWindow * r = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(r,400,400);
	gtk_window_set_position(r,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(r,"preferences-system-network");
	gtk_window_set_title(r,"Download - Plan C");
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add((GtkContainer *)r, vbox);
    GtkWidget * sv = gtk_button_new_with_mnemonic("_Save");
    GtkWidget * sa = gtk_button_new_with_mnemonic("Save _As");
    GtkWidget * ca = gtk_button_new_with_mnemonic("_Cancel");
    GtkWidget * ln;
    if(fn)
    {
		gtk_label_new(fn);
		g_signal_connect(sv,"activate",G_CALLBACK(c_download_save),fn);
	}
	else
	{
		gtk_label_new("Untitled file");

	}
    g_signal_connect(sa,"activate",G_CALLBACK(c_download_save_as),fn);
    return r;
}*/

static void c_destroy_window(GtkWindow * w, void * v)
{
	G_DOWNLOAD = NULL;
}

static void search_entry_change(GtkWidget * e, GtkTreeModelFilter * f)
{
	G_search = gtk_entry_get_text((GtkEntry *) e);
	gtk_tree_model_filter_refilter(f);
}

extern void InitDownloadWindow()
{
	if(G_DOWNLOAD)
	{
		gtk_window_present(G_DOWNLOAD);
		return;
	}
	G_search = NULL;
    G_DOWNLOAD =
		(GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(G_DOWNLOAD,600,400);
    gtk_window_set_position(G_DOWNLOAD,GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(G_DOWNLOAD,"document-save");
	gtk_window_set_title(G_DOWNLOAD,"Downloads - Plan C");
	GtkWidget * Vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget * searchEntry = gtk_search_entry_new();
	GtkWidget * scrollWin = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start(GTK_BOX(Vbox),searchEntry,0,1,0);
	gtk_box_pack_start(GTK_BOX(Vbox),scrollWin,1,1,0);
	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeModelSort * sort;
	GtkTreeSortable *sortable;
	GtkTreeModelFilter * filtered;

	/* Create a model.  We are using the store model for now, though we
	* could use any other GtkTreeModel */
	G_store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING
		,G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING
		,G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	/* custom function to fill the model with data */
	sql_download_read_to_tree(G_store, &treeIter);

	/* Create the filter modal */
	filtered = GTK_TREE_MODEL_FILTER
		(gtk_tree_model_filter_new (GTK_TREE_MODEL (G_store), NULL));

	gtk_tree_model_filter_set_visible_func(filtered,
		(GtkTreeModelFilterVisibleFunc) dlstrstr, NULL, NULL);

	sort = GTK_TREE_MODEL_SORT
		(gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(filtered)));
	sortable = GTK_TREE_SORTABLE(sort);

	gtk_tree_sortable_set_sort_column_id(sortable, FILE_COLUMN
		,GTK_SORT_DESCENDING);

	/* Create a view */
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (sortable));

	/* The view now holds a reference.  We can get rid of our own
    * reference */
	g_object_unref (G_OBJECT (G_store));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("File"
		,renderer, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 200);
	gtk_tree_view_column_set_resizable(column, TRUE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes ("Download Page"
		,renderer, "text", PAGE_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 300);
	gtk_tree_view_column_set_resizable(column, TRUE);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Status"
		,renderer, "text", STAT_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column,2);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_fixed_width (column, 100);
	gtk_tree_view_column_set_resizable(column, TRUE);

	g_signal_connect(G_DOWNLOAD, "destroy"
        ,G_CALLBACK(c_destroy_window), NULL);
	g_signal_connect_after(searchEntry, "search-changed"
        ,G_CALLBACK(search_entry_change), filtered);
	g_signal_connect(tree,"row-activated"
		,G_CALLBACK(c_download_dir), NULL);
	/*g_signal_connect(tree,"button-release-event"
		,G_CALLBACK(c_history_url_tab), v);*/

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_container_add(GTK_CONTAINER (scrollWin), tree);
	gtk_container_add(GTK_CONTAINER(G_DOWNLOAD), Vbox);
    gtk_widget_show_all((GtkWidget *) G_DOWNLOAD);
    return;
}
