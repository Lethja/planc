#include <stdio.h>
#include <glib.h>
extern gboolean sql_domain_policy_read(gchar * from, gchar * to);
extern void sql_history_write(const char * url, const char * title);
extern void sql_download_write(const char * page, const char * url
	,const char * file);
extern void sql_history_read_to_tree(void * store, void * treeIter);
extern void sql_download_read_to_tree(void * store, void * treeIter);
extern char * sql_speed_dial_get(size_t index);
