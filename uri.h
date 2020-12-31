/** Sanitize the address for webkit
 * Returned value never null and must always be freed after use
 * Will convert numbers to speed dial addresses if avaliable
**/
char * PrepAddress(const char * c);

enum AddressType
{
	Search, Address, SpeedDial, Unknown, Empty
};

/** Determine the type of input of this string */
enum AddressType DetermineAddressType(const char * uri);
