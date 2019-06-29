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

static const char * cleanHistory = \
		"DELETE FROM `HISTORY` WHERE `VISITED` < ?";

static const char * retrieveSearch = "SELECT * FROM `SEARCH`";

static const char * retrieveHistory = "SELECT * FROM `HISTORY`"
"ORDER BY `VISITED` DESC";

static const char * retrieveDial = "SELECT * FROM `DIAL`";

static const char * retrieveDownload = "SELECT * FROM `DOWNLOAD`";

#ifdef PLANC_FEATURE_DPOLC
static const char * selectDomainPolicy = "SELECT * FROM `POLICY` " \
"WHERE (INSTR(?,`FROM`) OR `FROM` IS NULL) " \
"AND (INSTR(?,`TO`) OR `TO` IS NULL)";

static const char * createPolicy = "PRAGMA synchronous=OFF;" \
"CREATE TABLE IF NOT EXISTS `POLICY`(`FROM` TEXT, `TO` TEXT," \
"`ALLOW` INTERGER)";

static const char * policyDefaultWhitelist = \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`)" \
"VALUES (null, null, 1)";

static const char * policyDefaultBlacklist = \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`)" \
"VALUES (null, null, 0)";

enum policyflag //Policy byteflag
{
	POLIC = (1<<1),	//Allow or Deny
	RECRF = (1<<2),	//Accept subdomains from
	RECRT = (1<<3)	//Accept subdomains to
//	EXPLO = (1<<4)	//Explicit override
};

static size_t compareDomains(gchar * req, const unsigned char * dbr
	,char implicit)
{
	size_t score = 0;
	if(implicit)
	{
		char * r = strstr(req, dbr);
		if(r) //If there's a match at all give a point
			score++;
		else
			return score; //No match no points
		if (r == req) //If the match is at the start give another point
			score++;
		if (!strncmp(r, dbr, strlen(dbr))) //Identical, another point
			score++;
	}
	else
	{
		if (!strcmp(req, dbr)) //They're identical, 3 points
			score = 3;
	}
	return score;
}

static size_t compute_rule(gchar * reqf, gchar * reqt
	,const unsigned char * dbrf, const unsigned char * dbrt
	,int * allow)
{
	/*We compute the best rule to follow based on how many points it
	 * accumluates, the more explicit the rule is the more points it
	 * will get */
	size_t score = 0;
	//If the database entry in NULL then we give it one point
	if(!dbrf)
		score++;
	else //Score based on explicit infomation
		score += compareDomains(reqf, dbrf, *allow && RECRF);
	if(!dbrt)
		score++;
	else
		score += compareDomains(reqt, dbrt, *allow && RECRT);
	return score;
}

/** Returns true if 'to' domains as allow to load from `from` */
extern gboolean sql_domain_policy_read(gchar * from, gchar * to)
{
	int r = -1; //Return deny incase of a error
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

	size_t bestScore = 0;
	while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		const unsigned char * f = sqlite3_column_text(stmt,0);
		const unsigned char * t = sqlite3_column_text(stmt,1);
		int allow = sqlite3_column_int(stmt,2);
		size_t score = compute_rule(from, to, f, t, &allow);
		if(score == bestScore)
		{
			if (r != 0) //Deny should always overrule allow on equ score
				r = (allow && POLIC) > 0;
		}
		else if(score > bestScore)
		{
			bestScore = score;
			r = (allow && POLIC) > 0;
		}
		//g_debug("%s > %s = %zu. Best = %zu", f, t, score, bestScore);
	}
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_DONE,db,stmt,"Policy");
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	if(r == 1)
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

extern char sql_history_clean(int64_t i)
{
	sqlite3 *db;
	int rc;
	HISTORYDIR(historydir);
	rc = sqlite3_open(historydir, &db);
	sqlite3_busy_timeout(db, 5000);
	g_free(historydir);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,NULL,"History");
	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db, cleanHistory, -1, &stmt, NULL);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"History");
	rc = sqlite3_bind_int64(stmt,1,i);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_OK,db,stmt,"History");
	rc = sqlite3_step(stmt);
	DB_IS_OR_RETURN_FALSE(rc,SQLITE_DONE,db,stmt,"History");
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
