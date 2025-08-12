static void c_goto_dial(GtkMenuItem *mi, GdkEventButton *e, void *v);

static int gotoIter(void *store, int count, char **data, char **columns);

void c_show_gotoMenu(GtkMenuItem *mi, PlancWindow *v);

void c_hide_gotoMenu(GtkMenuItem *mi, PlancWindow *v);

void c_select_tabsMi(GtkWidget *w, struct dpco_st *dp);

gboolean c_onclick_tabsMi(GtkMenuItem *mi, GdkEventButton *e, struct dpco_st *dp);

void c_destroy_tabsMi(GtkMenuItem *mi, struct dpco_st *dp);

void c_hide_tabsMenu(GtkMenuItem *mi, PlancWindow *v);

static GtkRadioMenuItem *add_tabMi(gint it, GSList *list, struct call_st *c);

void c_show_tabsMenu(GtkMenuItem *mi, PlancWindow *v);

void t_tabs_menu(PlancWindow *v, gboolean b);
