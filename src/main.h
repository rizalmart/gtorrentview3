/**
 * @file main.h
 *
 * @brief main header file
 *
 *  Thu Oct  7 01:03:49 2004
 *  Copyright  2004  Alejandro Claro
 *  aleo@apollyon.no-ip.com
 */

#ifndef _MAIN_H
#define _MAIN_H

/* DEFINES ******************************************************************/

#define LOG_WELCOME_MSN     PACKAGE_NAME " started."

/* GLOBALS ******************************************************************/

extern gboolean gissaved;

/* PROTOTYPES ***************************************************************/

G_BEGIN_DECLS

gpointer open_torrent_file(gpointer name);
gpointer tracker_scrape(gpointer tracker);
gpointer check_files(gpointer name);

G_END_DECLS

#endif /* _MAIN_H */
