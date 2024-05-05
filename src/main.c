/**
 * @file main.c
 *
 * @brief main functions
 *
 * Thu Oct  7 00:59:28 2004
 * Copyright  2004  Alejandro Claro
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
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>

#include <curl/curl.h>
#include <curl/easy.h> 

#include "bencode.h"
#include "utilities.h"
#include "mainwindow.h"
#include "gbitarray.h"
#include "sha1.h"
#include "main.h"

/* MACROS *******************************************************************/

#ifdef HAVE_FSEEKO
# define _fseeko fseeko
#else
# define _fseeko fseek
#endif

#define log_ok(format, args...)        mainwindow_log_printf(MAINWINDOW(gmainwin), LOG_OK, format, ## args)
#define log_warning(format, args...)   mainwindow_log_printf(MAINWINDOW(gmainwin), LOG_WARNING, format, ## args)
#define log_error(format, args...)     mainwindow_log_printf(MAINWINDOW(gmainwin), LOG_ERROR, format, ## args)

/* PRIVATE FUNCTIONS ********************************************************/

static void display_usage(void);
static void parse_cmd_line(gint argc, gchar **argv);

/* GLOBALS ******************************************************************/

static GtkWidget *gmainwin = NULL;
static gchar *gfilename = NULL;
static BencNode *gtorrentmetainfo = NULL;

gboolean gissaved = TRUE;

G_LOCK_DEFINE_STATIC(thread_mutex);
static GThread *scrape_thread = NULL;
static GThread *checkfiles_thread = NULL;

static gboolean scrape_cancel = FALSE;
static gboolean checkfiles_cancel = FALSE;

/* MAIN *********************************************************************/

int
main(int argc, char *argv[])
{
  /* Init GTK */
  gtk_init(&argc, &argv);

#ifdef ENABLE_NLS
  bindtextdomain(PACKAGE_NAME, LOCALE_DIR);
  bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
  textdomain(PACKAGE_NAME);
#endif
  
  /* check GTK version */
  if (!GTK_CHECK_VERSION(3, 0, 0))
  {
    g_printerr(_("Sorry, your GTK+ version (%d.%d.%d) does not work with GTorrentViewer.\n"
                 "Please use GTK+ %s or higher.\n"), gtk_major_version, gtk_minor_version,
                 gtk_micro_version, "3.0.0");
    exit(EXIT_FAILURE);
  }
  
  /* Init Thread Support */
  if(!g_thread_supported()) 
    g_thread_init(NULL);
  
  gdk_threads_init();

  /* Init LibCurl */
  curl_global_init(CURL_GLOBAL_NOTHING);
  g_atexit(curl_global_cleanup);

  /* parse command line options */
  parse_cmd_line(argc, argv);
  
  /* Create Main Window */
  gmainwin = mainwindow_new();

  /* open command line file if needed */  
  if(gfilename)
  {
    log_ok(_("Command line file option: %s."), gfilename);
    open_torrent_file(gfilename);
  }
  
  /* Enter the Main loop */
  gdk_threads_enter();;
  log_ok("%s", LOG_WELCOME_MSN);  
  gtk_widget_show(gmainwin);
  gtk_main();
  gdk_threads_leave();
 
  /* free any allocated memory */  
  if(gfilename)
    g_free(gfilename);

  if(gtorrentmetainfo)
    benc_node_destroy(gtorrentmetainfo);

  /* exit ok */
  exit(EXIT_SUCCESS);
}

/**
 * @brief display usage 
 */
static void
display_usage(void)
{
  g_print(_("Usage: gtv [options] [torrentfile]\n"));
  g_print("\n-h, --help             ");
  g_print(_("Display this text and exit."));
  g_print("\n-v, --version          ");
  g_print(_("Print version number and exit.\n"));

  exit(EXIT_SUCCESS);
}

/**
 * @brief parse command line options
 * 
 * BE AWARE: It use the filename global variable.
 *
 * @param argc: number of arguments
 * @param argv: pointer to the arguments
 */
static void
parse_cmd_line(gint argc, gchar **argv)
{
  gint c;
  static struct option long_options[] = {{"help", 0, NULL, 'h'},
                                         {"version", 0, NULL, 'v'},
                                         {0, 0, 0, 0}};

  while ((c = getopt_long(argc, argv, "hv", long_options, NULL)) != -1)
  {
    switch (c)
    {
    case 'h':
      display_usage();
      break;
    case 'v':
      g_printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      exit(EXIT_SUCCESS);
      break;
    }
  }
  
  if(optind < argc)
    gfilename = g_strdup(argv[optind]);
  
  return;
}

/**
 * @brief Get Torrent MetaInfo from a file. 
 *
 * @param name: the name of the file.
 * @return nothing, this is not a joinble thread.
 */
gpointer
open_torrent_file(gpointer name)
{
  FILE *fp;
  BencNode *root;
  MainWindow *mwin = MAINWINDOW(gmainwin);

  gdk_threads_enter();;
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->OpenToolButton), FALSE);
  log_ok(_("Opening %s."), (gchar*)name);
  gdk_threads_leave();

  if((fp = fopen((gchar*)name, "r")) == NULL)
  { 
    gdk_threads_enter();; 
    gtk_widget_set_sensitive(GTK_WIDGET(mwin->OpenToolButton), TRUE);
    log_error("%s", g_strerror(errno));
    gdk_threads_leave();
    g_free(name);
    return NULL;
  }

  root = benc_decode_file(fp);
  fclose(fp);

  if(root == NULL)
  {
    gdk_threads_enter();;
    gtk_widget_set_sensitive(GTK_WIDGET(mwin->OpenToolButton), TRUE);
    log_error(_("Open error: %s is not a bencoded torrent file or have corrupted data."),
              (gchar*)name);
    gdk_threads_leave();
    g_free(name);
    return NULL;
  }

  /* cancel any other thread */
  G_LOCK(thread_mutex);

  if(scrape_thread != NULL)
    scrape_cancel = TRUE;

  if(checkfiles_thread != NULL)
    checkfiles_cancel = TRUE;

  G_UNLOCK(thread_mutex);

  /* save filename pointer in a global variable, IMPORTANT: don't free it outside of here. */
  if(gfilename != NULL)
    g_free(gfilename);

  gfilename = name;

  /* save matainfo pointer in a global variable, IMPORTANT: don't free it outside of here. */
  if(gtorrentmetainfo != NULL)
    benc_node_destroy(gtorrentmetainfo);
  
  gtorrentmetainfo = root;

  /* ok, fill the GUI */
  gdk_threads_enter();;
  mainwindow_fill_general_tab(mwin, gtorrentmetainfo);
  gdk_threads_leave();

  gdk_threads_enter();;
  mainwindow_fill_files_tab(mwin, gtorrentmetainfo);
  gdk_threads_leave();

  gdk_threads_enter();;
  mainwindow_fill_trackers_tab(mwin, gtorrentmetainfo);
  gdk_threads_leave();

  gdk_threads_enter();;
  mainwindow_fill_torrent_tab(mwin, gtorrentmetainfo);
  gdk_threads_leave();

  gdk_threads_enter();;
  log_ok("%s",_("Open success."));
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->OpenToolButton), TRUE);
  gdk_threads_leave();

  return NULL;
}

/**
 * @brief Scrape the Tracker
 *
 * @param tracker: the tracker sitrng, if it terminate with "info_hash=", 
 *        then it is completed with the SHA1 of the opened torrent. It most 
 *        be dinamic allocated 'cos it will be free() here.
 * @return nothing, it is a no joinble thread.
 */
gpointer
tracker_scrape(gpointer tracker)
{
  MainWindow *mwin;
  gchar *string, *host, msn[CURL_ERROR_SIZE], torrent_sha[SHA_DIGEST_LENGTH];
  gchar *seeds_button_label, *tracker_button_label;
  FILE *fp;
  guint number, timeout;
  BencNode *root, *node, *child;
  CURL *curl;
  CURLcode success;

  G_LOCK(thread_mutex);
  if(scrape_thread == NULL)
    scrape_thread = g_thread_self();
  else
  {
    gdk_threads_enter();;
    log_warning("%s", _("Previous connection not finish yet. Try again later."));
    gdk_threads_leave();
    g_free(tracker);
    G_UNLOCK(thread_mutex);
    return NULL;
  }
  G_UNLOCK(thread_mutex);
  
  mwin = MAINWINDOW(gmainwin);
  timeout = DEF_WAIT_AFTER_SCRAPE;
  
  gdk_threads_enter();;
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshSeedsButton), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshTrackerButton), FALSE);
  gdk_threads_leave();
  
  node = benc_node_find_key(gtorrentmetainfo, "info");
  if(node != NULL)
  {
    string = benc_encode_buf(node, &number);
    SHA1((guint8*)string, number, (guint8*)torrent_sha);
    g_free(string);

    string = util_convert_to_hex(torrent_sha, SHA_DIGEST_LENGTH, "%");
    host = g_strdup_printf("%s?info_hash=%s", (gchar*)tracker, string);
    g_free(string);

    string = g_strrstr(host, "announce");
    if(string != NULL)
    {
      g_strlcpy(string, "scrape", 8);
      g_strlcpy(string+6, string+8, strlen(string+7));

      if((fp = tmpfile()) != NULL)
      {  
        if((curl = curl_easy_init()) != NULL)
        { 
          gdk_threads_enter();;
          log_ok(_("Connecting to %s"), host);
          gdk_threads_leave();

          curl_easy_setopt(curl, CURLOPT_URL, host);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
          curl_easy_setopt(curl, CURLOPT_VERBOSE, FALSE);
          curl_easy_setopt(curl, CURLOPT_NOPROGRESS, TRUE);
          curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, msn);

          if((success = curl_easy_perform(curl)) == 0)
          {
            G_LOCK(thread_mutex);
            if(!scrape_cancel)
            {
              rewind(fp);
              root = benc_decode_file(fp);

              if(root != NULL)
              {
                gdk_threads_enter();;
               
                if(!g_str_has_suffix((gchar*)tracker, "info_hash="))
                  mainwindow_fill_bencode_tree(mwin, mwin->TrackerTreeView, root);

                node = benc_node_find(root, BENC_TYPE_KEY, SHA_DIGEST_LENGTH, torrent_sha);

                if(node != NULL)
                {
                  child = benc_node_find_key(node, "complete");
                  gtk_entry_set_text(mwin->SeedEntry, child?benc_node_data(child):"?");

                  child = benc_node_find_key(node, "incomplete");
                  gtk_entry_set_text(mwin->PeersEntry, child?benc_node_data(child):"?");

                  child = benc_node_find_key(node, "downloaded");
                  gtk_entry_set_text(mwin->DownloadedEntry, child?benc_node_data(child):"?");
                }
                else
                {
                  gtk_entry_set_text(mwin->SeedEntry, "?");
                  gtk_entry_set_text(mwin->PeersEntry, "?");
                  gtk_entry_set_text(mwin->DownloadedEntry, "?");
                }
 
                benc_node_destroy(root);          

                log_ok("%s", _("Scrape success."));
                gdk_threads_leave();
              }
              else
              {
                gdk_threads_enter();;
                log_error("%s", _("Bad data from tracker"));
                gdk_threads_leave();
              }
            }

            G_UNLOCK(thread_mutex);
          }
          else
          {
            G_UNLOCK(thread_mutex);
            gdk_threads_enter();;
            log_error("%s", msn);
            gdk_threads_leave();
          }
          
          curl_easy_cleanup(curl);
        }
        else
        {
          gdk_threads_enter();;
          log_error("%s", _("Error in Curl library."));
          gdk_threads_leave();
        }
      }
      else
      {
        gdk_threads_enter();;
        log_error("%s", _("Couldn't create the temporary file for the tracker scrape."));
        gdk_threads_leave();
      }

      fclose(fp);
    }
    else 
    {
      gdk_threads_enter();;
      log_error("%s", _("This tracker don't support scrape."));
      gdk_threads_leave();
    }

    g_free(host);
  }
  else
  {
    gdk_threads_enter();;
    log_error("%s", _("Couldn't scrape. Bad Torrent data, Info section lost."));
    gdk_threads_leave();
  }
  
  g_free(tracker);

  /* interval timeout wait there */
  seeds_button_label = g_strdup(gtk_label_get_label(mwin->RefreshSeedsButtonLabel));
  tracker_button_label = g_strdup(gtk_label_get_label(mwin->RefreshTrackerButtonLabel));
  while(timeout > 0)
  {
    G_LOCK(thread_mutex);
    if(scrape_cancel)
    {
      G_UNLOCK(thread_mutex);
      break;
    }
    G_UNLOCK(thread_mutex);
    
    string = g_strdup_printf(_("Wait(%i)"), timeout);
    
    gdk_threads_enter();;
    gtk_label_set_label(mwin->RefreshSeedsButtonLabel, string); 
    gtk_label_set_label(mwin->RefreshTrackerButtonLabel, string); 
    gdk_threads_leave();
    g_free(string);
    
    timeout--;
    g_usleep(G_USEC_PER_SEC);
  }

  gdk_threads_enter();;
  gtk_label_set_label(mwin->RefreshSeedsButtonLabel, tracker_button_label); 
  gtk_label_set_label(mwin->RefreshTrackerButtonLabel, tracker_button_label); 
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshSeedsButton), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->RefreshTrackerButton), TRUE);
  gdk_threads_leave();

  g_free(seeds_button_label);
  g_free(tracker_button_label);

  G_LOCK(thread_mutex);
  scrape_thread = NULL;
  scrape_cancel = FALSE;
  G_UNLOCK(thread_mutex);

  return NULL;  
}

/**
 * @brief Check The files.
 *
 * @param name: the file or folder name, It most 
 *        be dinamic allocated 'cos it will be free() here.
 * @return nothing.
 */
gpointer
check_files(gpointer name)
{
  struct _fq
  {
    gchar *filename;
    GtkTreeIter iter;
    gint64 filesize, fileremain, firstpiecesize, lastpiecesize;
    guint  firstpiece, npieces;    
    GBitArray *pieces_array;
  } *files_queue;
  guint files_number, pieces_number;
  gint64 readed, piece_size, offset;
  guint i, j, k;
  gchar *string, *piece_buf, *torrent_sha_array, sha[SHA_DIGEST_LENGTH];
  GtkTreeModel *liststore;
  FILE *fp;
  BencNode *node;
  MainWindow *mwin = MAINWINDOW(gmainwin);

  G_LOCK(thread_mutex);
  if(checkfiles_thread == NULL)
    checkfiles_thread = g_thread_self();
  else
  {
    gdk_threads_enter();;
    log_warning("%s", _("Already checking files. Try again later."));
    gdk_threads_leave();
    G_UNLOCK(thread_mutex);
    if(name)
      g_free(name);    
  }
  G_UNLOCK(thread_mutex);

  gdk_threads_enter();;
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->CheckFilesButton), FALSE);
  gdk_threads_leave();

  G_LOCK(thread_mutex);
  node = benc_node_find_key(gtorrentmetainfo, "pieces");
  if(node != NULL)
  {
    pieces_number = benc_node_length(node)/SHA_DIGEST_LENGTH;
    torrent_sha_array = (gchar*)benc_node_data(node);
  }
  else
  {
    pieces_number = 0;
    torrent_sha_array = NULL;
  }
  node = benc_node_find_key(gtorrentmetainfo, "piece length");
  if(node != NULL)
    piece_size = strtol(benc_node_data(node), (char**)NULL, 10);
  else
    piece_size = 0;

  G_UNLOCK(thread_mutex);
  liststore = gtk_tree_view_get_model(mwin->FilesTreeView);
  files_number = gtk_tree_model_iter_n_children(liststore, NULL);
  if(files_number > 0 && pieces_number > 0 && piece_size > 0)
  {
    gdk_threads_enter();;
    log_ok("%s", _("Files check started."));
    gdk_threads_leave();

    G_LOCK(thread_mutex);
    piece_buf = g_malloc0(piece_size);
    /* load the files queue */
    files_queue = g_new0(struct _fq, files_number);
    for(i = 0, offset = 0; i < files_number; i++) 
    {
      if(gtk_tree_model_iter_nth_child(liststore, &files_queue[i].iter, NULL, i))
      {
        gtk_tree_model_get(liststore, &files_queue[i].iter, 
                           COL_FILE_NAME, &string,
                           COL_FILE_SIZE, &files_queue[i].filesize, 
                           COL_FILE_FIRST_PIECE, &files_queue[i].firstpiece,
                           COL_FILE_N_PIECES, &files_queue[i].npieces, 
                           COL_FILE_PIECESBITARRAY, &files_queue[i].pieces_array,
                           -1);
        g_bitarray_clear(files_queue[i].pieces_array);
        files_queue[i].fileremain = files_queue[i].filesize;
        files_queue[i].lastpiecesize = 0;
        files_queue[i].firstpiecesize = MIN(piece_size-offset, files_queue[i].filesize);
        if(offset >= piece_size)
          offset = files_queue[i].filesize - piece_size*(files_queue[i].npieces-1);
        else
          offset += files_queue[i].filesize - piece_size*(files_queue[i].npieces-1);
        
        if(files_number > 1)
          files_queue[i].filename = g_strdup_printf("%s/%s", (gchar*)name, string);
        else
          files_queue[i].filename = g_strdup(name);

        g_free(string);
      }
      else
      {
        gdk_threads_enter();;
        log_warning("%s", _("Unexpected files list reading error."));
        gdk_threads_leave();
      }
    }
    G_UNLOCK(thread_mutex);

    /* check the files */   
    for(i = 0, j = 0; i < files_number; i++, j = 0) 
    {     
      G_LOCK(thread_mutex);            
      if(checkfiles_cancel)
      {
        G_UNLOCK(thread_mutex);
        break;
      }

      gdk_threads_enter();;
      gtk_list_store_set(GTK_LIST_STORE(liststore), &files_queue[i].iter, 
                         COL_FILE_REMAINS, files_queue[i].fileremain, -1);    
      gdk_threads_leave();
      G_UNLOCK(thread_mutex);

      if((fp = fopen(files_queue[i].filename, "r")) != NULL)
      {
        if(i > 0 && (files_queue[i].firstpiece == files_queue[i-1].firstpiece + files_queue[i-1].npieces - 1))
        {
          if(g_bitarray_get_bit(files_queue[i-1].pieces_array, files_queue[i-1].firstpiece+files_queue[i-1].npieces-1) == TRUE)
          {
            g_bitarray_set_bit(files_queue[i].pieces_array, files_queue[i].firstpiece, 1);
            files_queue[i].fileremain -= files_queue[i].firstpiecesize;
          }
          _fseeko(fp, files_queue[i].firstpiecesize, SEEK_SET);
          j++;
        }
 
        for( ; j < files_queue[i].npieces && !feof(fp); j++)
        {
          G_LOCK(thread_mutex);      
          if(checkfiles_cancel)
          {
            G_UNLOCK(thread_mutex);
            break;
          }
          
          files_queue[i].lastpiecesize = (gint64)fread(piece_buf, sizeof(gchar), (size_t)piece_size, fp);
          readed = files_queue[i].lastpiecesize;
        
          if(readed < piece_size)
          {
            if(j == files_queue[i].npieces-1)
            {
              for(k = i+1; readed < piece_size && k < files_number; k++)
              {
                fclose(fp);
                if((fp = fopen(files_queue[k].filename, "r")) == NULL)
                  break;
          
                files_queue[k].lastpiecesize = (gint64)fread(piece_buf+readed, sizeof(gchar), (size_t)piece_size-readed, fp);
                readed += files_queue[k].lastpiecesize;
              }
            }
            else
            {
              gdk_threads_enter();;
              log_warning(_("%s is smaller than it should be"), files_queue[i].filename);
              gdk_threads_leave();
            }
          }
          
          SHA1((guint8*)piece_buf, (guint32)readed, (guint8*)sha);
          if(memcmp(sha, torrent_sha_array+((j+files_queue[i].firstpiece)*SHA_DIGEST_LENGTH), SHA_DIGEST_LENGTH) == 0)
          {
            g_bitarray_set_bit(files_queue[i].pieces_array, files_queue[i].firstpiece+j, 1);
            files_queue[i].fileremain -= files_queue[i].lastpiecesize;
            gdk_threads_enter();;
            gtk_list_store_set(GTK_LIST_STORE(liststore), &files_queue[i].iter, 
                               COL_FILE_REMAINS, files_queue[i].fileremain, -1);    
            gdk_threads_leave();
          }
          G_UNLOCK(thread_mutex);
        }

        if(fp != NULL)
          fclose(fp);

        G_LOCK(thread_mutex); 
        if(!checkfiles_cancel)
        {
          gdk_threads_enter();;
          gtk_list_store_set(GTK_LIST_STORE(liststore), &files_queue[i].iter, 
                             COL_FILE_ICON, mwin->file_state_icons[files_queue[i].fileremain>0?FILE_STATE_BAD:FILE_STATE_OK],
                             COL_FILE_REMAINS, files_queue[i].fileremain, -1);    
          gdk_threads_leave();
        }
        G_UNLOCK(thread_mutex); 
      }
      else
      {
        G_LOCK(thread_mutex); 
        if(!checkfiles_cancel)
        {
          gdk_threads_enter();;
          log_warning("%s: %s", files_queue[i].filename, g_strerror(errno));
          gtk_list_store_set(GTK_LIST_STORE(liststore), &files_queue[i].iter, 
                             COL_FILE_ICON, mwin->file_state_icons[FILE_STATE_BAD],
                             COL_FILE_REMAINS, files_queue[i].filesize, -1);    
          gdk_threads_leave();
        }
        G_UNLOCK(thread_mutex); 
      }
      
    }
    /* free the files queue */
    for(i = 0; i < files_number; i++) 
    {
      g_object_unref(G_OBJECT(files_queue[i].pieces_array));
      g_free(files_queue[i].filename);
    }
    g_free(files_queue);
    g_free(piece_buf);
    
    G_LOCK(thread_mutex); 
    gdk_threads_enter();;
    if(checkfiles_cancel)
      log_warning("%s", _("Files check canceled."));
    else
      log_ok("%s", _("Files check complete."));
    gdk_threads_leave();
    G_UNLOCK(thread_mutex); 
  }
  else
  {
    gdk_threads_enter();;
    log_error("%s", _("The files list seems to be empty"));
    gdk_threads_leave();
  }    

  if(name)
    g_free(name);    
 
  gdk_threads_enter();;
  gtk_widget_set_sensitive(GTK_WIDGET(mwin->CheckFilesButton), TRUE);
  gdk_threads_leave();
  G_LOCK(thread_mutex);
  checkfiles_thread = NULL;
  checkfiles_cancel = FALSE;
  G_UNLOCK(thread_mutex);
  return NULL;
}
