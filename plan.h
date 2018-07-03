#ifndef __PLANC_WINDOWCONTAINER_H__
#define __PLANC_WINDOWCONTAINER_H__

#ifndef NO_INCLUDE_WITHIN_HEADERS
#include <gtk/gtk.h>
#endif

#define PLANC_TYPE_WINDOW_CONTAINER \
	(planc_window_container_get_type ())
#define PLANC_WINDOW_CONTAINER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANC_TYPE_WINDOW_CONTAINER \
	,PlancWindowContainer))
#define PLANC_WINDOW_CONTAINER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST  ((klass), PLANC_TYPE_WINDOW_CONTAINER \
	,PlancWindowContainerClass))
#define PLANC_IS_WINDOW_CONTAINER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANC_TYPE_WINDOW_CONTAINER))
#define PLANC_IS_WINDOW_CONTAINER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE  ((klass), PLANC_TYPE_WINDOW_CONTAINER))
#define PLANC_WINDOW_CONTAINER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS  ((obj), PLANC_TYPE_WINDOW_CONTAINER \
	,PlancWindowContainerClass))

typedef struct _PlancWindowContainer      PlancWindowContainer;
typedef struct _PlancWindowContainerClass PlancWindowContainerClass;

struct _PlancWindowContainer
{
    GtkApplicationWindow parent_instance;
};

struct _PlancWindowContainerClass
{
    GtkApplicationWindowClass parent_class;
};

GType planc_window_container_get_type(void);

//PlancWindowContainer* planc_windowcontainer_new(void);

#endif /* __PLANC_WINDOWCONTAINER_H__ */
