#include "config.h"
#include "libdatabase.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <glib.h>

static const char * createHistory = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `HISTORY`("  \
		"ADDRESS TEXT, TITLE TEXT," \
		"VISITED INTERGER, UNIQUE (`ADDRESS`));";

static const char * createDownload = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `DOWNLOAD`("  \
		"PAGE TEXT, URL TEXT," \
		"FILE TEXT, UNIQUE (`FILE`));";

static const char * createDial = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `DIAL`("  \
		"DIAL INTERGER, URL TEXT," \
		"UNIQUE (`DIAL`));";

static const char * insertHistory = "INSERT OR REPLACE INTO" \
		"`HISTORY` " \
		"(`ADDRESS`,`TITLE`,`VISITED`)" \
		"VALUES (?1,?2,strftime('%s','now'));";

static const char * insertDownload = "INSERT OR REPLACE INTO" \
		"`DOWNLOAD` " \
		"(`PAGE`,`URL`,`FILE`)" \
		"VALUES (?1,?2,?3);";

static const char * selectSpeedDial = "SELECT * FROM `DIAL`" \
		" WHERE `DIAL` is ?";

static const char * selectDomainPolicy = "SELECT * FROM `POLICY` " \
"WHERE (`FROM` is ? OR `FROM` IS NULL) AND (`TO` is ? OR `TO` IS NULL)";

static const char * createPolicy = "PRAGMA synchronous=OFF;" \
"CREATE TABLE IF NOT EXISTS `POLICY`(`FROM` TEXT, `TO` TEXT," \
"`ALLOW` INTERGER, `INHERIT` INTERGER);";

static const char * policyDefaultWhitelist = \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`, `INHERIT`)" \
"VALUES (null, null, 1, 0)";

static const char * policyDefaultBlacklist = \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`, `INHERIT`)" \
"VALUES (null, null, 0, 0)";

static const char * retrieveHistory = "SELECT * FROM `HISTORY`";

static const char * retrieveDownload = "SELECT * FROM `DOWNLOAD`";

/** Returns true if 'to' domains as allow to load from `from` */
extern gboolean sql_domain_policy_read(gchar * from, gchar * to)
{
	int r = 0; //Return deny incase of a error
	sqlite3 * db;
	int rc;
	POLICYDIR(policydir);
	rc = sqlite3_open(policydir, &db);
	g_free(policydir);

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,selectDomainPolicy,-1,&stmt,NULL);
	rc = sqlite3_bind_text(stmt,1,from,	-1,SQLITE_STATIC);
	rc = sqlite3_bind_text(stmt,2,to,	-1,SQLITE_STATIC);
	if(rc != SQLITE_OK)
		return FALSE;
	gboolean implicit = FALSE;
	while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		size_t score = 0;
		const unsigned char * f = sqlite3_column_text(stmt,0);
		if(f)
			score++;
		const unsigned char * t = sqlite3_column_text(stmt,1);
		if(t)
			score++;
		switch(score)
		{
			case 1: //This is an implicit rule
				implicit = TRUE;
				r = sqlite3_column_int(stmt,2);
				if(!r) //Deny from one implict rule conculudes
				{
					sqlite3_finalize(stmt);
					sqlite3_close(db);
					return FALSE;
				}
			break;
			case 0: //This is a global default rule
				if(!implicit)
					r = sqlite3_column_int(stmt,2);
			break;
			case 2: //This is an explicit rule
				r = sqlite3_column_int(stmt,2);
				sqlite3_finalize(stmt);
				sqlite3_close(db);
				if(r)
					return TRUE;
				else
					return FALSE;
		}
	}
	if(rc != SQLITE_DONE)
		return FALSE;
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	if(r)
		return TRUE;
	else
		return FALSE;
}

extern void sql_history_write(const char * url, const char * title)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
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
	rc = sqlite3_prepare_v2(db,insertHistory,-1,&stmt,NULL);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error writing to history database0: %s\n"
			,zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return;
	}
	rc = sqlite3_bind_text(stmt,1,url,-1,SQLITE_STATIC);
	rc = sqlite3_bind_text(stmt,2,title,-1,SQLITE_STATIC);
	sqlite3_step(stmt);
    rc = sqlite3_finalize(stmt);
	sqlite3_close(db);
	return;
}

void createPolicyDatabase(int t)
{
	sqlite3 * db;
	POLICYDIR(policydir);
	int rc;
	rc = sqlite3_open(policydir, &db);
	g_free(policydir);
	sqlite3_exec(db,createPolicy,NULL,NULL,NULL);
	if(t)
		sqlite3_exec(db,policyDefaultWhitelist,NULL,NULL,NULL);
	else
		sqlite3_exec(db,policyDefaultBlacklist,NULL,NULL,NULL);
	sqlite3_close(db);
}

extern void sql_download_write(const char * page, const char * url
	,const char * file)
{
	sqlite3 *db;
	char * zErrMsg = 0;
	int rc;
	if(!url)
		return;
	DOWNLOADDIR(downloaddir);
	rc = sqlite3_open(downloaddir, &db);
	g_free(downloaddir);
	if(rc)
	{
		printf("Error opening download database: %s\n"
			,sqlite3_errmsg(db));
		return;
	}

	sqlite3_exec(db,createDownload,NULL,NULL,NULL);

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,insertDownload,-1,&stmt,NULL);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr, "Error writing to download database: %s\n"
			,zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return;
	}
	rc = sqlite3_bind_text(stmt,1,page,-1,SQLITE_STATIC);
	rc = sqlite3_bind_text(stmt,2,url,-1,SQLITE_STATIC);
	rc = sqlite3_bind_text(stmt,3,file,-1,SQLITE_STATIC);
	sqlite3_step(stmt);
    rc = sqlite3_finalize(stmt);
	sqlite3_close(db);
	return;
}

extern void sql_download_read_to_tree(void * store, void * treeIter)
{
	sqlite3 *db;
	int rc;
	DOWNLOADDIR(downloaddir);
	rc = sqlite3_open(downloaddir, &db);
	g_free(downloaddir);
	if(rc)
	{
		printf("Error opening download database: %s\n"
			,sqlite3_errmsg(db));
		return;
	}
	sqlite3_exec(db,retrieveDownload,treeIter,store,NULL);
	sqlite3_close(db);
}

extern void sql_history_read_to_tree(void * store, void * treeIter)
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
		return;
	}
	sqlite3_exec(db,retrieveHistory,treeIter,store,NULL);
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

	if(rc)
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
		const char * u =
			(const char *) sqlite3_column_text(stmt,1);
		r = malloc(strlen(u)+1);
		strncpy(r,u,strlen(u)+1);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return r;
}
