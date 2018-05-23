#include "main.h"
//#include "settings.h"
#include "download.h"
#include <sys/wait.h>

static void c_download_finished(WebKitDownload * d, GtkWidget * w)
{
	guint64 len = webkit_uri_response_get_content_length
		(webkit_download_get_response(d));
	gchar * l = NULL;
	if(len)
	{
		l = g_strdup_printf
			("Downloaded: %" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d),len,NULL);
		gtk_progress_bar_set_text((GtkProgressBar *)w,l);
	}
	else
	{
		l = g_strdup_printf
			("Downloaded: %" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d), NULL);
		gtk_progress_bar_set_text((GtkProgressBar *)w,l);
	}
	free(l);
	gtk_progress_bar_set_fraction((GtkProgressBar *)w
		,1.0);
	const gchar * file = webkit_download_get_destination(d);
	if(file)
	{
		gchar * dir = malloc(strlen(file)+1);
		strncpy(dir,file,strlen(file)+1);
		gchar * nullr = g_strrstr(dir,"/");
		nullr[0] = '\0';

		pid_t pid=fork();
		if (pid==0) { /* child process */
			execlp("xdg-open","xdg-open",dir,NULL);
			exit(127); /* only if execv fails */
		}
		else { /* pid!=0; parent process */
			waitpid(pid,0,0); /* wait for child to exit */
		}
	}
}

static void c_download_progress(WebKitDownload * d, guint pro
	,GtkWidget * w)
{
	guint64 len = webkit_uri_response_get_content_length
		(webkit_download_get_response(d));
	gchar * l = NULL;
	if(len)
	{
		l = g_strdup_printf
			("Downloading: %" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d),len,NULL);
		gtk_progress_bar_set_text((GtkProgressBar *)w,l);
		gtk_progress_bar_set_fraction((GtkProgressBar *)w
			,webkit_download_get_estimated_progress(d));
	}
	else
	{
		l = g_strdup_printf
			("Downloading: %" G_GUINT64_FORMAT
			,webkit_download_get_received_data_length(d), NULL);
		gtk_progress_bar_set_text((GtkProgressBar *)w,l);
		gtk_progress_bar_pulse((GtkProgressBar *)w);
	}
	free(l);
}

static void c_download_destination_created(WebKitDownload * d
	,gchar * fn, void * v)
{
	GtkWindow * r = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
	gtk_widget_show_all((GtkWidget *)r);
}

static gboolean c_download_name(WebKitDownload * d, gchar * fn
	,struct call_st * v)
{
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Save File"
		,(GtkWindow *) v->twin,GTK_FILE_CHOOSER_ACTION_SAVE ,("_Cancel")
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

void c_download_start(WebKitWebContext * wv
    ,WebKitDownload * dl, void * v)
{
	webkit_download_set_allow_overwrite(dl,TRUE);
    g_signal_connect(dl, "decide-destination"
        ,G_CALLBACK(c_download_name), v);
	g_signal_connect(dl, "created-destination"
        ,G_CALLBACK(c_download_destination_created), v);
}

/*GtkWindow * InitDownloadPrompt(gchar * fn)
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
