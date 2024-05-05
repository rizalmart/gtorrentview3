/**
 * @file utilities.h
 *
 * @brief The header of utilities.c
 *
 * Thu Sep 23 14:16:33 2004
 * Copyright (c) 2004  Alejandro Claro
 * aleo@apollyon.no-ip.com
 */

#ifndef _UTILITIES_H
#define _UTILITIES_H

/* INCLUDES *****************************************************************/

#include <gtk/gtk.h>
#include "bencode.h"

/* PROTOTYPES ***************************************************************/

G_BEGIN_DECLS

gchar *util_convert_to_hex(const gchar *data, guint length, const gchar *prefix);
gchar *util_convert_to_human(gdouble number, const gchar *suffix);
gchar *util_convert_node_to_string(BencNode *list, gchar *delimiter);

GdkPixbuf *util_get_pixbuf_from_file(const gchar *name);

G_END_DECLS

#endif /* _UTILITIES_H */
