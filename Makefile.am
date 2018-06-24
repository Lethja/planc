gsettings_SCHEMAS = priv.dis.planc.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = planc.desktop

bin_PROGRAMS = planc
plancdir = $(bindir)
planc_SOURCES = main.c settings.c database.c history.c main.h settings.h database.h history.h download.c download.h
planc_CFLAGS= $(GTK3_CFLAGS) $(WK4_CFLAGS) $(SQ3_CFLAGS) -lm -DWKED=\""$(libdir)/planc/webkit"\"
planc_LDFLAGS= $(GTK3_LIBS) $(WK4_LIBS) $(SQ3_LIBS)
planc_LDADD= libdomain.la
EXTRA_DIST = $(desktop_DATA) $(gsettings_SCHEMAS)

extension_LTLIBRARIES = libdomain.la
extensiondir = $(libdir)/planc/
libdomain_la_SOURCES = libdomain.c libdomain.h
libdomain_la_CFLAGS = $(GTK3_CFLAGS)
libdomain_la_LDFLAGS = $(GTK3_LIBS)

webextension_LTLIBRARIES = libpolicy.la
webextensiondir = $(libdir)/planc/webkit
libpolicy_la_SOURCES = libpolicy.c
libpolicy_la_CFLAGS = $(WK4_CFLAGS) $(SQ3_CFLAGS)
libpolicy_la_LDFLAGS = $(WK4_LIBS) $(SQ3_LIBS) -module -avoid-version -no-undefined
