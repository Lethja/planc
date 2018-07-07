#include <gtk/gtk.h>
#include "plan.h"

G_DEFINE_TYPE(PlancWindow, planc_window, GTK_TYPE_APPLICATION_WINDOW)

enum
{
  PROP_CALL,
  N_PROPERTIES
};

static void planc_window_set_property(GObject * object
	,guint property_id, const GValue *value, GParamSpec   *pspec)
{
	PlancWindow *self = PLANC_WINDOW(object);

	switch(property_id)
	{
    case PROP_CALL:
		self->call = g_value_get_pointer(value);		
	break;

    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	break;
    }
}

static void planc_window_get_property(GObject * object
	,guint property_id, GValue * value, GParamSpec *pspec)
{
	PlancWindow *self = PLANC_WINDOW(object);

	switch(property_id)
    {
    case PROP_CALL:
		g_value_set_pointer(value, self->call);
	break;

    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	break;
    }
}

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void planc_window_class_init(PlancWindowClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = planc_window_set_property;
	object_class->get_property = planc_window_get_property;

	obj_properties[PROP_CALL] =
    g_param_spec_pointer("call", "Call"
		,"The callback struct of this planc window"
		,G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
	//g_param_spec_pointer() //call_st
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
