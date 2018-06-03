extern GtkWindow * InitDownloadPrompt(gchar * fn);

extern void c_download_start(WebKitWebContext * wv
    ,WebKitDownload * dl, void * v);
	
extern void InitDownloadWindow();
extern gchar * getFileNameFromPath(const gchar * path);
extern gchar * getPathFromFileName(const gchar * path);
