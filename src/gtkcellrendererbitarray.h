/**
 * @file gtkcellrendererbitarray.h
 *
 * @brief header file for Custom Cell Rederer for GBitArray.
 *
 * Sat Oct  9 01:46:41 2004
 * Copyright  2004  Alejandro Claro
 * aleo@apollyon.no-ip.com
 */

#ifndef _GTKCELLRENDERERBITARRAY_H
#define _GTKCELLRENDERERBITARRAY_H

G_BEGIN_DECLS

/* MACROS *******************************************************************/

#define GTK_TYPE_CELL_RENDERER_BITARRAY             (gtk_cell_renderer_bitarray_get_type())
#define GTK_CELL_RENDERER_BITARRAY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),GTK_TYPE_CELL_RENDERER_BITARRAY, GtkCellRendererBitarray))
#define GTK_CELL_RENDERER_BITARRAY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_CELL_RENDERER_BITARRAY, GtkCellRendererBitarrayClass))
#define GTK_IS_CELL_PROGRESS_BITARRAY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),GTK_TYPE_CELL_RENDERER_BITARRAY))
#define GTK_IS_CELL_PROGRESS_BITARRAY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_CELL_RENDERER_BITARRAY))
#define GTK_CELL_RENDERER_BITARRAY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_CELL_RENDERER_BITARRAY, GtkCellRendererBitarrayClass))

/* TYPEDEF ******************************************************************/

typedef struct _GtkCellRendererBitarray GtkCellRendererBitarray;
typedef struct _GtkCellRendererBitarrayClass GtkCellRendererBitarrayClass;

/**
 * @brief CellRendererPieces object structure  
 */
struct _GtkCellRendererBitarray
{
  GtkCellRenderer parent;

  /* private */
  guint first_bit, bits;
  GBitArray *bit_array;
};

/**
 * @brief CellRendererPieces Class structure 
 */
struct _GtkCellRendererBitarrayClass
{
  GtkCellRendererClass parent_class;
};

/* PROTOTYPES ***************************************************************/

GType gtk_cell_renderer_bitarray_get_type(void);
GtkCellRenderer *gtk_cell_renderer_bitarray_new(void);

G_END_DECLS

#endif /* _GTKCELLRENDERERBITARRAY_H */
