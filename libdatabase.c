#include "libdatabase.h"
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define DB_CLOSE( rc, db, stmt, name ) \
fprintf(stderr, "Error while reading %s database: %s\n" \
	,name, sqlite3_errmsg(db)); \
sqlite3_finalize(stmt); \
sqlite3_close(db);

#define DB_IS_OR_RETURN_FALSE( rc, code, db, stmt, name ) \
if(rc != code) \
{ \
	DB_CLOSE( rc, db, stmt, name ) \
	return FALSE; \
}

#define DB_IS_OR_RETURN_NULL( rc, code, db, stmt, name ) \
if(rc != code) \
{ \
	DB_CLOSE( rc, db, stmt, name ) \
	return NULL; \
}

#define DB_IS_OR_RETURN( rc, code, db, stmt, name ) \
if(rc != code) \
{ \
	DB_CLOSE( rc, db, stmt, name ) \
	return; \
}

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
		"DIAL INTERGER, URL TEXT, NAME TEXT," \
		"UNIQUE (`DIAL`));";

static const char * createSearch = "PRAGMA synchronous=OFF;" \
		"CREATE TABLE IF NOT EXISTS `SEARCH`("  \
		"`KEY` TEXT, `URL` TEXT, `NAME` TEXT," \
		"UNIQUE (`KEY`) UNIQUE(`NAME`));";

static const char * insertHistory = "INSERT OR REPLACE INTO" \
		"`HISTORY` " \
		"(`ADDRESS`,`TITLE`,`VISITED`)" \
		"VALUES (?1,?2,strftime('%s','now'));";

static const char * insertDownload = "INSERT OR REPLACE INTO" \
		"`DOWNLOAD` " \
		"(`PAGE`,`URL`,`FILE`)" \
		"VALUES (?1,?2,?3);";

static const char * insertSearch = "INSERT OR REPLACE INTO" \
		"`SEARCH` " \
		"(`KEY`,`URL`,`NAME`)" \
		"VALUES (?1,?2,?3);";

static const char * selectSpeedDial = "SELECT * FROM `DIAL`" \
		" WHERE `DIAL` is ?";

static const char * selectSearchKey = "SELECT * FROM `SEARCH`" \
		" WHERE `KEY` is ?";

static const char * selectSpeedDialName = "SELECT * FROM `DIAL`" \
		" WHERE `URL` is ? OR `NAME` is ?";

static const char * deleteSearch =
		"DELETE FROM `SEARCH` WHERE `KEY` is ?";

static const char * retrieveSearch = "SELECT * FROM `SEARCH`";

static const char * retrieveHistory = "SELECT * FROM `HISTORY`"
"ORDER BY `VISITED` DESC";

static const char * retrieveDial = "SELECT * FROM `DIAL`";

static const char * retrieveDownload = "SELECT * FROM `DOWNLOAD`";

#ifdef PLANC_FEATURE_DPOLC
static const char * selectDomainPolicy = "SELECT * FROM `POLICY` " \
"WHERE (`FROM` IS ? OR `FROM` IS NULL) " \
"AND ((`TO` IS ? OR `TO` IS NULL)" \
"OR `INHERIT` IS 1)";

static const char * createPolicy = "PRAGMA synchronous=OFF;" \
"CREATE TABLE IF NOT EXISTS `POLICY`(`FROM` TEXT, `TO` TEXT," \
"`ALLOW` INTERGER, `INHERIT` INTERGER);";

static const char * policyDefaultWhitelist = \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`, `INHERIT`)" \
"VALUES (null, null, 1, 0)";

static const char * policyDefaultBlacklist = \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`, `INHERIT`)" \
"VALUES (null, null, 0, 0)";

/** Returns true if 'to' domains as allow to load from `from` */
extern gboolean sql_domain_policy_read(gchar * from, gchar * to)
{
	int r = 0; //Return deny incase of a error
	sqlite3 * db;
	int rc;
	POLICYDIR(policydir);
	rc = sqlite3_open(policydir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(policydir);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"Policy");
	sqlite3_stmt * stmt = NULL;
	rc = sqlite3_prepare_v2(db,selectDomainPolicy,-1,&stmt,NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Policy");
	rc = sqlite3_bind_text(stmt,1,from,	-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Policy");
	rc = sqlite3_bind_text(stmt,2,to,	-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Policy");

	gboolean implicit = FALSE;
	while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		size_t score = 0;
		const unsigned char * f = sqlite3_column_text(stmt,0);
		if(f && strcmp(from, (char *) f) == 0)
			score++;
		const unsigned char * t = sqlite3_column_text(stmt,1);
		if(t)
		{
			if(strcmp(to, (char *) t) == 0)
				score++;
			else if(sqlite3_column_int(stmt,3)) //If inherit
			{
				gchar * g = strstr(to, (char *) t);
				if(g && (char *) g+strlen(g) == (char *) to+strlen(to))
					score++;
				else
					continue;
			}
		}
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
				if(!f && !t && !implicit)
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
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_DONE,db,stmt,"Policy");
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	if(r)
		return TRUE;
	else
		return FALSE;
}

void createPolicyDatabase(int t)
{
	sqlite3 * db;
	POLICYDIR(policydir);
	int rc;
	rc = sqlite3_open(policydir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(policydir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Policy");
	rc = sqlite3_exec(db,createPolicy,NULL,NULL,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Policy");
	if(t)
		rc = sqlite3_exec(db,policyDefaultWhitelist,NULL,NULL,NULL);
	else
		rc = sqlite3_exec(db,policyDefaultBlacklist,NULL,NULL,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Policy");
	sqlite3_close(db);
}
#endif

extern void sql_history_write(const char * url, const char * title)
{
	sqlite3 *db;
	//char *zErrMsg = 0;
	int rc;
	if(!url)
		return;
	HISTORYDIR(historydir);
	rc = sqlite3_open(historydir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(historydir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"History");
	rc = sqlite3_exec(db,createHistory,NULL,NULL,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"History");
	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,insertHistory,-1,&stmt,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"History");
	rc = sqlite3_bind_text(stmt,1,url,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"History");
	rc = sqlite3_bind_text(stmt,2,title,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"History");
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return;
}

extern void sql_download_write(const char * page, const char * url
	,const char * file)
{
	sqlite3 *db;
	//char * zErrMsg = 0;
	int rc;
	if(!url)
		return;
	DOWNLOADDIR(downloaddir);
	rc = sqlite3_open(downloaddir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(downloaddir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Downloads");

	rc = sqlite3_exec(db,createDownload,NULL,NULL,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Downloads");
	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,insertDownload,-1,&stmt,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"Downloads");
	rc = sqlite3_bind_text(stmt,1,page,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"Downloads");
	rc = sqlite3_bind_text(stmt,2,url,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"Downloads");
	rc = sqlite3_bind_text(stmt,3,file,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,stmt,"Downloads");
	rc = sqlite3_step(stmt);
	DB_IS_OR_RETURN(rc,SQLITE_DONE,db,stmt,"Downloads");
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return;
}

extern char sql_search_write(const char * key, const char * url
	,const char * name)
{
	sqlite3 *db;
	int rc;
	SEARCHDIR(searchdir);
	rc = sqlite3_open(searchdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(searchdir);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"Search");
	rc = sqlite3_exec(db,createSearch,NULL,NULL,NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"Search");
	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,insertSearch,-1,&stmt,NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_bind_text(stmt,1,key,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_bind_text(stmt,2,url,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_bind_text(stmt,3,name,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_step(stmt);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_DONE,db,stmt,"Search");
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return 1;
}

extern char sql_search_drop(const char * key)
{
	sqlite3 *db;
	int rc;
	SEARCHDIR(searchdir);
	rc = sqlite3_open(searchdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(searchdir);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"Search");
	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db, deleteSearch, -1, &stmt, NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_bind_text(stmt,1,key,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_step(stmt);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_DONE,db,stmt,"Search");
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return 1;
}

extern void sql_download_read_to_tree(void * store, void * treeIter)
{
	sqlite3 *db;
	int rc;
	DOWNLOADDIR(downloaddir);
	rc = sqlite3_open(downloaddir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(downloaddir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Download");
	rc = sqlite3_exec(db,retrieveDownload,treeIter,store,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Download");
	sqlite3_close(db);
}

extern void sql_history_read_to_tree(void * store, void * treeIter)
{
	sqlite3 *db;
	int rc;
	HISTORYDIR(historydir);
	rc = sqlite3_open(historydir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(historydir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"History");
	rc = sqlite3_exec(db,retrieveHistory,treeIter,store,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"History");
	sqlite3_close(db);
}

extern void sql_search_read_to_tree(void * store, void * treeIter)
{
	sqlite3 *db;
	int rc;
	SEARCHDIR(searchdir);
	rc = sqlite3_open(searchdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(searchdir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Search");
	sqlite3_exec(db,createSearch,NULL,NULL,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Search");
	rc = sqlite3_exec(db,retrieveSearch,treeIter,store,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Search");
	sqlite3_close(db);
}

extern void sql_speed_dial_read_to_menu(void * store, void * menuIter)
{
	sqlite3 *db;
	int rc;
	DIALDIR(dialdir);
	rc = sqlite3_open(dialdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(dialdir);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Dial");
	rc = sqlite3_exec(db,retrieveDial,menuIter,store,NULL);
	DB_IS_OR_RETURN(rc,SQLITE_OK,db,NULL,"Dial");
	sqlite3_close(db);
}

extern char * sql_speed_dial_get_by_name(const char * index)
{
	sqlite3 *db;
	int rc;
	DIALDIR(dialdir);
	rc = sqlite3_open(dialdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(dialdir);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,NULL,"Dial");
	sqlite3_exec(db,createDial,NULL,NULL,NULL);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,NULL,"Dial");

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,selectSpeedDialName,-1,&stmt,NULL);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,stmt,"Dial");
	rc = sqlite3_bind_text(stmt,1,index,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,stmt,"Dial");
	rc = sqlite3_bind_text(stmt,2,index,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,stmt,"Dial");
	rc = sqlite3_step(stmt);
	char * r = NULL;
	if(rc == SQLITE_ROW)
	{
		const char * u =
			(const char *) sqlite3_column_text(stmt,1);
		r = malloc(strlen(u)+1);
		strcpy(r,u);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return r;
}

extern char * sql_speed_dial_get(size_t index)
{
	sqlite3 *db;
	int rc;
	DIALDIR(dialdir);
	rc = sqlite3_open(dialdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(dialdir);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,NULL,"Dial");
	sqlite3_exec(db,createDial,NULL,NULL,NULL);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,NULL,"Dial");

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,selectSpeedDial,-1,&stmt,NULL);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,stmt,"Dial");
	rc = sqlite3_bind_int(stmt,1,index);
	DB_IS_OR_RETURN_NULL(rc,SQLITE_OK,db,stmt,"Dial");
	rc = sqlite3_step(stmt);
	char * r = NULL;
	if(rc == SQLITE_ROW)
	{
		const char * u =
			(const char *) sqlite3_column_text(stmt,1);
		r = malloc(strlen(u)+1);
		strcpy(r,u);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return r;
}

extern char sql_search_get(char * key, char ** name, char ** uri)
{
	sqlite3 *db;
	int rc;
	SEARCHDIR(searchdir);
	rc = sqlite3_open(searchdir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(searchdir);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"Search");
	sqlite3_exec(db,createSearch,NULL,NULL,NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"Search");

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db,selectSearchKey,-1,&stmt,NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_bind_text(stmt,1,key,-1,SQLITE_STATIC);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"Search");
	rc = sqlite3_step(stmt);
	int r = 0;
	if(rc == SQLITE_ROW)
	{
		if(uri)
		{
			const char * u = (const char *) sqlite3_column_text(stmt,1);
			*uri = malloc(strlen(u)+1);
			if(!*uri)
			{
				sqlite3_finalize(stmt);
				sqlite3_close(db);
				return 0;
			}
			strcpy(*uri,u);
		}
		if(name)
		{
			const char * u = (const char *) sqlite3_column_text(stmt,2);
			*name = malloc(strlen(u)+1);
			if(!*name)
			{
				sqlite3_finalize(stmt);
				sqlite3_close(db);
				if(uri && *uri)
				{
					free(*uri);
				}
				return 0;
			}
			strcpy(*name,u);
		}
		r = 1;
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return r;
}
