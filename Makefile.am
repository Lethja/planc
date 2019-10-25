gsettings_SCHEMAS = priv.dis.planc.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = planc.desktop

bin_PROGRAMS = planc
plancdir = $(bindir)
planc_SOURCES = main.c settings.c history.c main.h settings.h history.h download.c download.h plan.c plan.h uri.c uri.h
if GNOME
planc_SOURCES += gmenu.c gmenu.h
endif
if DMENU
planc_SOURCES += dmenu.c dmenu.h
endif
planc_CFLAGS = $(GTK3_CFLAGS) $(WK4_CFLAGS) -lm -DWKED=\""$(libdir)/planc/webkit"\"
planc_LDFLAGS = $(GTK3_LIBS) $(WK4_LIBS)
EXTRA_DIST = $(desktop_DATA) $(gsettings_SCHEMAS)

if DPOLC
planc_LDADD = libdomain.la libdatabase.la

extension_LTLIBRARIES = libdomain.la
extensiondir = $(libdir)/planc/
libdomain_la_SOURCES = libdomain.c libdomain.h
libdomain_la_CFLAGS = $(GTK3_CFLAGS)
libdomain_la_LDFLAGS = $(GTK3_LIBS)

database_LTLIBRARIES = libdatabase.la
databasedir = $(libdir)/planc/
libdatabase_la_SOURCES = libdatabase.c libdatabase.h
libdatabase_la_CFLAGS = $(GTK3_CFLAGS) $(SQ3_CFLAGS)
libdatabase_la_LDFLAGS = $(GTK3_LIBS) $(SQ3_LIBS)

webextension_LTLIBRARIES = libpolicy.la
webextensiondir = $(libdir)/planc/webkit
libpolicy_la_SOURCES = libpolicy.c
libpolicy_la_CFLAGS = $(WK4_CFLAGS) $(SQ3_CFLAGS)
libpolicy_la_LDFLAGS = $(WK4_LIBS) $(SQ3_LIBS) -module -avoid-version -no-undefined
libpolicy_la_LIBADD = libdomain.la libdatabase.la
else
planc_SOURCES += libdomain.c libdomain.h libdatabase.c libdatabase.h
planc_CFLAGS += $(SQ3_CFLAGS)
planc_LDFLAGS += $(SQ3_LIBS)
endif
