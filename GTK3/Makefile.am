gsettings_SCHEMAS = priv.dis.planc.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = planc.desktop

bin_PROGRAMS = planc
planc_SOURCES = main.c main.h
planc_CFLAGS= $(GTK3_CFLAGS) $(WK4_CFLAGS)
planc_LDFLAGS= $(GTK3_LIBS) $(WK4_LIBS)
