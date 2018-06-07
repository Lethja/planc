gsettings_SCHEMAS = priv.dis.planc.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = planc.desktop

bin_PROGRAMS = planc
plancdir = $(bindir)
planc_SOURCES = main.c settings.c database.c history.c main.h settings.h database.h history.h download.c download.h
planc_CFLAGS= $(GTK3_CFLAGS) $(WK4_CFLAGS) $(SQ3_CFLAGS) -lm -DWKED=\""$(libdir)/planc/web-extension"\"
planc_LDFLAGS= $(GTK3_LIBS) $(WK4_LIBS) $(SQ3_LIBS)
EXTRA_DIST = $(desktop_DATA) $(gsettings_SCHEMAS)

webextension_LTLIBRARIES = ext_policy.la
webextensiondir = $(libdir)/planc/web-extension
ext_policy_la_SOURCES = ext_url.c
ext_policy_la_CFLAGS = $(WK4_CFLAGS) $(SQ3_CFLAGS)
ext_policy_la_LIBADD = $(WEB_EXTENSION_LIBS)
ext_policy_la_LDFLAGS = $(WK4_LIBS) $(SQ3_LIBS) -module -avoid-version -no-undefined
