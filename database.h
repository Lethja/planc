extern void sql_history_write(const char * url, const char * title);
extern void sql_download_write(const char * page, const char * url
	,const char * file);
extern void sql_history_read_to_tree(void * store);
extern void sql_download_read_to_tree(void * store);
extern char * sql_speed_dial_get(size_t index);
