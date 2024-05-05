/**
 * @file utilities.c
 *
 * @brief some utils functions
 *
 * Thu Sep 23 14:15:14 2004
 * Copyright (c) 2004  Alejandro Claro
 * aleo@apollyon.no-ip.com
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *****************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include "utilities.h"

/* FUNCTIONS ****************************************************************/

/**
 * @brief Convert some data to a Hex null terminated string representation.
 *
 * @param data: pointer to the data.
 * @param length: the length of the data to convert.
 * @param prefix: a null terminated the prefix to put in all the hex numbers.
          (can be NULL). 
 * @return a pointer to a new allocated string.
 */
gchar *
util_convert_to_hex(const gchar *data, guint length, const gchar *prefix)
{
  guint i, prefix_length;
  gchar *hex, *p;
  gchar ch;
  gchar list[] = "0123456789abcdef";
                    
  prefix_length = prefix?strlen(prefix):0;
  hex = g_malloc(1 + length*(2 + prefix_length));
  
  for(i=0, p=hex; i<length; i++)
  {
    if(prefix)
    {
      memmove(p, prefix, prefix_length);
      p += prefix_length;
    }
    
    ch = (gchar)(data[i] & 0xF0);
    ch = (gchar)(ch >> 4);
    ch = (gchar)(ch & 0x0F);    
    *(p++) = list[(gint)ch];
    
    ch = (gchar)(data[i] & 0x0F);
    *(p++) = list[(gint)ch];
  }
  
  hex[length*(2 + prefix_length)] = '\0';
  return hex;    
}

/**
 * @brief Convert a number to human readable.
 *
 * @param number: the number to convert.
 * @param suffix: the unit suffix to use.
 * @return a new allocated string.
 */
gchar *
util_convert_to_human(gdouble number, const gchar *suffix)
{
  gchar *string;
  gdouble new_number;
  gchar presuffix[] = "KMGT";
  guint i;
  
  for(i=0, new_number=number; i<4; i++)
    if(new_number>1024.0l)
      new_number /= 1024.0l;
    else
      break;      

    if(i==0)
      string = g_strdup_printf("%i %s", (gint)new_number, suffix?suffix:""); 
    else
      string = g_strdup_printf("%.2f %c%s", new_number, presuffix[i-1], suffix?suffix:""); 
    
  return string;
}

/**
 * @brief conver a BencNode list to a string with the list's elements
 *        separated by a delimiter.
 *
 * @param list: the BencNode list (have to be a BENC_TYPE_LIST type).
 * @param delimiter: the delimiter for the elements. (CAN'T be NULL)
 * @return a new allocated string.
 */
gchar *
util_convert_node_to_string(BencNode *list, gchar *delimiter)
{
  gchar *string, *tmp;
  BencNode *child;
  
  if(benc_node_type(list) != BENC_TYPE_LIST || benc_node_is_leaf(list))
    return NULL;
  
  child = benc_node_first_child(list);
  string = g_strdup(benc_node_data(child));
  
  for(child = benc_node_next_sibling(child); child != NULL;
      child = benc_node_next_sibling(child))
  {
    tmp = g_strdup_printf("%s%s%s", string, delimiter, benc_node_data(child));
    g_free(string);
    string = tmp;
  }
  
  return string;  
}

/**
 * @brief Load a picture from file. If can't it warning and return null
 *
 * @param name: the pixmap file name.
 * @return a new GdkPixBuf or NULL if can't load the file.
 */
GdkPixbuf *
util_get_pixbuf_from_file(const gchar *name)
{
  GdkPixbuf *picture;
  GError *err = NULL;
  
  picture = gdk_pixbuf_new_from_file(name, &err);
  
  if(err)
  {
    g_warning("%s", err->message);
    g_error_free(err);
    return NULL;
  }
  
  return picture;
}

/* END **********************************************************************/
