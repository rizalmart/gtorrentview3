## Process this file with automake to produce Makefile.in
#
# 2004 (c) Alejandro Claro
#

man_MANS = gtorrentviewer.1

desktopdir = @DESKTOPDIR@
desktop_in_files = gtorrentviewer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(man_MANS)		

CLEANFILES =	 			\
	$(desktop_DATA)
