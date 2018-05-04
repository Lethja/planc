gsettings_SCHEMAS = priv.dis.planc.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = planc.desktop

bin_PROGRAMS = planc
planc_SOURCES = main.c settings.c database.c main.h settings.h database.h
planc_CFLAGS= $(GTK3_CFLAGS) $(WK4_CFLAGS) $(SQ3_CFLAGS)
planc_LDFLAGS= $(GTK3_LIBS) $(WK4_LIBS) $(SQ3_LIBS)

EXTRA_DIST = $(desktop_DATA) $(gsettings_SCHEMAS)
