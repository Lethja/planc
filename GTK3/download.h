extern GtkWindow * InitDownloadPrompt(gchar * fn);

extern void c_download_start(WebKitWebContext * wv
    ,WebKitDownload * dl, void * v);

static gboolean c_download_name(WebKitDownload * d, gchar * fn
	,struct call_st * v);
	
extern void InitDownloadWindow(void * v);
