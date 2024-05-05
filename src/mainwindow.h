/**
 * @file mainwindow.h
 *
 * @brief Main Windows Object Header File
 *
 * Thu Oct  7 02:07:58 2004
 * Copyright  2004  Alejandro Claro
 * aleo@apollyon.no-ip.com
 */

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

G_BEGIN_DECLS

/* DEFINES ******************************************************************/

#define MAX_HEX_TO_SHOW_TREEVIEW  30 /*if invalid utf8 show just 30 hex nums*/
#define MAX_TREE_STRING_LEN      200 /*max lenght of a row in the a treeview*/

#define DEF_WAIT_AFTER_SCRAPE 30 /* wait 30 seconds after scrape a tracker */ 

#define DIRECTORY_DELIMITER  "/"

#define STRING_ICON_FILE     PIXMAPS_DIR "/string.png"
#define INTEGER_ICON_FILE    PIXMAPS_DIR "/integer.png"
#define LIST_ICON_FILE       PIXMAPS_DIR "/list.png"
#define DICTIONARY_ICON_FILE PIXMAPS_DIR "/dictionary.png"
#define UNKNOWN_ICON_FILE    PIXMAPS_DIR "/unknown.png"
#define INFO_ICON_FILE       PIXMAPS_DIR "/info.png"
#define OK_ICON_FILE         PIXMAPS_DIR "/ok.png"
#define ERROR_ICON_FILE      PIXMAPS_DIR "/error.png"
#define WARNING_ICON_FILE    PIXMAPS_DIR "/warning.png"
#define ABOUT_PIXMAP_FILE    PIXMAPS_DIR "/about.png"

#define MAINWINDOW_SYSTEM_ICON_FILE SYSTEM_PIXMAPS_DIR "/gtorrentviewer.png"
#define MAINWINDOW_ICON_FILE        PIXMAPS_DIR "/gtorrentviewer.png"

#define MAINWINDOW_TITLE     "Torrent Metainfo Viewer v" PACKAGE_VERSION

enum /* files list columns */
{
  COL_FILE_ICON = 0,
  COL_FILE_NAME,
  COL_FILE_SIZE,
  COL_FILE_FIRST_PIECE,
  COL_FILE_N_PIECES,
  COL_FILE_REMAINS,
  COL_FILE_PIECESBITARRAY,
  NUM_FILE_COLS
};

enum /* detailed trees columns */
{
  COL_ICON = 0,
  COL_TEXT,
  NUM_COLS
};

enum /* log events */
{
  LOG_OK = 0,
  LOG_WARNING,
  LOG_ERROR,
  NUM_LOG_EVENTS
};

enum /* files states */
{
  FILE_STATE_OK = 0,
  FILE_STATE_BAD,
  FILE_STATE_UNKNOWN,
  NUM_FILE_STATES
};

/* MACROS *******************************************************************/

#define MAINWINDOW_TYPE            (mainwindow_get_type())
#define MAINWINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MAINWINDOW_TYPE, MainWindow))
#define MAINWINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MAINWINDOW_TYPE, MainWindowClass))
#define IS_MAINWINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MAINWINDOW_TYPE))
#define IS_MAINWINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MAINWINDOW_TYPE))

/* TYPEDEF ******************************************************************/

typedef struct _MainWindow       MainWindow;
typedef struct _MainWindowClass  MainWindowClass;

/**
 * @brief MainWindow object structure  
 */
struct _MainWindow
{
  GtkWindow parent; /**< the window */
  
  GtkToolButton *NewToolButton;  /**< The New  button */
  GtkToolButton *OpenToolButton; /**< The Open button */
  GtkToolButton *SaveToolButton; /**< The Save button */
  GtkToolButton *QuitToolButton; /**< The Quit/Exit button */
  
  GtkButton *RefreshSeedsButton;       /**< The Refresh Seeds/Peers button */
  GtkLabel  *RefreshSeedsButtonLabel;  /**< Refresh Seeds/Peers button Label */
  GtkButton *CheckFilesButton;         /**< The Check Files button */
  GtkLabel  *CheckFilesButtonLabel;    /**< Check Files button Label */
  GtkButton *RefreshTrackerButton;     /**< The Refresh Tracker Info button */
  GtkLabel  *RefreshTrackerButtonLabel;/**< efresh Tracker Info button Label */  

  GtkComboBox *TrackerComboBox; /**< The Tracker list ComboBox */
  
  GtkTreeView *FilesTreeView;   /**< The Files List */
  GtkTreeView *TorrentTreeView; /**< The Torrent Details Tree */
  GtkTreeView *TrackerTreeView; /**< The Tracker Details Tree */
  GtkTreeView *LogTreeView;     /**< The Log List */
  
  GtkEntry *NameEntry;       /**< The Name textbox */
  GtkEntry *SHAEntry;        /**< The SHA textbox */
  GtkEntry *TrackerEntry;    /**< The Tracker textbox */
  GtkEntry *CreatedEntry;    /**< The Created By textbox */
  GtkEntry *DateEntry;       /**< The Date textbox */
  GtkEntry *SeedEntry;       /**< The Seeds textbox */
  GtkEntry *PeersEntry;      /**< The Peers textbox */
  GtkEntry *DownloadedEntry; /**< The Downloaded textbox */
  GtkEntry *PiecesEntry;     /**< The Pieces textbox */
  GtkEntry *PieceLenEntry;   /**< The Piece lenght textbox */
  GtkEntry *FilesEntry;      /**< The Number of Files textbox */
  GtkEntry *SizeEntry;       /**< The Total Size textbox */
  
  GtkTextView *CommentTextView; /**< The Comments textbox */
  
  GtkStatusbar *MainStatusBar;  /**< The Window's Status Bar */

  GdkPixbuf *benc_icons[BENC_TYPE_ALL]; /**< The Icons used in Details trees */
  GdkPixbuf *file_state_icons[NUM_FILE_STATES]; /**< The Icons used in Files list */
  GdkPixbuf *log_icons[NUM_LOG_EVENTS]; /**< The Icons used in Log list */
};

/**
 * @brief MainWindow Class structure 
 */
struct _MainWindowClass
{
  GtkWindowClass parent_class;
};

/* PROTOTYPES ***************************************************************/

GType mainwindow_get_type(void);
GtkWidget* mainwindow_new(void);

gint mainwindow_log_printf(MainWindow const *mwin, gshort event_type, gchar const *format, ...) G_GNUC_PRINTF(3, 4);

void mainwindow_fill_general_tab(MainWindow const *mwin, BencNode *torrent);
void mainwindow_fill_files_tab(MainWindow const *mwin, BencNode *torrent);
void mainwindow_fill_trackers_tab(MainWindow const *mwin, BencNode *torrent);
void mainwindow_fill_torrent_tab(MainWindow const *mwin, BencNode *torrent);

void mainwindow_fill_bencode_tree(MainWindow const *mwin, GtkTreeView *tree, BencNode *torrent);

G_END_DECLS

#endif /* _MAINWINDOW_H */
