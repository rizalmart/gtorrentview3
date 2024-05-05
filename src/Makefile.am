## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir) \
  -DLOCALE_DIR=\""$(datadir)/locale"\" \
  -DPIXMAPS_DIR=\""$(pkgdatadir)/pixmaps"\" \
  -DSYSTEM_PIXMAPS_DIR=\""@PIXMAPDIR@"\" 

CFLAGS = @CFLAGS@ @CURL_CFLAGS@ @GTK_CFLAGS@

LDADD = @GTK_LIBS@ @CURL_LIBS@

bin_PROGRAMS = gtorrentviewer

gtorrentviewer_SOURCES = main.c \
              mainwindow.c \
              bencode.c \
              utilities.c \
              sha1.c \
              gbitarray.c \
              gtkcellrendererbitarray.c \
              inline_pixmaps.c

noinst_HEADERS = main.h \
                 mainwindow.h \
                 bencode.h \
                 utilities.h \
                 sha1.h \
                 gbitarray.h \
                 gtkcellrendererbitarray.h \
                 inline_pixmaps.h 

CLEANFILES      = *~
DISTCLEANFILES  = .deps/*.P

BUILT_SOURCES = inline_pixmaps.c

inline_pixmaps.c: $(top_srcdir)/pixmaps/about.png
	@echo "#include <gtk/gtk.h>" > inline_pixmaps.c
	gdk-pixbuf-csource --raw --extern --name=about_inline_pixmap $(top_srcdir)/pixmaps/about.png >> inline_pixmaps.c
