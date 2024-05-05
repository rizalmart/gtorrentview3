/**
 * @file mainwindow.c
 *
 * @brief Main Window Object Functions and CallBacks
 *
 * Thu Oct  7 02:32:35 2004
 * Copyright (C) 2023  Alejandro Claro
 * aleo@apollyon.no-ip.com
 * ported to GTK+3 by rizalmart
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

#include <time.h>
#include <stdarg.h>
#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>

#include "bencode.h"
#include "utilities.h"
#include "sha1.h"
#include "main.h"
#include "gbitarray.h"
#include "gtkcellrendererbitarray.h"
#include "mainwindow.h"

#include "inline_pixmaps.h"

/* PRIVATE FUNCTIONS PROTOTYPES *********************************************/

static void mainwindow_init(MainWindow *mwin);
static void mainwindow_class_init(MainWindowClass *klass);
static void mainwindow_finalize(GObject *gobject);

static void mainwindow_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time);
static gboolean mainwindow_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time);
static gboolean mainwindow_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time);

static void mainwindow_create_toolbar(MainWindow *mwin, GtkWidget *vbox);
static void mainwindow_create_general_tab(MainWindow *mwin, GtkWidget *notebook);
static void mainwindow_create_files_tab(MainWindow *mwin, GtkWidget *notebook);
static void mainwindow_create_torrentdetails_tab(MainWindow *mwin, GtkWidget *notebook);
static void mainwindow_create_trackersdetails_tab(MainWindow *mwin, GtkWidget *notebook);
static void mainwindow_create_log_tab(MainWindow *mwin, GtkWidget *notebook);
static void mainwindow_create_about_tab(MainWindow *mwin, GtkWidget *notebook);

static void mainwindow_signal_autoconnect(MainWindow *mwin);
static void mainwindow_drag_drop_signal_connect(GtkWidget *widget);

static void mainwindow_append_row_bencode_tree(GtkTreeStore *treestore, GtkTreeIter *parent, gchar *prefix, GdkPixbuf **icons, BencNode *data);

void cell_int64_to_human(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);

/* callbacks */
gboolean on_MainWindow_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_NewToolButton_clicked(MainWindow *mwin, gpointer user_data);
void on_OpenToolButton_clicked(MainWindow *mwin, gpointer user_data);
void on_SaveToolButton_clicked(MainWindow *mwin, gpointer user_data);
void on_QuitToolButton_clicked(MainWindow *mwin, gpointer user_data);
void on_RefreshSeedsButton_clicked(MainWindow *mwin, gpointer user_data);
void on_CheckFilesButton_clicked(MainWindow *mwin, gpointer user_data);
void on_RefreshTrackerButton_clicked(MainWindow *mwin, gpointer user_data);

/* DEFINES AND ENUMS ********************************************************/

enum
{
  TARGET_URI_LIST = 100
};

/* GLOBALS ******************************************************************/

static GtkTargetEntry drag_types[] =
{
  { "text/uri-list", 0, TARGET_URI_LIST },
};

static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);

static gpointer parent_class;

/* FUNCTIONS ****************************************************************/

/**
 * @brief get type function for MainWindow Object
 */
GType
mainwindow_get_type(void)
{
  static GType mwin_type = 0;

  if (!mwin_type)
  {
    static const GTypeInfo mwin_info =
    {
      sizeof (MainWindowClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) mainwindow_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (MainWindow),
      0,
      (GInstanceInitFunc) mainwindow_init,
      NULL
    };

    mwin_type = g_type_register_static (GTK_TYPE_WINDOW, "MainWindow", &mwin_info, 0);
  }

  return mwin_type;
}

/**
 * @brief method new for MainWindow Object
 */
GtkWidget*
mainwindow_new(void)
{
  return GTK_WIDGET(g_object_new(mainwindow_get_type(), NULL));
}

/**
 * @brief print a log message in the log tree view 
 *        and in the status bar.
 *
 * @param mwin: the MainWindow.
 * @param event_type: the event type (LOG_OK, LOG_WARNING or LOG_ERROR)
 * @param format: printf style format string.
 * @param ...: variables list.
 * @return the number of characters printed. 
 */
gint
mainwindow_log_printf(MainWindow const *mwin, gshort event_type, gchar const *format,...)
{
  va_list args;
  gint printed;
  GtkListStore *liststore;
  GtkTreeIter child;
  gchar *string, *msn, timestamp[10];
  time_t tim;

  va_start(args, format);
  printed = g_vasprintf(&msn, format, args);
  va_end(args);
  
  
  gtk_statusbar_pop(GTK_STATUSBAR(mwin->MainStatusBar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(mwin->MainStatusBar), 0, msn);

  tim = time(0);
  strftime(timestamp, sizeof(timestamp), "[%H:%M]", localtime(&tim));
  string = g_strdup_printf("%s %s", timestamp, msn);
  
  liststore = GTK_LIST_STORE(gtk_tree_view_get_model(mwin->LogTreeView));
  gtk_list_store_prepend(liststore, &child);
  gtk_list_store_set(liststore, &child, COL_ICON, mwin->log_icons[event_type], 
                     COL_TEXT, string, -1);

  g_free(msn);
  g_free(string);
  return printed;
}

/**
 * @brief Fill the General Tab
 *
 * @param mwin: the MainWindow.
 * @param torrent: the BencNode metainfo.
 */
void
mainwindow_fill_general_tab(MainWindow const *mwin, BencNode *torrent)
{
  GtkTextBuffer *text_buffer;
  BencNode *node;
  gchar *string, torrent_sha[SHA_DIGEST_LENGTH], date_string[100];
  guint number;
  GDate *date;

  /* name */
  node = benc_node_find_key(torrent, "name");
  gtk_entry_set_text(mwin->NameEntry, node!=NULL?benc_node_data(node):"");

  /* tracker announce */
  node = benc_node_find_key(torrent, "announce");
  gtk_entry_set_text(mwin->TrackerEntry, node!=NULL?benc_node_data(node):"");

  /* sha1 of info header */
  node = benc_node_find_key(torrent, "info");
  if(node != NULL)
  {
    string = benc_encode_buf(node, &number);
    SHA1((guint8*)string, number, (guint8*)torrent_sha);
    g_free(string);
    string = util_convert_to_hex(torrent_sha, SHA_DIGEST_LENGTH, NULL);      
    gtk_entry_set_text(mwin->SHAEntry, string);
    g_free(string);
  }
  else
    gtk_entry_set_text(mwin->SHAEntry, "");

  /* created by */
  node = benc_node_find_key(torrent, "created by");
  gtk_entry_set_text(mwin->CreatedEntry, node!=NULL?benc_node_data(node):"");

  /* comments */
  node = benc_node_find_key(torrent, "comment");
  text_buffer = gtk_text_view_get_buffer(mwin->CommentTextView);
  gtk_text_buffer_set_text(text_buffer, node!=NULL?benc_node_data(node):"", -1);

  /* date */
  node = benc_node_find_key(torrent, "creation date");
  if(node != NULL)
  {
    date = g_date_new();
    number = (guint)g_strtod(benc_node_data(node), (gchar**)NULL);
    g_date_set_time(date,(GTime)number);
    g_date_strftime(date_string, 100, "%x", date);
    gtk_entry_set_text(mwin->DateEntry, date_string);
    g_date_free(date);
  }
  else
    gtk_entry_set_text(mwin->DateEntry, "");

  /* clean the seeds, peers and dowloaded entry */
  gtk_entry_set_text(mwin->SeedEntry, "");
  gtk_entry_set_text(mwin->PeersEntry, "");
  gtk_entry_set_text(mwin->DownloadedEntry, "");

  /* activate the resfresh seed and peers button */
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshSeedsButton), TRUE);

  return;
}

/**
 * @brief Fill the Files Tab
 *
 * @param mwin: the MainWindow.
 * @param torrent: the BencNode metainfo.
 */
void
mainwindow_fill_files_tab(MainWindow const *mwin, BencNode *torrent)
{
  GtkListStore *liststore;
  GtkTreeIter child;
  GBitArray *bitarray;
  BencNode *node, *subnode, *value;
  gchar *string;
  gint files_number, i, total_pieces, piece_length, n_pieces;
  gint64 size;
  gdouble first_piece, total_size, tmp;

  files_number = 0;
  total_size = 0.0l;

  /* pieces */ 
  node = benc_node_find_key(torrent, "pieces");
  if(node != NULL)
  {
    total_pieces = benc_node_length(node)/SHA_DIGEST_LENGTH;
    string = g_strdup_printf("%i", total_pieces);
    gtk_entry_set_text(mwin->PiecesEntry, string);
    g_free(string);
  }
  else
  {
    total_pieces = 0;
    gtk_entry_set_text(mwin->PiecesEntry, "0");
  }

  bitarray = G_BITARRAY(g_bitarray_new(total_pieces));
  
  /* piece length */
  node = benc_node_find_key(torrent, "piece length");
  if(node != NULL)
  {
    piece_length = (gint)g_strtod(benc_node_data(node), (char**)NULL);
    string = util_convert_to_human((gdouble)piece_length, "B");
    gtk_entry_set_text(mwin->PieceLenEntry, string);
    g_free(string);
  }
  else
  {
    piece_length = 0;
    gtk_entry_set_text(mwin->PieceLenEntry, "0");
  }

  /* create the file list */  
  liststore = gtk_list_store_new(NUM_FILE_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                 G_TYPE_INT64, G_TYPE_UINT, G_TYPE_UINT,
                                 G_TYPE_INT64, G_TYPE_OBJECT);
  
  node = benc_node_find_key(torrent, "files");
  if(node == NULL) /* single file mode */
  {
    subnode = benc_node_find_key(torrent, "name");
    if(subnode != NULL)
    {
      files_number = 1;
      gtk_list_store_append(liststore, &child);
      gtk_list_store_set(liststore, &child, 
                      COL_FILE_ICON, mwin->file_state_icons[FILE_STATE_UNKNOWN],
                      COL_FILE_NAME, benc_node_data(subnode),
                      -1);

      subnode = benc_node_find_key(torrent, "length");
      total_size = subnode?(g_strtod(benc_node_data(subnode), (gchar**)NULL)):((gdouble)G_MAXUINT);  
      gtk_list_store_set(liststore, &child, COL_FILE_SIZE, (gint64)total_size, 
                         COL_FILE_FIRST_PIECE, 0, 
                         COL_FILE_N_PIECES, total_pieces, 
                         COL_FILE_REMAINS, ((gint64)-1),
                         COL_FILE_PIECESBITARRAY, bitarray,
                         -1);
    }
  } 
  else /* multi file mode */
  {
    total_size = 0.0l;
    first_piece = 0.0l;
    files_number = (gint)g_strtod(benc_node_data(node), (gchar**)NULL);
    for(i = 0; i<files_number; i++)
    {
      subnode = benc_node_nth_child(node, i);
      if(subnode == NULL)
        break;

      gtk_list_store_append(liststore, &child);

      value = benc_node_find_key(subnode, "path");
      if(value != NULL)
      {
        string = util_convert_node_to_string(value, DIRECTORY_DELIMITER);
        if(string != NULL)
        {
          gtk_list_store_set(liststore, &child, 
                      COL_FILE_ICON, mwin->file_state_icons[FILE_STATE_UNKNOWN], 
                      COL_FILE_NAME, string,
                      -1);
          g_free(string);
        }
      }
      
      value = benc_node_find_key(subnode, "length");
      if(value != NULL)
      {
        size = (gint64)g_strtod(benc_node_data(value), (gchar**)NULL);
        n_pieces = (piece_length==0)?0:(guint)ceil(modf(first_piece, &tmp) + ((gdouble)size)/piece_length);
        gtk_list_store_set(liststore, &child, COL_FILE_SIZE, (gint64)size,
                           COL_FILE_FIRST_PIECE, (guint)first_piece,
                           COL_FILE_N_PIECES, n_pieces,
                           COL_FILE_REMAINS, ((gint64)-1),
                           COL_FILE_PIECESBITARRAY, bitarray, 
                           -1);
        
        first_piece += (piece_length!=0)?((gdouble)size)/piece_length:0;
        total_size += (gdouble)size; 
      }
    }
  }

  gtk_tree_view_set_model(mwin->FilesTreeView, GTK_TREE_MODEL(liststore));
  g_object_unref(G_OBJECT(liststore));

  /* number of files */ 
  string = g_strdup_printf("%i", files_number);
  gtk_entry_set_text(mwin->FilesEntry, string);
  g_free(string); 

  /* total size */ 
  string = util_convert_to_human((gdouble)total_size,"B");
  gtk_entry_set_text(mwin->SizeEntry, string?string:"");
  g_free(string);

  /* activate the check file button */
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->CheckFilesButton), TRUE);

  g_object_unref(G_OBJECT(bitarray));
  return;
}

/**
 * @brief Fill the Trackers Details Tab
 *
 * @param mwin: the MainWindow.
 * @param torrent: the BencNode metainfo.
 */
void
mainwindow_fill_trackers_tab(MainWindow const *mwin, BencNode *torrent)
{
  GtkListStore *liststore;
  GtkTreeIter iter;
  BencNode *node, *subnode;
  
  liststore = gtk_list_store_new(1, G_TYPE_STRING);

  gtk_combo_box_set_active(mwin->TrackerComboBox, -1); 

  node = benc_node_find_key(torrent, "announce");
  gtk_list_store_append(liststore, &iter);
  gtk_list_store_set(liststore, &iter, 0, node!=NULL?benc_node_data(node):"", -1);

  node = benc_node_find_key(torrent, "announce-list");
  if(node != NULL) /* multi-tracker support */
  {
    for (node = benc_node_first_child(node); node != NULL;
         node = benc_node_next_sibling(node))
    {
      for(subnode = benc_node_first_child(node); subnode != NULL; 
          subnode = benc_node_next_sibling(subnode))
      {
        gtk_list_store_append(liststore, &iter);
        gtk_list_store_set(liststore, &iter, 0, benc_node_data(subnode), -1);
      }
    }
  }
  
  gtk_combo_box_set_model(mwin->TrackerComboBox, GTK_TREE_MODEL(liststore));
  g_object_unref(G_OBJECT(liststore));

  gtk_combo_box_set_active(mwin->TrackerComboBox, 0);
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshTrackerButton), TRUE);
  
  return;
}

/**
 * @brief Fill the Torrent Details Tab
 *
 * @param mwin: the MainWindow.
 * @param torrent: the BencNode metainfo.
 */
void
mainwindow_fill_torrent_tab(MainWindow const *mwin, BencNode *torrent)
{
  mainwindow_fill_bencode_tree(mwin, mwin->TorrentTreeView, torrent);
  return;
}

/**
 * @brief Fill the Torrent details or the Tracker details tree
 *        with Bencode Meta information.
 *
 * @param mwin: the MainWindow.
 * @param tree: the GtkTreeView to fill.
 * @param torrent: the BencNode Tree to be used.
 */
void
mainwindow_fill_bencode_tree(MainWindow const *mwin, GtkTreeView *tree, BencNode *torrent)
{
  GtkTreeStore *treestore;
  
  treestore = gtk_tree_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

  mainwindow_append_row_bencode_tree(treestore, NULL,
                      (benc_node_type(torrent) == BENC_TYPE_DICTIONARY
                      || benc_node_type(torrent) == BENC_TYPE_LIST)?"root":NULL,
                      (GdkPixbuf**)mwin->benc_icons, torrent);

  gtk_tree_view_set_model(tree, GTK_TREE_MODEL(treestore));
  g_object_unref(G_OBJECT(treestore));
  
  return;
}

/**
 * @brief Append rows to the Torrent details Tree or the Tracker details tree.
 *        It is a Internal funcion used just by mainwindow_fill_bencode_tree.
 *
 * @param treestore: the treestore to append a row.
 * @param parent: the parent of the item to append.
 * @param prefix: a prefix to prepend to the row text.
 * @param icons: the array of bencode type icons.
 * @param data: the BencNode item to append.
 */
static void
mainwindow_append_row_bencode_tree(GtkTreeStore *treestore, GtkTreeIter *parent, 
                                   gchar *prefix, GdkPixbuf **icons,
                                   BencNode *data)
{
  GtkTreeIter child;
  gchar *string, *node_data;
  BencNode *next_node;
  gboolean valid_utf8;
  
  node_data = NULL;
  next_node = data;
  
  while(next_node!=NULL)
  {
    valid_utf8 = g_utf8_validate(benc_node_data(next_node),
                                 benc_node_length(next_node),NULL);
    if(!valid_utf8) 
    {            
      string = util_convert_to_hex(benc_node_data(next_node),
                  MIN(benc_node_length(next_node), MAX_HEX_TO_SHOW_TREEVIEW), NULL);
      
      if(benc_node_length(next_node)>MAX_HEX_TO_SHOW_TREEVIEW)
      {
        node_data = g_strdup_printf("\"%s...\"", string);
        g_free(string);
      }
      else
        node_data = string;
    }
    else
    {
      node_data = g_strndup(benc_node_data(next_node),
                         MIN( benc_node_length(next_node),MAX_TREE_STRING_LEN));
    }
 
    if(benc_node_type(next_node) == BENC_TYPE_KEY)
    {
      mainwindow_append_row_bencode_tree(treestore, parent, node_data, icons,
                                         benc_node_first_child(next_node));       
    }
    else
    {
      gtk_tree_store_append(treestore, &child, parent);
      
      switch(benc_node_type(next_node))
      {
        case BENC_TYPE_INTEGER:
          string = g_strdup_printf("%s%s%s", prefix?prefix:"", prefix?" = ":"",
                                   node_data);
          break;
        case BENC_TYPE_STRING:
          string = g_strdup_printf("%s (%i)%s%s", prefix?prefix:"",
                     benc_node_length(next_node), prefix?" = ":" ", node_data);
          break;
        case BENC_TYPE_DICTIONARY:
          string = g_strdup_printf("%s%s{%s}", prefix?prefix:"", prefix?" ":"",
                                   node_data);
          break;
        default: /* BENC_TYPE_LIST */
          string = g_strdup_printf("%s%s[%s]", prefix?prefix:"", prefix?" ":"",
                                   node_data);
      }
      
      gtk_tree_store_set(treestore, &child, COL_ICON,
               icons[benc_node_type(next_node)], COL_TEXT, string, -1);
      
      g_free(string);
      
      if(!benc_node_is_leaf(next_node))
      {
        mainwindow_append_row_bencode_tree(treestore, &child, NULL, icons,
                                           benc_node_first_child(next_node));       
      }
    }
    
    g_free(node_data);
    next_node = benc_node_next_sibling(next_node);
  }
  
  return;
}

/**
 * @brief New Button CallBack
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param data: a pointer to the instance that fire the event(the New Button).
 */
void
on_NewToolButton_clicked(MainWindow *mwin, gpointer user_data)
{
  return;
}

/**
 * @brief Open Button CallBack
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param data: a pointer to the instance that fire the event(the Open Button).
 */
void
on_OpenToolButton_clicked(MainWindow *mwin, gpointer user_data)
{
  GtkWidget *dialog;
  gchar *filename, *lastdir;
  GError *err;
  
  dialog = gtk_file_chooser_dialog_new (_("Open File"), NULL,
                           GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
                           GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
                           GTK_RESPONSE_ACCEPT, NULL);
  
  
  lastdir = g_object_get_data(G_OBJECT(mwin), "lastdir");
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), lastdir?lastdir:g_get_home_dir());

  /* if click on anything diferent from OK here in nothing more to do */
  if(gtk_dialog_run(GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
  {
    gtk_widget_destroy(dialog);
    return;
  }

  /* get the selected file name */
  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  gtk_widget_destroy(dialog);

  /* remember the lastdir */
  g_object_set_data_full(G_OBJECT(mwin), "lastdir",
                         g_path_get_dirname(filename),
                         (GDestroyNotify)g_free);

  /* create the open file thread */
  if(g_thread_create(open_torrent_file, filename, FALSE, &err) == NULL)
  {
    g_warning(err->message);
    g_free(filename);
    g_error_free(err);
  } 

  return;
}

/**
 * @brief Save Button CallBack
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param data: a pointer to the instance that fire the event(the Save Button).
 */
void
on_SaveToolButton_clicked(MainWindow *mwin, gpointer user_data)
{
  return;
}

/**
 * @brief Refresh Seeds and Peers Button CallBack
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param user_data: a pointer to the instance that fire the event(the Refresh Button).
 */
void
on_RefreshSeedsButton_clicked(MainWindow *mwin, gpointer user_data)
{
  gchar *tracker;
  GError *err;

  tracker = g_strdup_printf("%s%s", gtk_entry_get_text(mwin->TrackerEntry),
                            "?info_hash=");

  if(g_thread_create(tracker_scrape, tracker, FALSE, &err) == NULL)
  {
    g_warning(err->message);
    g_free(tracker);
    g_error_free(err);
  } 
  
  return;  
}

/**
 * @brief Check Files Button CallBack
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param user_data: a pointer to the instance that fire the event(the Check Button).
 */
void
on_CheckFilesButton_clicked(MainWindow *mwin, gpointer user_data)
{
  GtkWidget *dialog;
  GError *err;
  GtkFileChooserAction action;
  gchar *filename, *title, *lastdir;
  guint number_files;

  filename = NULL;
  title = _("Open Folder");
  number_files = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(mwin->FilesTreeView), NULL);
  action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  
  switch(number_files)
  {
   case 0:
     break;
   case 1:
     title = _("Open File");
     action = GTK_FILE_CHOOSER_ACTION_OPEN;
   default:
     dialog = gtk_file_chooser_dialog_new (title, NULL,
                             action, GTK_STOCK_CANCEL,
                             GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
                             GTK_RESPONSE_ACCEPT, NULL);
  
     lastdir = g_object_get_data(G_OBJECT(mwin), "lastdir");
     gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), lastdir?lastdir:g_get_home_dir());

     /* if click on anything diferent from OK here in nothing more to do */
     if(gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
     {
       /* get the selected file or folder name */
       filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
       if(g_thread_create(check_files, filename, FALSE, &err) == NULL)
       {
         g_warning(err->message);
         g_free(filename); 
         g_error_free(err);
       }
     }
     gtk_widget_destroy(dialog);
  } 

  return;
}

/**
 * @brief Trackers Refresh Button CallBack
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param user_data: a pointer to the instance that fire the event (the Refresh Button).
 */
void
on_RefreshTrackerButton_clicked(MainWindow *mwin, gpointer user_data)
{
  GtkListStore *list;
  GtkTreeIter iter;
  GValue itemvalue = {0};
  gchar *tracker;
  GError *err;
  
  list = GTK_LIST_STORE(gtk_combo_box_get_model(mwin->TrackerComboBox));
  gtk_combo_box_get_active_iter(mwin->TrackerComboBox, &iter);
  gtk_tree_model_get_value(GTK_TREE_MODEL(list), &iter, 0, &itemvalue);
  tracker = g_strdup(g_value_get_string(&itemvalue));

  if(g_thread_create(tracker_scrape, tracker, FALSE, &err) == NULL)
  {
    g_warning(err->message);
    g_error_free(err);
  } 
  
  return;
}

/**
 * @brief Quit Button CallBack. synthesize delete_event to close the window.
 *
 * @param mwin: a pointer to the MainWindow. 
 * @param user_data: a pointer to the instance that fire the event(the Quit Button).
 */
void
on_QuitToolButton_clicked(MainWindow *mwin, gpointer user_data)
{
  GdkEventAny event;
  GtkWidget *widget;

  widget = GTK_WIDGET(mwin);
  
  event.type = GDK_DELETE;
  event.window = gtk_widget_get_window(widget);
  event.send_event = TRUE;

  g_object_ref(G_OBJECT(event.window));
  gtk_main_do_event((GdkEvent*)&event);
  g_object_unref(G_OBJECT(event.window));

  return;
}

/**
 * @brief MainWindow Detete Event CallBack
 *
 * @param widget: a pointer to the MainWindow. 
 * @param event: a pointer to the event structure.
 * @param user_data: a pointer to user data (not used).
 */
gboolean
on_MainWindow_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  gtk_main_quit();
  return FALSE;
}

/**
 * @brief Manage the drop event on MainWindow. 
 *        Open the torrent file when drop it inside MainWindow :)
 */
static void  
mainwindow_drag_data_received(GtkWidget *widget, GdkDragContext *context,
			                        gint x, gint y, GtkSelectionData *data,
			                        guint info, guint time)
{
  gchar *begin, *end, *filename, *tmp;
  GError *err;
  
  filename = NULL;
  
  gint length = gtk_selection_data_get_length(data);
  gint format = gtk_selection_data_get_format(data);
  auto data1 = gtk_selection_data_get_data(data);

  if((length >= 0) && (format == 8) && info == TARGET_URI_LIST)
  {
  	for(begin = (gchar*)data1; begin != NULL;)
    {
  		if (*begin != '#')
      {
  			while(g_ascii_isspace(*begin))
				  begin++;

   			for(end = begin; (*end != '\0') && (*end != '\n') && (*end != '\r');)
				  end++;

			  if (end > begin)
        {
				  for(end--; end > begin && g_ascii_isspace(*end);)
            end--;
          
				  tmp = g_strndup(begin, end-begin+1); 
          filename = g_filename_from_uri(tmp, NULL, NULL);
          g_free(tmp);
          break; 
			  }
        else
        {
          filename = NULL;
          break;
        }
		  }
      begin = strchr(begin, '\n');
		  if(begin != NULL)
			  begin++;      
    }

    /* create the open file thread */
    if(filename != NULL)
    {
      if(g_thread_create(open_torrent_file, filename, FALSE, &err) == NULL)
      {
        g_warning(err->message);
        g_free(filename);
        g_error_free(err);
      } 
    }
    
    gtk_drag_finish(context, TRUE, FALSE, time);
    return;
  }

  gtk_drag_finish(context, FALSE, FALSE, time);
  return;  
}

/**
 * @brief Override the gtk_WIDGET_drag_motion function to get URLs.
 *
 * @return FALSE if cann't find mime type in any target list. TRUE otherwise.
 */
static gboolean 
mainwindow_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x,
			                 gint y, guint time)
{
	GtkTargetList  *list;
	GtkWidgetClass *widget_class;
	gboolean        result;

	list = gtk_target_list_new(drag_types, n_drag_types);

	if (gtk_drag_dest_find_target(widget, context, list) != GDK_NONE) 
	{
		gdk_drag_status(context, gdk_drag_context_get_suggested_action(context), time);
		result = TRUE;
	} else
	{
		widget_class = GTK_WIDGET_GET_CLASS(widget);
		result = (*widget_class->drag_motion)(widget, context, x, y, time);
	}

	gtk_target_list_unref(list);
	return result;
}

/**
 * @brief Override the gtk_WIDGET_drag_drop function to get URLs.
 *
 * @return FALSE if cann't find mime type in any target list. TRUE otherwise.
 */
static gboolean 
mainwindow_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x,
			               gint y, guint time)
{
	GtkTargetList  *list;
	GtkWidgetClass *widget_class;
	gboolean        result;
	GdkAtom         target;

	list = gtk_target_list_new (drag_types, n_drag_types);

	target = gtk_drag_dest_find_target(widget, context, list);
	if (target != GDK_NONE)
	{
		gtk_drag_get_data(widget, context, target, time);
		result = TRUE;
	}
	else
	{
		widget_class = GTK_WIDGET_GET_CLASS(widget);
		result = (*widget_class->drag_drop)(widget, context, x, y, time);
	}

	gtk_target_list_unref(list);
	return result;
}

/**
 * @brief free resources when the last unref is call.
 *
 * @param object: the MainWindow.
 */
static void
mainwindow_finalize(GObject *gobject)
{
  guint i;
  MainWindow *self = MAINWINDOW(gobject);

  /* free file state icons */
  for(i=0; i<NUM_FILE_STATES; i++)
    if(self->file_state_icons[i] != NULL)
      g_object_unref(G_OBJECT(self->file_state_icons[i]));

  /* free log icons */
  for(i=0; i<NUM_LOG_EVENTS; i++)
    if(self->log_icons[i] != NULL)
      g_object_unref(G_OBJECT(self->log_icons[i]));

  /* free benc torrent and tracker details trees icons */
  for(i=0; i<BENC_TYPE_ALL; i++)
    if(self->benc_icons[i] != NULL)
      g_object_unref(G_OBJECT(self->benc_icons[i]));
  
  (*G_OBJECT_CLASS(parent_class)->finalize)(gobject);
  return;
}

/**
 *  @brief override the parent's functions that we need to implement.
 *
 * @param  klass: the MainWindow Class.
 */
static void
mainwindow_class_init(MainWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);
  
  object_class->finalize = mainwindow_finalize;
  
  return;
}

/**
 * @brief Create the widgets inside the main windows, and initialize them.
 *
 * This function is called when a MainWindows is created with mainwindow_new.
 *
 * @param mwin: pointer to the create object.
 */
static void
mainwindow_init(MainWindow *mwin)
{
  GdkPixbuf *icon_pixbuf;
  GtkWidget *vbox;
  GtkWidget *notebook;

  /* main window parent startup */
  gtk_window_set_title(GTK_WINDOW(mwin), MAINWINDOW_TITLE);
  gtk_window_set_type_hint (GTK_WINDOW(mwin), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_position(GTK_WINDOW(mwin), GTK_WIN_POS_CENTER);
  icon_pixbuf = util_get_pixbuf_from_file(MAINWINDOW_SYSTEM_ICON_FILE);
  
  /* if can't open the system icon try the icon in the pkgdata directory */
  if(icon_pixbuf == NULL)
  {
    icon_pixbuf = util_get_pixbuf_from_file(MAINWINDOW_ICON_FILE);
  }

  if(icon_pixbuf)
  {
    gtk_window_set_icon(GTK_WINDOW(mwin), icon_pixbuf);
    gdk_pixbuf_unref(icon_pixbuf);
  }

  /* make the widgets inside the main window */
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(mwin), vbox);

  /* create the tool bar */
  mainwindow_create_toolbar(mwin, vbox);

  /* create the notebook */
  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(notebook), 2);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);

  /* create General TAB */
  mainwindow_create_general_tab(mwin, notebook);
  /* create Files TAB */
  mainwindow_create_files_tab(mwin, notebook);
  /* create Torrent Details TAB */
  mainwindow_create_torrentdetails_tab(mwin, notebook);
  /* create Trackers Details TAB */
  mainwindow_create_trackersdetails_tab(mwin, notebook);
  /* create Log TAB */
  mainwindow_create_log_tab(mwin, notebook);
  /* create the About TAB */
  mainwindow_create_about_tab(mwin, notebook);

  /* create the MainWindow Status Bar */
  mwin->MainStatusBar = GTK_STATUSBAR(gtk_statusbar_new());
  gtk_widget_show(GTK_WIDGET(mwin->MainStatusBar));
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(mwin->MainStatusBar), FALSE, TRUE, 0);

  /* load the pixmaps used in the Torrent and Trackers Trees */
  mwin->benc_icons[BENC_TYPE_STRING] = util_get_pixbuf_from_file(STRING_ICON_FILE);
  mwin->benc_icons[BENC_TYPE_INTEGER] = util_get_pixbuf_from_file(INTEGER_ICON_FILE);
  mwin->benc_icons[BENC_TYPE_LIST] = util_get_pixbuf_from_file(LIST_ICON_FILE);
  mwin->benc_icons[BENC_TYPE_DICTIONARY] = util_get_pixbuf_from_file(DICTIONARY_ICON_FILE);
  mwin->benc_icons[BENC_TYPE_KEY] = NULL;

  /* load the pixmaps used in the Log list */
  mwin->log_icons[LOG_OK] = util_get_pixbuf_from_file(INFO_ICON_FILE); 
  mwin->log_icons[LOG_WARNING] = util_get_pixbuf_from_file(WARNING_ICON_FILE); 
  mwin->log_icons[LOG_ERROR] = util_get_pixbuf_from_file(ERROR_ICON_FILE); 

  /* load the pixmaps used in the Files list */
  mwin->file_state_icons[FILE_STATE_OK] = util_get_pixbuf_from_file(OK_ICON_FILE); 
  mwin->file_state_icons[FILE_STATE_UNKNOWN] = util_get_pixbuf_from_file(UNKNOWN_ICON_FILE); 
  mwin->file_state_icons[FILE_STATE_BAD] = mwin->log_icons[LOG_ERROR]; 
  g_object_ref(G_OBJECT(mwin->file_state_icons[FILE_STATE_BAD]));

  /* connect the widgets signals */
  mainwindow_signal_autoconnect(mwin);

  return;  
}

/**
 * @brief create and append the MainWindow ToolBar.
 *
 * @param mwin: the MainWindow.
 * @param vbox: the container.
 */
static void
mainwindow_create_toolbar(MainWindow *mwin, GtkWidget *vbox)
{
	
  GtkWidget *hbox, *alignment;
  GtkTooltip *tooltips;

  //tooltips = gtk_tooltip_new();

  hbox = gtk_hbox_new(FALSE, 3);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);

  mwin->NewToolButton = GTK_TOOL_BUTTON(gtk_tool_button_new_from_stock("gtk-new"));
  gtk_widget_show(GTK_WIDGET(mwin->NewToolButton));
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(mwin->NewToolButton), FALSE, TRUE, 0);
  gtk_widget_set_visible(GTK_WIDGET(mwin->NewToolButton), FALSE);
  gtk_widget_set_tooltip_text(GTK_TOOL_ITEM(mwin->NewToolButton), _("New"));

  mwin->OpenToolButton = GTK_TOOL_BUTTON(gtk_tool_button_new_from_stock("gtk-open"));
  gtk_widget_show(GTK_WIDGET(mwin->OpenToolButton));
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(mwin->OpenToolButton), FALSE, TRUE, 0);
  gtk_widget_set_tooltip_text(GTK_TOOL_ITEM(mwin->OpenToolButton), _("Open"));

  mwin->SaveToolButton = GTK_TOOL_BUTTON(gtk_tool_button_new_from_stock ("gtk-save-as"));
  gtk_widget_show(GTK_WIDGET(mwin->SaveToolButton));
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(mwin->SaveToolButton), FALSE, TRUE, 0);
  gtk_widget_set_visible(GTK_WIDGET(mwin->SaveToolButton), FALSE);
  gtk_widget_set_tooltip_text(GTK_TOOL_ITEM(mwin->SaveToolButton), _("Save As"));

  alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
  gtk_widget_show(alignment);
  gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);

  mwin->QuitToolButton = GTK_TOOL_BUTTON(gtk_tool_button_new_from_stock("gtk-quit"));
  gtk_widget_show(GTK_WIDGET(mwin->QuitToolButton));
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(mwin->QuitToolButton), FALSE, TRUE, 0);
  gtk_widget_set_tooltip_text(GTK_TOOL_ITEM(mwin->QuitToolButton), _("Quit"));

  return;
}

/**
 * @brief create and append the MainWindow General Tab.
 *
 * @param mwin: the MainWindow.
 * @param notebook: the container.
 */
static void
mainwindow_create_general_tab(MainWindow *mwin, GtkWidget *notebook)
{
  GtkWidget *vbox, *table1, *table2, *hbox1, *hbox2, *scrolledwindow1;
  GtkWidget *hseparator1, *alignment, *hbox3, *image;
  GtkWidget *label1, *label2, *label3, *label4, *label5, *label6, *label7;
  GtkWidget *label8, *label9, *label10;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(notebook), vbox);
  
  //gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK(notebook), vbox,
  //                                   FALSE, FALSE, GTK_PACK_START);
                                     
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), vbox, FALSE);                                    

  table1 = gtk_table_new(5, 2, FALSE);
  gtk_widget_show(table1);
  gtk_box_pack_start(GTK_BOX(vbox), table1, TRUE, TRUE, 0);

  label1 = gtk_label_new(_("Info hash:"));
  gtk_widget_show(label1);
  gtk_table_attach(GTK_TABLE(table1), label1, 0, 1, 1, 2,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment(GTK_MISC(label1), 1, 0.5);

  mwin->NameEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->NameEntry));
  gtk_table_attach(GTK_TABLE(table1), GTK_WIDGET(mwin->NameEntry), 1, 2, 0, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 2);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->NameEntry), FALSE);

  mwin->SHAEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->SHAEntry));
  gtk_table_attach(GTK_TABLE(table1), GTK_WIDGET(mwin->SHAEntry), 1, 2, 1, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->SHAEntry), FALSE);

  mwin->TrackerEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->TrackerEntry));
  gtk_table_attach(GTK_TABLE (table1), GTK_WIDGET(mwin->TrackerEntry), 1, 2, 2, 3,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 2);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->TrackerEntry), FALSE);

  label2 = gtk_label_new(_("Tracker:"));
  gtk_widget_show(label2);
  gtk_table_attach(GTK_TABLE(table1), label2, 0, 1, 2, 3,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 3);
  gtk_misc_set_alignment(GTK_MISC(label2), 1, 0.5);

  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox1);
  gtk_table_attach(GTK_TABLE(table1), hbox1, 1, 2, 3, 4,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (GTK_FILL), 3, 0);

  mwin->CreatedEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->CreatedEntry));
  gtk_box_pack_start(GTK_BOX(hbox1), GTK_WIDGET(mwin->CreatedEntry), TRUE, TRUE, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->CreatedEntry), FALSE);

  label3 = gtk_label_new(_("Date:"));
  gtk_widget_show(label3);
  gtk_box_pack_start(GTK_BOX(hbox1), label3, FALSE, FALSE, 2);
  gtk_misc_set_alignment(GTK_MISC(label3), 1, 0.5);

  mwin->DateEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show (GTK_WIDGET(mwin->DateEntry));
  gtk_box_pack_start(GTK_BOX(hbox1), GTK_WIDGET(mwin->DateEntry), FALSE, TRUE, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->DateEntry), FALSE);

  label4 = gtk_label_new(_("Created By:"));
  gtk_widget_show(label4);
  gtk_table_attach(GTK_TABLE(table1), label4, 0, 1, 3, 4,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment(GTK_MISC(label4), 1, 0.5);

  label5 = gtk_label_new(_("Comments:"));
  gtk_widget_show(label5);
  gtk_table_attach(GTK_TABLE(table1), label5, 0, 1, 4, 5,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 3);
  gtk_misc_set_alignment(GTK_MISC(label5), 1, 0.5);

  scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow1);
  gtk_table_attach(GTK_TABLE(table1), scrolledwindow1, 1, 2, 4, 5,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 3, 3);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow1),
                                      GTK_SHADOW_IN);

  mwin->CommentTextView = GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_widget_show(GTK_WIDGET(mwin->CommentTextView));
  gtk_container_add(GTK_CONTAINER(scrolledwindow1), GTK_WIDGET(mwin->CommentTextView));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(mwin->CommentTextView), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(mwin->CommentTextView), GTK_WRAP_WORD);

  label6 = gtk_label_new(_("Name:"));
  gtk_widget_show(label6);
  gtk_table_attach(GTK_TABLE(table1), label6, 0, 1, 0, 1,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 3);
  gtk_misc_set_alignment(GTK_MISC(label6), 1, 0.5);

  hseparator1 = gtk_hseparator_new();
  gtk_widget_show(hseparator1);
  gtk_box_pack_start(GTK_BOX(vbox), hseparator1, FALSE, FALSE, 3);

  hbox2 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 3);

  table2 = gtk_table_new(2, 4, FALSE);
  gtk_widget_show(table2);
  gtk_box_pack_start(GTK_BOX(hbox2), table2, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table2), 3);

  label7 = gtk_label_new(_("Peers:"));
  gtk_widget_show(label7);
  gtk_table_attach(GTK_TABLE(table2), label7, 0, 1, 1, 2,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment(GTK_MISC(label7), 1, 0.5);

  label8 = gtk_label_new(_("Downloaded:"));
  gtk_widget_show(label8);
  gtk_table_attach(GTK_TABLE(table2), label8, 2, 3, 1, 2,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment(GTK_MISC(label8), 1, 0.5);

  mwin->DownloadedEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->DownloadedEntry));
  gtk_table_attach(GTK_TABLE(table2), GTK_WIDGET(mwin->DownloadedEntry), 3, 4, 1, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->DownloadedEntry), FALSE);

  mwin->PeersEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->PeersEntry));
  gtk_table_attach(GTK_TABLE(table2), GTK_WIDGET(mwin->PeersEntry), 1, 2, 1, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->PeersEntry), FALSE);

  mwin->SeedEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->SeedEntry));
  gtk_table_attach(GTK_TABLE(table2), GTK_WIDGET(mwin->SeedEntry), 1, 2, 0, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 0, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->SeedEntry), FALSE);

  label9 = gtk_label_new(_("Seeds:"));
  gtk_widget_show(label9);
  gtk_table_attach(GTK_TABLE(table2), label9, 0, 1, 0, 1,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment(GTK_MISC(label9), 1, 0.5);

  mwin->RefreshSeedsButton = GTK_BUTTON(gtk_button_new());
  gtk_widget_show(GTK_WIDGET(mwin->RefreshSeedsButton));
  gtk_box_pack_start(GTK_BOX(hbox2), GTK_WIDGET(mwin->RefreshSeedsButton), FALSE, FALSE, 3);
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshSeedsButton), FALSE);

  alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment);
  gtk_container_add(GTK_CONTAINER(mwin->RefreshSeedsButton), alignment);

  hbox3 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox3);
  gtk_container_add(GTK_CONTAINER(alignment), hbox3);

  image = gtk_image_new_from_stock("gtk-refresh", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image);
  gtk_box_pack_start(GTK_BOX(hbox3), image, FALSE, FALSE, 0);

  mwin->RefreshSeedsButtonLabel = GTK_LABEL(gtk_label_new_with_mnemonic(_("_Refresh")));
  gtk_widget_show(GTK_WIDGET(mwin->RefreshSeedsButtonLabel));
  gtk_box_pack_start(GTK_BOX(hbox3), GTK_WIDGET(mwin->RefreshSeedsButtonLabel),
                     FALSE, FALSE, 0);

  label10 = gtk_label_new(_("General"));
  gtk_widget_show(label10);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK (notebook),
                 gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 0), label10);

  return;
}

/**
 * @brief create and append the MainWindow Files Tab.
 *
 * @param mwin: the MainWindow.
 * @param notebook: the container.
 */
static void
mainwindow_create_files_tab(MainWindow *mwin, GtkWidget *notebook)
{
  GtkWidget *vbox, *hbox1, *hbox2, *frame, *table, *image;
  GtkWidget *alignment1, *alignment2, *scrolledwindow;
  GtkWidget *label1, *label2, *label3, *label4, *label5, *label6, *label7;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkListStore *liststore;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(notebook), vbox);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 3);

  alignment1 = gtk_alignment_new(0.5, 0.5, 1, 1);
  gtk_widget_show(alignment1);
  gtk_container_add(GTK_CONTAINER(frame), alignment1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment1), 3, 3, 3, 3);

  scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow);
  gtk_container_add(GTK_CONTAINER(alignment1), scrolledwindow);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),
                                      GTK_SHADOW_IN);

  /* create and initialize files list */
  mwin->FilesTreeView = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_widget_show(GTK_WIDGET(mwin->FilesTreeView));
  gtk_container_add(GTK_CONTAINER(scrolledwindow), GTK_WIDGET(mwin->FilesTreeView));
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mwin->FilesTreeView), TRUE);

  col = gtk_tree_view_column_new(); /* column #1 */
	gtk_tree_view_column_set_title(col, _("Name"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COL_FILE_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COL_FILE_NAME, NULL);

  gtk_tree_view_column_set_resizable(col , TRUE);
	gtk_tree_view_append_column(mwin->FilesTreeView, col);
  
	col = gtk_tree_view_column_new(); /* column #2 */
	gtk_tree_view_column_set_title(col, _("Size"));

  renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func(col, renderer, cell_int64_to_human, 
                                          GINT_TO_POINTER(COL_FILE_SIZE), NULL);	

  gtk_tree_view_append_column(mwin->FilesTreeView, col);

	col = gtk_tree_view_column_new(); /* column #3 */
	gtk_tree_view_column_set_title(col, _("First Piece"));

  renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COL_FILE_FIRST_PIECE, NULL);
	
  gtk_tree_view_append_column(mwin->FilesTreeView, col);

	col = gtk_tree_view_column_new(); /* column #4 */
	gtk_tree_view_column_set_title(col, _("# Pieces"));

  renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COL_FILE_N_PIECES, NULL);
	
  gtk_tree_view_append_column(mwin->FilesTreeView, col);

	col = gtk_tree_view_column_new(); /* column #5 */
	gtk_tree_view_column_set_title(col, _("Remains"));

  renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func(col, renderer, cell_int64_to_human, 
                                          GINT_TO_POINTER(COL_FILE_REMAINS), NULL);	

  gtk_tree_view_append_column(mwin->FilesTreeView, col);

	col = gtk_tree_view_column_new(); /* column #6 */
	gtk_tree_view_column_set_title(col, _("Complete Pieces")); 

  renderer = gtk_cell_renderer_bitarray_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "first_bit", COL_FILE_FIRST_PIECE);  
  gtk_tree_view_column_add_attribute(col, renderer, "bits", COL_FILE_N_PIECES);
  gtk_tree_view_column_add_attribute(col, renderer, "bit_array", COL_FILE_PIECESBITARRAY);

  gtk_tree_view_append_column(mwin->FilesTreeView, col);

  liststore = gtk_list_store_new(NUM_FILE_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                 G_TYPE_INT64, G_TYPE_UINT, G_TYPE_UINT,
                                 G_TYPE_INT64, G_TYPE_OBJECT);
  gtk_tree_view_set_model(mwin->FilesTreeView, GTK_TREE_MODEL(liststore));
  g_object_unref(G_OBJECT(liststore));
  /* end initialize tree */

  label1 = gtk_label_new(_("Files list:"));
  gtk_widget_show(label1);
  gtk_frame_set_label_widget(GTK_FRAME(frame), label1);

  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox1);
  gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 3);

  table = gtk_table_new(2, 4, FALSE);
  gtk_widget_show(table);
  gtk_box_pack_start(GTK_BOX(hbox1), table, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 3);

  label2 = gtk_label_new(_("Total size:"));
  gtk_widget_show(label2);
  gtk_table_attach(GTK_TABLE(table), label2, 2, 3, 1, 2,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label2), 1, 0.5);

  label3 = gtk_label_new(_("Files:"));
  gtk_widget_show(label3);
  gtk_table_attach(GTK_TABLE(table), label3, 0, 1, 1, 2,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_misc_set_alignment(GTK_MISC(label3), 1, 0.5);

  mwin->FilesEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->FilesEntry));
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(mwin->FilesEntry), 1, 2, 1, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->FilesEntry), FALSE);

  mwin->SizeEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->SizeEntry));
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(mwin->SizeEntry), 3, 4, 1, 2,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->SizeEntry), FALSE);

  label4 = gtk_label_new(_("Pieces:"));
  gtk_widget_show(label4);
  gtk_table_attach(GTK_TABLE(table), label4, 0, 1, 0, 1,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 3, 3);
  gtk_misc_set_alignment(GTK_MISC(label4), 1, 0.5);

  label5 = gtk_label_new(_("Piece length:"));
  gtk_widget_show(label5);
  gtk_table_attach(GTK_TABLE(table), label5, 2, 3, 0, 1,
                   (GtkAttachOptions) (GTK_FILL),
                   (GtkAttachOptions) (0), 0, 3);
  gtk_misc_set_alignment(GTK_MISC(label5), 1, 0.5);

  mwin->PiecesEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->PiecesEntry));
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(mwin->PiecesEntry), 1, 2, 0, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->PiecesEntry), FALSE);

  mwin->PieceLenEntry = GTK_ENTRY(gtk_entry_new());
  gtk_widget_show(GTK_WIDGET(mwin->PieceLenEntry));
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(mwin->PieceLenEntry), 3, 4, 0, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions) (0), 3, 0);
  gtk_editable_set_editable(GTK_EDITABLE(mwin->PieceLenEntry), FALSE);

  mwin->CheckFilesButton = GTK_BUTTON(gtk_button_new());
  gtk_widget_show(GTK_WIDGET(mwin->CheckFilesButton));
  gtk_box_pack_start(GTK_BOX(hbox1), GTK_WIDGET(mwin->CheckFilesButton), FALSE, FALSE, 3);
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->CheckFilesButton), FALSE);

  alignment2 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment2);
  gtk_container_add(GTK_CONTAINER(mwin->CheckFilesButton), alignment2);

  hbox2 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox2);
  gtk_container_add(GTK_CONTAINER(alignment2), hbox2);

  image = gtk_image_new_from_stock("gtk-apply", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image);
  gtk_box_pack_start(GTK_BOX(hbox2), image, FALSE, FALSE, 0);

  label6 = gtk_label_new_with_mnemonic(_("_Check"));
  gtk_widget_show(label6);
  gtk_box_pack_start(GTK_BOX(hbox2), label6, FALSE, FALSE, 0);

  label7 = gtk_label_new(_("Files"));
  gtk_widget_show(label7);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
                 gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 1), label7);

  return;
}

/**
 * @brief create and append the MainWindow Torrent Details Tab.
 *
 * @param mwin: the MainWindow.
 * @param notebook: the container.
 */
static void
mainwindow_create_torrentdetails_tab(MainWindow *mwin, GtkWidget *notebook)
{
  GtkWidget *scrolledwindow, *label1;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow);
  gtk_container_add(GTK_CONTAINER(notebook), scrolledwindow);
  gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow), 3);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),
                                      GTK_SHADOW_IN);

  /* create and initialize Torrent Detailed Tree */ 
  mwin->TorrentTreeView = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_widget_show(GTK_WIDGET(mwin->TorrentTreeView));
  gtk_container_add(GTK_CONTAINER(scrolledwindow), GTK_WIDGET(mwin->TorrentTreeView));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mwin->TorrentTreeView), FALSE);

 	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "");

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COL_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COL_TEXT, NULL);

	gtk_tree_view_append_column(mwin->TorrentTreeView, col);
  /* end initialize Torrent Detailed tree */
  
  label1 = gtk_label_new(_("Torrent Details"));
  gtk_widget_show(label1);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
                 gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 2), label1);

  return;
}

/**
 * @brief create and append the MainWindow Trackers Details Tab.
 *
 * @param mwin: the MainWindow.
 * @param notebook: the container.
 */
static void
mainwindow_create_trackersdetails_tab(MainWindow *mwin, GtkWidget *notebook)
{
  GtkWidget *vbox, *hbox, *hbox2, *scrolledwindow, *label1, *label2;
  GtkWidget *alignment, *image;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(notebook), vbox);
  //gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK(notebook), vbox,
  //                                    FALSE, FALSE, GTK_PACK_START);
                                      
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), vbox, FALSE);                                      

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

  label1 = gtk_label_new(_("Tracker:"));
  gtk_widget_show(label1);
  gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 3);
  gtk_misc_set_alignment(GTK_MISC(label1), 1, 0.5);

  /* create and initialize the Trackers Combo box */
  mwin->TrackerComboBox = GTK_COMBO_BOX(gtk_combo_box_new());
  gtk_widget_show(GTK_WIDGET(mwin->TrackerComboBox));
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(mwin->TrackerComboBox), TRUE, TRUE, 3);

  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mwin->TrackerComboBox), renderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mwin->TrackerComboBox), renderer,
                                 "text", 0, NULL);
  /* end initialize Tracker Combo Box */

  mwin->RefreshTrackerButton = GTK_BUTTON(gtk_button_new());
  gtk_widget_show(GTK_WIDGET(mwin->RefreshTrackerButton));
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(mwin->RefreshTrackerButton), FALSE, FALSE, 3);
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshTrackerButton), FALSE);

  alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment);
  gtk_container_add(GTK_CONTAINER(mwin->RefreshTrackerButton), alignment);

  hbox2 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox2);
  gtk_container_add(GTK_CONTAINER(alignment), hbox2);

  image = gtk_image_new_from_stock("gtk-refresh", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image);
  gtk_box_pack_start(GTK_BOX(hbox2), image, FALSE, FALSE, 0);

  mwin->RefreshTrackerButtonLabel = GTK_LABEL(gtk_label_new_with_mnemonic(_("_Refresh")));
  gtk_widget_show(GTK_WIDGET(mwin->RefreshTrackerButtonLabel));
  gtk_box_pack_start(GTK_BOX(hbox2), GTK_WIDGET(mwin->RefreshTrackerButtonLabel),
                     FALSE, FALSE, 0);

  scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow);
  gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow), 3);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),
                                      GTK_SHADOW_IN);

  /* create and initialize Tracker Detailed Tree */
  mwin->TrackerTreeView = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_widget_show(GTK_WIDGET(mwin->TrackerTreeView));
  gtk_container_add(GTK_CONTAINER(scrolledwindow), GTK_WIDGET(mwin->TrackerTreeView));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mwin->TrackerTreeView), FALSE);

  col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "");

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COL_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COL_TEXT, NULL);

	gtk_tree_view_append_column(mwin->TrackerTreeView, col);
 /* end initialize Tracker Detailed Tree */
 
  label2 = gtk_label_new(_("Trackers Details"));
  gtk_widget_show(label2);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),
                 gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 3), label2);

  return;
}

/**
 * @brief create and append the MainWindow Log Tab.
 *
 * @param mwin: the MainWindow.
 * @param notebook: the container.
 */
static void
mainwindow_create_log_tab(MainWindow *mwin, GtkWidget *notebook)
{
  GtkWidget *scrolledwindow, *label1;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkListStore *liststore;

  scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolledwindow);
  gtk_container_add(GTK_CONTAINER(notebook), scrolledwindow);
  gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow), 3);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),
                                      GTK_SHADOW_IN);

  /* create and initialize the Log list */
  mwin->LogTreeView = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_widget_show(GTK_WIDGET(mwin->LogTreeView));
  gtk_container_add(GTK_CONTAINER(scrolledwindow), GTK_WIDGET(mwin->LogTreeView));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mwin->LogTreeView), FALSE);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mwin->LogTreeView), TRUE);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "");

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COL_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COL_TEXT, NULL);

	gtk_tree_view_append_column(mwin->LogTreeView, col);

  liststore = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
  gtk_tree_view_set_model(mwin->LogTreeView, GTK_TREE_MODEL(liststore));
  g_object_unref(G_OBJECT(liststore));
  /* end initialize the Log list */

  label1 = gtk_label_new(_("Log"));
  gtk_widget_show(label1);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
                 gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 4), label1);

  return;
}

/**
 * @brief create and append the MainWindow About Tab.
 *
 * @param mwin: the MainWindow.
 * @param notebook: the container.
 */
static void 
mainwindow_create_about_tab(MainWindow *mwin, GtkWidget *notebook)
{
  GdkPixbuf *image_pixbuf;
  GtkWidget *vbox, *image;
  GtkWidget *label1, *label2, *label3, *label4;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(notebook), vbox);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), vbox, FALSE); 

  image_pixbuf = gdk_pixbuf_new_from_inline(-1, about_inline_pixmap, FALSE, NULL);
  image = gtk_image_new_from_pixbuf(image_pixbuf);
  gtk_widget_show(image);
  gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);
  gdk_pixbuf_unref(image_pixbuf);

  label1 = gtk_label_new(_("<b><big>GTorrentViewer v"PACKAGE_VERSION"</big></b>"));
  gtk_widget_show(label1);
  gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 0);
  gtk_label_set_use_markup(GTK_LABEL(label1), TRUE);

  label2 = gtk_label_new(_("GTorrentViewer is a Viewer/Editor for .torrent " 
                           "files.\n\nIt is written using Gtk+ and is licensed "
                           "under the GPL."));
  gtk_widget_show(label2);
  gtk_box_pack_start(GTK_BOX(vbox), label2, TRUE, TRUE, 0);
  gtk_label_set_use_markup(GTK_LABEL (label2), TRUE);
  gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment(GTK_MISC(label2), 0.5, 0);
  gtk_misc_set_padding(GTK_MISC(label2), 0, 5);

  label3 = gtk_label_new(_("<small>Copyright (C) 2024 Claro Alejandro</small>"));
  gtk_widget_show(label3);
  gtk_box_pack_start(GTK_BOX(vbox), label3, FALSE, FALSE, 0);
  gtk_label_set_use_markup(GTK_LABEL(label3), TRUE);
  gtk_misc_set_padding(GTK_MISC(label3), 0, 5);

  label4 = gtk_label_new(_("About"));
  gtk_widget_show(label4);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),
                 gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 5), label4);

  return;
}

/**
 * @brief connect the widget signals.
 *
 * @param mwin: the MainWindow.
 */
static void
mainwindow_signal_autoconnect(MainWindow *mwin)
{
  /* delete event in MainWindow */
  g_signal_connect((gpointer)mwin, "delete_event",
                   G_CALLBACK(on_MainWindow_delete_event), NULL);
  
  /* clicked event in all buttons */
  g_signal_connect_swapped((gpointer)mwin->QuitToolButton, "clicked",
                           G_CALLBACK(on_QuitToolButton_clicked),
                           G_OBJECT(mwin));
  g_signal_connect_swapped((gpointer)mwin->NewToolButton, "clicked",
                           G_CALLBACK(on_NewToolButton_clicked),
                           G_OBJECT(mwin));
  g_signal_connect_swapped((gpointer)mwin->OpenToolButton, "clicked",
                           G_CALLBACK(on_OpenToolButton_clicked),
                           G_OBJECT(mwin));
  g_signal_connect_swapped((gpointer)mwin->SaveToolButton, "clicked",
                           G_CALLBACK(on_SaveToolButton_clicked),
                           G_OBJECT(mwin));
  g_signal_connect_swapped((gpointer)mwin->RefreshSeedsButton, "clicked",
                           G_CALLBACK(on_RefreshSeedsButton_clicked),
                           G_OBJECT(mwin));
  g_signal_connect_swapped((gpointer)mwin->CheckFilesButton, "clicked",
                           G_CALLBACK(on_CheckFilesButton_clicked),
                           G_OBJECT(mwin));
  g_signal_connect_swapped((gpointer)mwin->RefreshTrackerButton, "clicked",
                           G_CALLBACK(on_RefreshTrackerButton_clicked),
                           G_OBJECT(mwin));

	/* Drag and Drop support */
	gtk_drag_dest_set(GTK_WIDGET (mwin), GTK_DEST_DEFAULT_DROP |
                    GTK_DEST_DEFAULT_MOTION, drag_types, n_drag_types,
                    GDK_ACTION_COPY);
		
	g_signal_connect(G_OBJECT (mwin), "drag_data_received",
			             G_CALLBACK(mainwindow_drag_data_received), NULL);
	/*
	 * Get drag n drop signals before the standard TextView and Entry Widgets
   * functions to deal with url-lists.
	 */
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->NameEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->SHAEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->TrackerEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->CreatedEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->DateEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->SeedEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->PeersEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->DownloadedEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->PiecesEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->PieceLenEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->FilesEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->SizeEntry));
  mainwindow_drag_drop_signal_connect(GTK_WIDGET(mwin->CommentTextView));

  return;
}

/**
 * @brief override the drag n drop signal functions of text view 
 *        and entry widgets.
 *
 * @param widget: the widget to override signal with with mainwindow functions.
 */
static void
mainwindow_drag_drop_signal_connect(GtkWidget *widget)
{
  GObject *object = G_OBJECT(widget); 
 	GtkTargetList *list;
  
  list = gtk_drag_dest_get_target_list(widget);
  
  if(list != NULL)
  {
  	gtk_target_list_add_table(list, drag_types, n_drag_types);
	  g_signal_connect(object, "drag_data_received",
                     G_CALLBACK(mainwindow_drag_data_received), NULL);
  	g_signal_connect(object, "drag_motion", 
                     G_CALLBACK(mainwindow_drag_motion), NULL);
	  g_signal_connect(object, "drag_drop",
			               G_CALLBACK(mainwindow_drag_drop), NULL);
  }
  
  return;
}

/**
 * @brief render a unsigned int cell as human readable.
 *
 * @param tree_column: the GtkTreeViewColumn of the cell.
 * @param cell: the cell renderer.
 * @param tree_model: the GtkTreeModel.
 * @param iter: the GtkTreeIter row iteractor.
 * @param data: the column position number.
 */
void 
cell_int64_to_human(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, 
                  GtkTreeModel      *tree_model,  GtkTreeIter     *iter, 
                  gpointer           data)
{
  gint64 number;
  gchar *human;

  gtk_tree_model_get(tree_model, iter, GPOINTER_TO_INT(data), &number, -1);

  if(number < 0)
    g_object_set (cell, "text", "?", NULL);
  else
  {  
    human = util_convert_to_human((gdouble)number,"B");
    g_object_set (cell, "text", human, NULL);
    g_free (human);
  }

  return;
}
