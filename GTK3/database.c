#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <glib.h>

static const char * createHistory = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `HISTORY`("  \
		"ADDRESS TEXT, TITLE TEXT," \
		"VISITED INTERGER, UNIQUE (`ADDRESS`));";

static const char * insert = "INSERT OR REPLACE INTO `HISTORY` " \
		"(`ADDRESS`,`TITLE`,`VISITED`)" \
		"VALUES (?1,?2,strftime('%s','now'));";

extern void sql_history_write(const char * url, const char * title)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;
	if(!url)
		return;
	/* Open database */
	rc = sqlite3_open("history.db", &db);

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
	rc = sqlite3_bind_text(stmt,1,url,-1,SQLITE_STATIC);	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error writing to history database1: %s\n"
			,sqlite3_errstr(rc));
	}
	rc = sqlite3_bind_text(stmt,2,title,-1,SQLITE_STATIC);	if( rc != SQLITE_OK )
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
	rc = sqlite3_open("history.db", &db);
	if( rc )
	{
		printf("Error opening history database: %s\n"
			,sqlite3_errmsg(db));
		return NULL;
	}
}

extern void * sql_history_read()
{
	sqlite3 *db;
	int rc;
	rc = sqlite3_open("history.db", &db);
	if( rc )
	{
		printf("Error opening history database: %s\n"
			,sqlite3_errmsg(db));
		return NULL;
	}
}
