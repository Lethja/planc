#include "config.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit-web-extension.h>
#include <sqlite3.h>

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
	{
		g_print("R: '%s'\n", webkit_uri_response_get_uri(res));
		return FALSE;
	}
	const gchar * purl = webkit_web_page_get_uri(wp);
	const gchar * rurl = webkit_uri_request_get_uri(req);
	gchar * pdom = getDomainName(purl);
	gchar * rdom = getDomainName(rurl);
	if(pdom && rdom)
	{
		//gboolean r;
		if(strcmp(pdom,rdom) == 0)
		{
			g_debug("A: '%s'\n", rdom);
			//r = FALSE;
		}
		else
		{
			g_debug("D: '%s'\n", rdom);
			//r = TRUE;
		}
		free(pdom); free(rdom);
		//Dry run. Return r when fully implemented
		return FALSE;
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

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
    g_signal_connect (extension, "page-created",
                      G_CALLBACK (web_page_created_callback),
                      NULL);
}
