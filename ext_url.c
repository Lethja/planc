#include "config.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit-web-extension.h>
#include <sqlite3.h>
#define POLICYDIR(policydir) char * (policydir) \
= g_build_filename(g_get_user_config_dir() \
,"planc","policy",NULL)

static const char * selectDomainPolicy = "SELECT * FROM `POLICY` " \
"WHERE (`FROM` is ? OR `FROM` IS NULL) AND (`TO` is ? OR `TO` IS NULL)";
	
static const char * createPolicy = "PRAGMA synchronous=OFF;" \
"CREATE TABLE IF NOT EXISTS `POLICY`(`FROM` TEXT, `TO` TEXT," \
"`ALLOW` INTERGER);" \
"INSERT INTO `POLICY` (`FROM`, `TO`, `ALLOW`) VALUES (null, null, 0)" \
"WHERE NOT EXISTS(SELECT * FROM `POLICY`" \
"WHERE `FROM IS NULL AND `TO` IS NULL);";

#define g_debug g_print

/** Returns true if 'to' domains should load from `from` */
static gboolean domainLookup(gchar * from, gchar * to)
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

static gchar * removeWWW(gchar * dom)
{
	for(size_t i = 0; i != 2; i++)
	{
		if(tolower(dom[i]) != 'w')
			return dom;
	}
	if(dom[3] == '.' && dom[4] != '\0')
	return dom+4;
}

static gchar * getDomainName(const gchar * url)
{
	gchar * st = strstr(url,"://");
	if(st)
		st += 3;
	else
		return NULL;
	st = removeWWW(st);
	gchar * et = strchr(st,'/');
	guint len = 0;
	
	if(et)
		len = strlen(st) - strlen(et);
	else
		len = strlen(st);
	
	gchar * r = malloc(len+1);
	strncpy(r,st,len+1);
	r[len] = '\0';
	return r;
}

static gboolean c_webpage_send_request(WebKitWebPage * wp
	,WebKitURIRequest * req, WebKitURIResponse * res
	,gpointer user_data)
{
	if(res)
		return FALSE;
	const gchar * purl = webkit_web_page_get_uri(wp);
	const gchar * rurl = webkit_uri_request_get_uri(req);
	gchar * pdom = getDomainName(purl);
	gchar * rdom = getDomainName(rurl);
	if(pdom && rdom)
	{
		gboolean r;
		if(strcmp(pdom,rdom) == 0)
		{
			r = FALSE;
		}
		else
		{
			if(!domainLookup(pdom,rdom))
			{
				g_debug("DENY: '%s' > '%s'\n", pdom, rdom);
				r = TRUE;
			}
			else
			{
				g_debug("PASS: '%s' > '%s'\n", pdom, rdom);
				r = FALSE;
			}
		}
		free(pdom); free(rdom);
		//Dry run. Return r when fully implemented
		return r;
	}
	return FALSE;
}

static void
web_page_created_callback (WebKitWebExtension *extension,
	WebKitWebPage * wp, gpointer user_data)
{
	g_signal_connect (wp, "send-request"
		,G_CALLBACK (c_webpage_send_request), NULL);
}

static void createPolicyDatabase()
{
	sqlite3 * db;
	POLICYDIR(policydir);
	int rc;
	rc = sqlite3_open(policydir, &db);
	g_print("Created policy database '%s'",policydir);
	g_free(policydir);
	sqlite3_exec(db,createPolicy,NULL,NULL,NULL);
	sqlite3_close(db);
}

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
	//createPolicyDatabase();
    g_signal_connect (extension, "page-created"
		,G_CALLBACK (web_page_created_callback), NULL);
}
