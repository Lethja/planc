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
	char * r = (char *) haystack, * n = (char *) needles;
	size_t h_st = strlen(haystack), n_st = strlen(needles);
	if(!h_st || !n_st) //Early return
		return NULL;
	size_t n_max = n_st; //strlen(needles)
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
		h_st--;
		n = (char *) needles;
		n_st = n_max;
	} while (h_st);
	return NULL;
}

/**
 * Get the key for an implicit search
 */
static void ImplicitSearch(char ** uri, char ** searchPtr
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
static void EscapeChar(char ** str, char * position, const char * ins)
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
static size_t CountEscapeChar(const char * str)
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
static char * FormatQuery(char * url, char ** q)
{
	size_t s = strlen(url)+strlen(*q), i = strlen(url);
	size_t padding = CountEscapeChar(*q);
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
				EscapeChar(&r, r+i, "%3A");
			break;
		}
		i++;
	}
	return r;
}

enum AddressType DetermineAddressType(const char * uri)
{
	const char * it = uri;
	size_t digits = 0;
	if(*it == '\0')
		return Empty;

	do
	{
		switch(*it)
		{
			case ' ':
				return Search;
			case ':':
			case '.':
				return Address;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				digits++;
			break;
		}
		it++;
	} while(*it != '\0');

	if(strlen(uri) == digits)
		return SpeedDial;

	if(g_settings_get_boolean(G_SETTINGS, "planc-search-implicit"))
		return Search;
	return Unknown;
}

/** Prepare to attempt to find a search key in 'c'
 * or use implicit searching if enabled
 */
static char * SetupSearch(const char * c)
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
			if(!url && g_settings_get_boolean(G_SETTINGS
				,"planc-search-implicit"))
				ImplicitSearch(&url, &sp, c);
			else //Explicit search. Move sp over the first space
				sp++;
		}
	}
	else //Could be a implicit search, use default search if available
	{
		if(g_settings_get_boolean(G_SETTINGS, "planc-search-implicit"))
		/* Since we've already determined there's no space. Don't search
		 * if any symbols commonly used in addresses are found */
			if(!strchrany(c, ".:/\\"))
				ImplicitSearch(&url, &sp, c);
	}

	if(url)
	{
		r = FormatQuery(url, &sp);
		free(url);
	}
	return r;
}

static char * SetupAddressPath(const char * c)
{
	//Check if protocol portion of the url exists or add it
	char * p = strstr(c,"://");
	if(!p)
	{
		size_t s = strlen("http://")+strlen(c)+1;
		p = malloc(s);
		//Find out if this should be a file:// or http://
		if(c[0] == '/') //This is an absolute local path
			snprintf(p, s, "file://%s", c);
		else
			snprintf(p, s, "http://%s", c);
	}
	else
		p = strdup(c);
	return p;
}

/** Sanitize the address for webkit
 * Returned value never null and always must be freed after use
 * Will convert numbers to speed dial addresses if avaliable
**/
char * PrepAddress(const char * c)
{
	switch(DetermineAddressType(c))
	{
		case Address:
			return SetupAddressPath(c);
		case SpeedDial:
			return sql_speed_dial_get(atoi(c));
		case Search:
			return SetupSearch(c);
		default:
			return strdup("about:blank");
	}
}
