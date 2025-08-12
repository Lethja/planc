gsettings_SCHEMAS = priv.dis.planc.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = planc.desktop

bin_PROGRAMS = planc
plancdir = $(bindir)
planc_SOURCES = src/main.c src/settings.c src/history.c src/main.h src/settings.h src/history.h src/download.c src/download.h src/plan.c src/plan.h src/uri.c src/uri.h
if GNOME
planc_SOURCES += src/gmenu.c src/gmenu.h
endif
if DMENU
planc_SOURCES += src/dmenu.c src/dmenu.h
endif
planc_CFLAGS = $(GTK3_CFLAGS) $(WK4_CFLAGS) -lm -DWKED=\""$(libdir)/planc/webkit"\"
planc_LDFLAGS = $(GTK3_LIBS) $(WK4_LIBS)
EXTRA_DIST = $(desktop_DATA) $(gsettings_SCHEMAS)

if DPOLC
planc_LDADD = libdomain.la libdatabase.la

extension_LTLIBRARIES = libdomain.la
extensiondir = $(libdir)/planc/
libdomain_la_SOURCES = src/libdomain.c src/libdomain.h
libdomain_la_CFLAGS = $(GTK3_CFLAGS)
libdomain_la_LDFLAGS = $(GTK3_LIBS)

database_LTLIBRARIES = libdatabase.la
databasedir = $(libdir)/planc/
libdatabase_la_SOURCES = src/libdatabase.c src/libdatabase.h
libdatabase_la_CFLAGS = $(GTK3_CFLAGS) $(SQ3_CFLAGS)
libdatabase_la_LDFLAGS = $(GTK3_LIBS) $(SQ3_LIBS)

webextension_LTLIBRARIES = libpolicy.la
webextensiondir = $(libdir)/planc/webkit
libpolicy_la_SOURCES = src/libpolicy.c
libpolicy_la_CFLAGS = $(WK4_CFLAGS) $(SQ3_CFLAGS)
libpolicy_la_LDFLAGS = $(WK4_LIBS) $(SQ3_LIBS) -module -avoid-version -no-undefined
libpolicy_la_LIBADD = libdomain.la libdatabase.la
else
planc_SOURCES += src/libdomain.c src/libdomain.h src/libdatabase.c src/libdatabase.h
planc_CFLAGS += $(SQ3_CFLAGS)
planc_LDFLAGS += $(SQ3_LIBS)
endif
