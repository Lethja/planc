#include <gtk/gtk.h>
#include "plan.h"

G_DEFINE_TYPE(PlancWindow, planc_window, GTK_TYPE_APPLICATION_WINDOW)

static void planc_window_class_init(PlancWindowClass* klass)
{
	
}

static void planc_window_init(PlancWindow* self)
{
	
}

PlancWindow * planc_window_new(GtkApplication * app)
{
	g_return_val_if_fail(GTK_IS_APPLICATION (app), NULL);
	PlancWindow * self = g_object_new(PLANC_TYPE_WINDOW
		,"application", app, NULL);
	return self;
}
