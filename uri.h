/** Sanitize the address for webkit
 * Returned value never null and must always be freed after use
 * Will convert numbers to speed dial addresses if avaliable
**/
char * prepAddress(const char * c);
