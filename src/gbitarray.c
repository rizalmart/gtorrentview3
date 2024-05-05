/**
 * @file gbitarray.c
 *
 * @brief GBitArray Object functions
 *
 *  Wed Oct 13 01:19:00 2004
 *  Copyright  2004  Alejandro Claro
 *  aleo@apollyon.no-ip.com
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
#include "config.h"
#endif

#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef ENABLE_NLS
#define P_(String) dgettext(GETTEXT_PACKAGE "-properties",String)
#else 
#define P_(String) (String)
#endif

#include "gbitarray.h"

/* PRIVATE FUNCTIONS PROTOTYPES *********************************************/

static void g_bitarray_init(GTypeInstance *instance, gpointer g_class);
static void g_bitarray_class_init(gpointer g_class, gpointer class_data);
static void g_bitarray_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void g_bitarray_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);
static void g_bitarray_finalize(GObject *gobject);

/* MACROS *******************************************************************/

#define BITARRAY_BYTES(n)  ((n/8)+1)

/* TYPEDEF AND ENUMS ********************************************************/

enum
{
  PROP_SIZE = 1
};

/* GLOBALS ******************************************************************/

static gpointer parent_class;

/* FUNCTIONS ****************************************************************/

/**
 * @brief here register GBitArray type with the GObject 
 *        type system if it hasn't done so yet. 
 */
GType
g_bitarray_get_type(void)
{
  static GType g_bitarray_type = 0;

  if (!g_bitarray_type)
  {
    static const GTypeInfo g_bitarray_info =
    {
      sizeof(GBitArrayClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) g_bitarray_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(GBitArray),
      0,    /* n_preallocs */
      (GInstanceInitFunc) g_bitarray_init,
      NULL
    };

    g_bitarray_type = g_type_register_static(G_TYPE_OBJECT, "GBitArray",
                                              &g_bitarray_info, 0);
  }

  return g_bitarray_type;
}

/**
 * @brief  new GBitArray.
 *
 * @param p: the bit array size.
 * @return a new GBitArray Object.
 */
GObject *
g_bitarray_new(guint p)
{
 GBitArray *new = g_object_new(G_TYPE_BITARRAY, NULL); 
  
 g_bitarray_set_size(new, p);
  
 return G_OBJECT(new);
}

/**
 * @brief set some default properties.
 *
 * @param instance: the GBitArray.
 * @param g_class: the GBitArray Class.
 */
static void
g_bitarray_init(GTypeInstance *instance, gpointer g_class)
{
  GBitArray *self = G_BITARRAY(instance);
  
  self->size = 0;
  self->array = NULL;
  
  return;
}

/**
 *  @brief override the parent's functions that we need to implement.
 *
 * @param  g_class: the GBitArray Class.
 * @param  class_data: unused.
 */
static void
g_bitarray_class_init(gpointer g_lass, gpointer class_data)
{
  GBitArrayClass *klass = G_BITARRAY_CLASS(g_lass);
  GObjectClass   *object_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  object_class->get_property = g_bitarray_get_property;
  object_class->set_property = g_bitarray_set_property;

  object_class->finalize = g_bitarray_finalize;

  g_object_class_install_property(object_class, PROP_SIZE,
                                  g_param_spec_uint("size",
                                                    P_("Bit size"),
                                                    P_("Number of bit of array"),
                                                    0, G_MAXUINT, 0,
                                                    G_PARAM_READWRITE));

  return;
}

/**
 * @brief free resources when the last unref is call.
 *
 * @param object: the GBitArray.
 */
static void
g_bitarray_finalize(GObject *gobject)
{
  GBitArray *self = G_BITARRAY(gobject);

  if(self->array)
    g_free(self->array);
  
  (*G_OBJECT_CLASS(parent_class)->finalize)(gobject);
  return;
}

/**
 * @brief get the properties.
 */
static void
g_bitarray_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
  GBitArray *self = G_BITARRAY(object);

  switch (param_id)
  {
    case PROP_SIZE:
      g_value_set_uint(value, self->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
  }
  
  return;
}

/**
 * @brief set the properties.
 */
static void
g_bitarray_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
  GBitArray *self = G_BITARRAY(object);
  
  switch (param_id)
  {
    case PROP_SIZE:
      g_bitarray_set_size(self, g_value_get_uint(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
  }
  
  return;
}

/**
 * @brife get the array size in bits.
 *
 * @param bitarray: the GBitArray Object.
 * @return the array size in bytes.
 */
guint
g_bitarray_get_size(GBitArray *bitarray)
{  
  return bitarray->size;
}

/**
 * @brife set the array size in bits.
 *
 * @param bitarray: the GBitArray Object.
 * @param p: the new size in bits.
 */
void
g_bitarray_set_size(GBitArray *bitarray, guint p)
{
  bitarray->size = p;
  
  if(bitarray->array)
    bitarray->array = g_realloc(bitarray->array, BITARRAY_BYTES(p));
  else
    bitarray->array = g_malloc0(BITARRAY_BYTES(p));
  
  g_object_notify(G_OBJECT(bitarray), "size");
  return;  
}

/**
 * @brife get the state (ON or OFF) of a bit in a GBitArray.
 *
 * @param bitarray: the GBitArray Object.
 * @param bit: the bit number.
 * @return the state of the bit (FALSE if OFF, TRUE if ON).
 */
gboolean
g_bitarray_get_bit(GBitArray *bitarray, guint bit)
{
  gboolean is_on;
  
  if(bit > bitarray->size)
  {
    g_warning("%s", P_("Try to read a bit from bitarray beyond the array size"));
    is_on = FALSE;
  }
  else
    is_on = (bitarray->array[bit/8] & (0x01 << (8-1-(bit-((bit/8)*8)))))?TRUE:FALSE;
  
  return is_on;      
}

/**
 * @brife set the state (ON or OFF) of a bit in a GBitArray.
 *
 * @param bitarray: the GBitArray Object.
 * @param bit: the bit number.
 * @param state: the new bit state.
 * @return the state of the bit (FALSE if OFF, TRUE if ON).
 */
gboolean
g_bitarray_set_bit(GBitArray *bitarray, guint bit, gboolean state)
{
  gboolean is_on;
  
  if(bit > bitarray->size)
  {
    g_warning("%s", P_("Try to set a bit from bitarray beyond the array size"));
    is_on = FALSE;
  }
  else
  {
    if(state)
      bitarray->array[bit/8] |= (0x01 << (8-1-(bit-((bit/8)*8))));
    else
      bitarray->array[bit/8] &= ~(0x01 << (8-1-(bit-((bit/8)*8))));
    
    is_on = state;
  }
  
  return is_on;
}

/**
 * @brife put all the bit of a GBitArray to OFF state.
 *
 * @param bitarray: the GBitArray Object.
 */
void
g_bitarray_clear(GBitArray *bitarray)
{
  if(bitarray->array != NULL && bitarray->size > 0)
    memset(bitarray->array, 0, BITARRAY_BYTES(bitarray->size));
  
  return;
}
