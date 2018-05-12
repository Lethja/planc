#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <glib.h>
#include "history.h"

static const char * createHistory = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `HISTORY`("  \
		"ADDRESS TEXT, TITLE TEXT," \
		"VISITED INTERGER, UNIQUE (`ADDRESS`));";

static const char * createDial = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `DIAL`("  \
		"DIAL INTERGER, URL TEXT," \
		"UNIQUE (`DIAL`));";

static const char * insert = "INSERT OR REPLACE INTO `HISTORY` " \
		"(`ADDRESS`,`TITLE`,`VISITED`)" \
		"VALUES (?1,?2,strftime('%s','now'));";

static const char * selectSpeedDial = "SELECT * FROM `DIAL`" \
		" WHERE `DIAL` is ?";

#define DIALDIR(dialdir) char * (dialdir) \
= g_build_filename(g_get_user_config_dir() \
,g_get_prgname(),"dial",NULL)

#define HISTORYDIR(historydir) char * (historydir) \
= g_build_filename(g_get_user_data_dir() \
,g_get_prgname(),"history",NULL)

static const char * retrieve = "SELECT * FROM `HISTORY`";

extern void sql_history_write(const char * url, const char * title)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;
	if(!url)
		return;
	HISTORYDIR(historydir);
	rc = sqlite3_open(historydir, &db);
	g_free(historydir);
	if( rc )
	{
		printf("Error opening history database: %s\n"
			,sqlite3_errmsg(db));
		return;
	}

	sqlite3_exec(db,createHistory,NULL,NULL,NULL);

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,insert,-1,&stmt,NULL);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error writing to history database0: %s\n"
			,zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return;
	}
	rc = sqlite3_bind_text(stmt,1,url,-1,SQLITE_STATIC);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error writing to history database1: %s\n"
			,sqlite3_errstr(rc));
	}
	rc = sqlite3_bind_text(stmt,2,title,-1,SQLITE_STATIC);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error writing to history database2: %s\n"
			,sqlite3_errstr(rc));
	}
	sqlite3_step(stmt);
    rc = sqlite3_finalize(stmt); if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error writing to history database3: %s\n"
			,sqlite3_errstr(rc));
	}
	sqlite3_close(db);
	return;
}

extern void * sql_history_search(char * c)
{
	sqlite3 *db;
	int rc;
	HISTORYDIR(historydir);
	rc = sqlite3_open(historydir, &db);
	g_free(historydir);
	if( rc )
	{
		printf("Error opening history database: %s\n"
			,sqlite3_errmsg(db));
		return NULL;
	}
}

extern void * sql_history_read_to_tree(void * store)
{
	sqlite3 *db;
	int rc;
	HISTORYDIR(historydir);
	rc = sqlite3_open(historydir, &db);
	g_free(historydir);
	if( rc )
	{
		printf("Error opening history database: %s\n"
			,sqlite3_errmsg(db));
		return NULL;
	}
	sqlite3_exec(db,retrieve,treeIter,store,NULL);
	sqlite3_close(db);
}

extern char * sql_speed_dial_get(size_t index)
{
	sqlite3 *db;
	int rc;
	DIALDIR(dialdir);
	rc = sqlite3_open(dialdir, &db);
	g_free(dialdir);

	sqlite3_exec(db,createDial,NULL,NULL,NULL);

	if( rc )
	{
		printf("Error opening dial database: %s\n"
			,sqlite3_errmsg(db));
		return NULL;
	}
	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,selectSpeedDial,-1,&stmt,NULL);
	rc = sqlite3_bind_int(stmt,1,index);
	rc = sqlite3_step(stmt);
	char * r = NULL;
	if(rc == SQLITE_ROW)
	{
		unsigned const char * u = sqlite3_column_text(stmt,1);
		r = malloc(strlen(u)+1);
		strncpy(r,u,strlen(u)+1);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return r;
}
