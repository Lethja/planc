#include "main.h"
#include "libdatabase.h"
#include "uri.h"

/**
 * Like strncpy but stop the copy on a certian character
 * instead of a number, be warned it won't stop on '\0'
 */
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

/**
 * Return a pointer within 'haystack' on the first instance of
 * any chararcter within 'needles' being found
 * or 'NULL' if no 'needles' are found
 */
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
		} while (n_st);
		r++;
		n = (char *) needles;
		h_st--;
		n_st = n_max;
	} while (h_st);
	return NULL;
}

/**
 * Get the key for an implicit search
 */
static void implicitSearch(char ** uri, char ** searchPtr
	,const char * addr)
{
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

/**
 * Remove the character at 'position' and replace it with 'ins'
 * Make sure 'str' is big enough to fit the result
 */
static void escapeChar(char ** str, char * position, const char * ins)
{
	char * tmpcat = malloc(strlen(ins)+strlen(position)+1);
	strcpy(tmpcat, ins);
	strcat(tmpcat, position+1);
	strcpy(position, tmpcat);
	free(tmpcat);
}

/**
 * Calculate how big the EscapeChar version of the string will be
 * allows you to allocate enough space on the heap once rather then
 * constantly calling realloc()
 */
static size_t countEscapeChar(const char * str)
{
	size_t r = 0;
	char * i = (char *) str;
	while(i = strchrany(i, ":"))
	{
		r += 2;
		i++;
	}
	return r;
}

/**
 * Convert a human search query into http paramaters compatible escape
 * characters. This function is only used for detection, call escapeChar
 * to actually insert into the string
 */
static char * formatQuery(char * url, char ** q)
{
	size_t s = strlen(url)+strlen(*q), i = strlen(url);
	size_t padding = countEscapeChar(*q);
	char * r = malloc(s+padding+1);
	strcpy(r,url);
	strcat(r,*q);
	while(r[i] != '\0')
	{
		switch(r[i])
		{
			case ' ':
				r[i] = '+';
			break;
			case ':':
				escapeChar(&r, r+i, "%3A");
			break;
		}
		i++;
	}
	return r;
}

/** Prepare to attempt to find a search key in 'c'
 * or use implicit searching if enabled
 */
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
	else //Could be a implicit search, use default search if available
	{
setupSearch_tryImplicit:
		if(g_settings_get_boolean(G_SETTINGS, "planc-search-implicit"))
			if(!strchrany(c, ".:/\\"))
				implicitSearch(&url, &sp, c);
	}

	if(url)
	{
		r = formatQuery(url, &sp);
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
            r = sql_speed_dial_get(atoi(c));
        else if(c[0] != '/') //This isn't an absolute directory
			r = setupSearch(c);

        if(!r) //Not a search query
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
