## Process this file with automake to produce Makefile.in
#
# 2004 (c) Alejandro Claro
#

SUBDIRS = src pixmaps data po

pkgdata_DATA = README

EXTRA_DIST = AUTHORS                \
             ChangeLog              \
             NEWS                   \
             README                 \
             debian                 \
  	         intltool-extract.in 	\
 	         intltool-merge.in      \
	         intltool-update.in	

DISTCLEANFILES = intltool-extract   \
  	             intltool-merge     \
                 intltool-update

# erase the autom4te.cache when building a distribution
distclean-local:
	rm -rf autom4te.cache
