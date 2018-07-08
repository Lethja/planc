#include <gtk/gtk.h>
#include "plan.h"

G_DEFINE_TYPE(PlancWindow, planc_window, GTK_TYPE_APPLICATION_WINDOW)

/*static void planc_window_set_property(GObject * object
	,guint property_id, const GValue *value, GParamSpec   *pspec)
{

}

static void planc_window_get_property(GObject * object
	,guint property_id, GValue * value, GParamSpec *pspec)
{

}
*/

static void planc_window_class_init(PlancWindowClass* klass)
{
}

struct call_st * planc_window_get_call(PlancWindow * self)
{
	return self->call;
}

void planc_window_set_call(PlancWindow * self, struct call_st * call)
{
	self->call = call;
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
