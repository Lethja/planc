#include "config.h"
#include "libdomain.h"
#include "libdatabase.h"
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit-web-extension.h>

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
			if(sql_domain_policy_read(pdom,rdom))
			{
				g_debug("PASS: '%s' > '%s'\n", pdom, rdom);
				r = FALSE;
			}
			else
			{
				g_debug("DENY: '%s' > '%s'\n", pdom, rdom);
				r = TRUE;
			}
		}
		free(pdom); free(rdom);
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

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
    g_signal_connect (extension, "page-created"
		,G_CALLBACK (web_page_created_callback), NULL);
}
