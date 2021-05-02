/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */
#include "0searchfulDefs.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <pthread.h>

#include "interface.h"
#include "support.h"
#include "search.h"
#include "savestate.h" /* Add support for save/restore config.ini */

GtkWidget *mainWindowApp = NULL; /* Holds pointer to the main window (global) */
gchar *gConfigFile = NULL; /* created by main(), destroyed by destroyGKeyFile() */

/*
 * Searchmonkey entry point
 */
int main (int argc, char *argv[])
{
  GdkPixbuf* pixBuf;
  gchar *tmpStr;
  gint opt;
  gboolean fDparameter = FALSE;
  gboolean fTparameter = FALSE;
  gboolean fFparameter = FALSE;


#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  /* Handle GTK command line options and the locale settings */
  gtk_set_locale ();
  gtk_init (&argc, &argv);

 /* Initiate threads */
  if(!g_thread_supported()) /* Luc A janv 2018 ; useless if gtk >=2.32*/
     g_thread_init (NULL);/* deprecated since 2.32 */
  gdk_threads_init ();/* deprecated since gtk 3.6 */
  gdk_threads_enter ();/* id */

  /* Create main window, and load (create) ini config file */
  add_pixmap_directory (config_PackageDataDir "/pixmaps/" config_Package); /* New location for all pixmaps */
  add_pixmap_directory (config_PackageDataDir "/pixmaps"); /* Gnome users /usr/share/pixmaps folder */
  mainWindowApp = create_window1 ();
  gConfigFile = g_build_filename(g_get_home_dir(), "." config_Package, SEARCHMONKEY_CONFIG, NULL); /* Create hidden directory to store searchmonkey data */
  createGKeyFile (G_OBJECT (mainWindowApp), MASTER_OPTIONS_DATA);

  /* Create pointer to the argv command line */
  g_object_set_data(G_OBJECT(mainWindowApp), "argvPointer", argv[0]);

  /* Set up global strings */
  tmpStr = g_strdup_printf(_("\nNo content searching performed for this file.\n\nPlease ensure containing search was enabled, and that an expression was entered prior to starting search.\n"));
  g_object_set_data_full(G_OBJECT(mainWindowApp), "noContextSearchString", tmpStr, g_free);
  tmpStr = g_strdup_printf(_("N/A"));
  g_object_set_data_full(G_OBJECT(mainWindowApp), "notApplicable", tmpStr, g_free);

 /* some stuff to launch SearchMonkey with parameters - Luc A Janv 2018 */
 /* -? = help
    -d = directory
    -f = files
    -t containing text
 exemple : searchmonkey -d /home/tux -f ODT -t linux

*/
  // g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter1", NULL);
   while ((opt = getopt(argc, argv, "?d:f:t:")) != -1)
      {
               switch (opt) {
               case 'f':
                 if(!fFparameter) {
                   printf("-f file name parameter=%s\n", optarg);
                   if(optarg!=NULL)
                       g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter2", optarg);
                         else g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter2", NULL);
                   fFparameter = TRUE;}
                   break;
               case 't':
                 if(!fTparameter) {
                   printf("-t containing text parameter=%s\n", optarg);
                   if(optarg!=NULL)
                       g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter3", optarg);
                           else g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter3", NULL);
                   fTparameter = TRUE;}
                   break;
               case 'd':
                 if(!fDparameter) {
                   printf("-d directory parameter=%s\n", optarg);
                   if(optarg!=NULL)
                      g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter1", optarg);
                          else g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter1", NULL);
                   fDparameter = TRUE;}
                   break;
               default: /* '?' */
                   printf("\n----------------\nhow to launch Searchmonkey with parameters :\nsearchmonkey -d {directory} -f {file name} -t {containing text}\nExample :\n searchmonkey -d /home/tux/documents -f txt -t linux\n");
               }
      }/* wend */
 //if(argv[1]!=NULL)
   // {
     // printf("** started with %s: **\n", argv[1]);
     // g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter1", argv[1]);
  //  }
  //else g_object_set_data(G_OBJECT(mainWindowApp), "argvParameter1", NULL);
  /* Show app, and start main loop */
  gtk_widget_show (mainWindowApp);
  gtk_main ();

  /* Clean exit */
  gdk_threads_leave ();/* deprecated since gtk 3.6 */
  return 0;
}

