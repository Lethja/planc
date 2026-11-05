#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

extern GtkWindow *InitDownloadPrompt(gchar *fn);

extern void c_download_start(WebKitWebContext *wv, WebKitDownload *dl, void *v);

extern void InitDownloadWindow();

extern gchar *getFileNameFromPath(gchar *path);

extern gchar *getPathFromFileName(gchar *path);

extern void openFile(const gchar *file);
