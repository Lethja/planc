#include "config.h"
#include <stdio.h>
#include <glib.h>
extern gboolean sql_domain_policy_read(gchar * from, gchar * to);
extern void sql_history_write(const char * url, const char * title);
extern void sql_download_write(const char * page, const char * url
	,const char * file);
extern void sql_history_read_to_tree(void * store, void * treeIter);
extern void sql_download_read_to_tree(void * store, void * treeIter);
extern char * sql_speed_dial_get(size_t index);
extern void createPolicyDatabase();
//Macros to quickly get string of database path, must free.

#define DIALDIR(dialdir) char * (dialdir) \
= g_build_filename(g_get_user_config_dir() \
,PACKAGE_NAME,"dial",NULL)

#define HISTORYDIR(historydir) char * (historydir) \
= g_build_filename(g_get_user_data_dir() \
,PACKAGE_NAME,"history",NULL)

#define DOWNLOADDIR(downloaddir) char * (downloaddir) \
= g_build_filename(g_get_user_data_dir() \
,PACKAGE_NAME,"download",NULL)

#define POLICYDIR(policydir) char * (policydir) \
= g_build_filename(g_get_user_config_dir() \
,PACKAGE_NAME,"policy",NULL)
