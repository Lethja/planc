#include <gtk/gtk.h>

struct menu_st
{
	GtkWidget *menu;
	GtkWidget *fileMenu;
	GtkWidget *fileMi;
	GtkWidget *quitMi;
};

struct tool_st
{
	GtkWidget *top;
	GtkToolItem *newTb;
	GtkToolItem *openTb;
	GtkToolItem *saveTb;
	GtkToolItem *sep;
	GtkToolItem *exitTb;
};

void InitToolbar(struct tool_st * tool)
{
	tool->top = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(tool->top), GTK_TOOLBAR_ICONS);

	tool->newTb = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_toolbar_insert(GTK_TOOLBAR(tool->top), tool->newTb, -1);

	tool->openTb = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_toolbar_insert(GTK_TOOLBAR(tool->top), tool->openTb, -1);

	tool->saveTb = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_toolbar_insert(GTK_TOOLBAR(tool->top), tool->saveTb, -1);

	tool->sep = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(tool->top), tool->sep, -1); 

	tool->exitTb = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_toolbar_insert(GTK_TOOLBAR(tool->top), tool->exitTb, -1);
}

void InitMenubar(struct menu_st * menu)
{
  menu->menu = gtk_menu_bar_new();
  menu->fileMenu = gtk_menu_new();

  menu->fileMi = gtk_menu_item_new_with_label("File");
  menu->quitMi = gtk_menu_item_new_with_label("Quit");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->fileMi), menu->fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu->fileMenu), menu->quitMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu->menu), menu->fileMi);
}

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *vbox;

	struct menu_st menu;
	struct tool_st tool;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
	gtk_window_set_title(GTK_WINDOW(window), "Simple menu & toolbar");

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	InitMenubar(&menu);
	InitToolbar(&tool);

	gtk_box_pack_start(GTK_BOX(vbox), menu.menu, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), tool.top, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(window), "destroy",
		G_CALLBACK(gtk_main_quit), NULL);

	g_signal_connect(G_OBJECT(menu.quitMi), "activate",
		G_CALLBACK(gtk_main_quit), NULL);  
	
	g_signal_connect(G_OBJECT(tool.exitTb), "clicked", 
		G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);

	gtk_main();

  return 0;
}
