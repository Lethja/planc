#include "main.h"
#include "libdatabase.h"
#include "uri.h"

static void strtcpy(char * dest, const char * src, const char token)
{
    size_t it = 0;
    while(src[it] != token)
    {
        dest[it] = src[it];
        it++;
    }
    dest[it] = '\0';
}

static char * strchrany(const char * haystack, const char * needles)
{
	char * r = (char *) haystack;
	char * n = (char *) needles;
	size_t h_st, n_st, n_max;
	h_st = strlen(haystack);
	n_st = n_max = strlen(needles);
	if(!h_st || !n_st)
		return NULL;

	do
	{
		do
		{
			if(*r == *n)
				return r;
			n++;
			n_st--;
		} while(n_st);
		r++;
		n = (char *) needles;
		h_st--;
		n_st = n_max;
	} while (h_st);
	return NULL;
}

static void implicitSearch(char ** uri, char ** searchPtr
	,const char * addr)
{
	if(!g_settings_get_boolean(G_SETTINGS
			,"planc-search-implicit"))
		return;

	char * key = g_settings_get_string(G_SETTINGS, "planc-search");
	if(key)
	{
		{
			if(strcmp(key," "))
				sql_search_get(key, NULL, uri);
			if(addr[0] == ' ')
				*searchPtr = (char *) addr+1;
			else
				*searchPtr = (char *) addr;
		}
		free(key);
	}
}

static char * setupSearch(const char * c)
{
	char * r = NULL, * url = NULL, * sp = NULL;

	sp = strchr(c, ' ');
	if(sp) //The first word could the a search provider key
	{
		gchar * key = malloc(strlen(c)-strlen(sp)+1);
		if(key)
		{
			strtcpy(key,c,' ');
			sql_search_get(key, NULL, &url);
			free(key);
			if(!url)
				goto setupSearch_tryImplicit;
			else //Explicit search. Move sp over the first space
				sp++;
		}
	}
	else //Implicit search, use default search if available
	{
setupSearch_tryImplicit:
		if(!strchrany(c, ".:/\\"))
			implicitSearch(&url, &sp, c);
	}

	if(url)
	{
		size_t s = strlen(url)+strlen(sp)+1;
		r = malloc(s);
		strcpy(r,url);
		strcat(r,sp);
		for(size_t i = strlen(url)+1; i < s; i++)
		{
			if(r[i] == ' ')
				r[i] = '+';
		}
		free(url);
	}
	return r;
}

/** Sanitize the address for webkit
 * Returned value never null and always must be freed after use
 * Will convert numbers to speed dial addresses if avaliable
**/
char * prepAddress(const char * c)
{
    char * p;
    if(strcmp(c,"") == 0 || strstr(c,"about:") == c)
    {
        p = malloc(strlen("about:blank")+1);
        strncpy(p,"about:blank",strlen("about:blank")+1);
        return p;
    }
    /*Check if speed dial or search entry
     * isdigit() means it's impossible to confuse with ipv4 (127.0.0.1)
    */
    p = strstr(c,"://");
    if(!p)
    {
        char * r = NULL;
        size_t i = 0;
        for(; i < strlen(c); i++)
        {
            if(!isdigit(c[i]))
                break;
        }
        if(i == strlen(c)) //This is a dial
        {
            r = sql_speed_dial_get(atoi(c));
        }
        else if(c[0] != '/') //This isn't an absolute directory
        {
			r = setupSearch(c);
        }
        if(!r)
            r = (char *) c;
        //Check if protocol portion of the url exists or add it
        p = strstr(r,"://");
        if(!p)
        {
            size_t s = strlen("http://")+strlen(r)+1;
            p = malloc(s);
            //Find out if this should be a file:// or http://
            if(r[0] == '/') //This is an absolute local path
                snprintf(p,s,"file://%s",r);
            else
                snprintf(p,s,"http://%s",r);
        }
        else
        {
            p = malloc(strlen(r)+1);
            memcpy(p,r,strlen(r)+1);
        }
        if(r != c)
            free(r);
    }
    else
    {
        p = malloc(strlen(c)+1);
        memcpy(p,c,strlen(c)+1);
    }
    return p;
}
