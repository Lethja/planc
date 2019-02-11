#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libdomain.h"

static char * removeWWW(char * dom)
{
	for(size_t i = 0; i != 2; i++)
	{
		if(tolower(dom[i]) != 'w')
			return dom;
	}
	if(dom[3] == '.' && dom[4] != '\0')
		return dom+4;
	else
		return dom;
}

extern char * getDomainName(const char * url)
{
	char * st = strstr(url,"://");
	if(st)
		st += 3;
	else
		return NULL;
	st = removeWWW(st);
	char * et = strchr(st,'/');
	char * pv = strrchr(st,':');
	if(pv)
		et = pv;

	size_t len = 0;
	if(et)
		len = strlen(st) - strlen(et);
	else
		len = strlen(st);

	char * r = malloc(len+1);
	strncpy(r,st,len+1);
	r[len] = '\0';
	return r;
}
