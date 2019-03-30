#include "config.h"
#include <stdio.h>
#include <glib.h>
extern void sql_history_write(const char * url, const char * title);
extern void sql_download_write(const char * page, const char * url
	,const char * file);
extern void sql_history_read_to_tree(void * store, void * treeIter);
extern void sql_download_read_to_tree(void * store, void * treeIter);
extern void sql_search_read_to_tree(void * store, void * treeIter);
extern char sql_search_write(const char * key, const char * url
	,const char * name);
extern char sql_search_drop(const char * key);
extern char sql_history_clean(int64_t i);
extern char * sql_speed_dial_get(size_t index);
extern char * sql_speed_dial_get_by_name(const char * index);
extern void sql_speed_dial_read_to_menu(void * store, void * menuIter);
extern char sql_search_get(char * key, char ** name, char ** uri);
//Macros to quickly get string of database path, must free.

#define DIALDIR(dialdir) char * (dialdir) \
= g_build_filename(g_get_user_config_dir() \
,PACKAGE_NAME,"dial",NULL)

#define SEARCHDIR(searchdir) char * (searchdir) \
= g_build_filename(g_get_user_config_dir() \
,PACKAGE_NAME,"search",NULL)

#define HISTORYDIR(historydir) char * (historydir) \
= g_build_filename(g_get_user_data_dir() \
,PACKAGE_NAME,"history",NULL)

#define DOWNLOADDIR(downloaddir) char * (downloaddir) \
= g_build_filename(g_get_user_data_dir() \
,PACKAGE_NAME,"download",NULL)

#ifdef PLANC_FEATURE_DPOLC
#define POLICYDIR(policydir) char * (policydir) \
= g_build_filename(g_get_user_config_dir() \
,PACKAGE_NAME,"policy",NULL)

extern gboolean sql_domain_policy_read(gchar * from, gchar * to);
extern void createPolicyDatabase();
#endif
