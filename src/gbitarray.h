/**
 * @file gbitarray.h
 *
 * @brief Header file for GBitArray Object.
 *
 *  Wed Oct 13 00:49:06 2004
 *  Copyright  2004  Alejandro Claro
 *  aleo@apollyon.no-ip.com
 */

#ifndef _GBITARRAY_H
#define _GBITARRAY_H

G_BEGIN_DECLS

/* MACROS *******************************************************************/

#define G_TYPE_BITARRAY             (g_bitarray_get_type())
#define G_BITARRAY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), G_TYPE_BITARRAY, GBitArray))
#define G_BITARRAY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  G_TYPE_BITARRAY, GBitArrayClass))
#define G_IS_BITARRAY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), G_TYPE_BITARRAY))
#define G_IS_BITARRAY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), G_TYPE_BITARRAY))
#define G_BITARRAY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), G_TYPE_BITARRAY, GBitArrayClass))

/* TYPEDEF ******************************************************************/

typedef struct _GBitArray GBitArray;
typedef struct _GBitArrayClass GBitArrayClass;

/**
 * @brief GBitArray object structure  
 */
struct _GBitArray
{
  GObject parent;

  /* private */
  guint size;
  gchar *array;
};

/**
 * @brief GBitArray Class structure 
 */
struct _GBitArrayClass
{
  GObjectClass parent_class;
};

/* PROTOTYPES ***************************************************************/

GType g_bitarray_get_type(void);
GObject *g_bitarray_new(guint p);

guint g_bitarray_get_size(GBitArray *bitarray);
void  g_bitarray_set_size(GBitArray *bitarray, guint p);

gboolean g_bitarray_get_bit(GBitArray *bitarray, guint bit);
gboolean g_bitarray_set_bit(GBitArray *bitarray, guint bit, gboolean state);

void g_bitarray_clear(GBitArray *bitarray);

G_END_DECLS

#endif /* _GBITARRAY_H */
