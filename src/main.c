#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "purple.h"
#include "defines.h"
#include "config.h"
#include "chat.h"

/* Global values! */
char *config_path;
GMainLoop *loop;

static void
handle_term (int sinum) {
  if (g_main_loop_is_running (loop)) {
    g_message ("Caught SIGTERM ...");
    g_main_loop_quit (loop);
  }
}

/* Acceptable command line options */
GOptionEntry options[] = {
    { "config", 'c', 0,
      G_OPTION_ARG_STRING, &config_path,
      "Location of configuration file", NULL }
};

int
main (int argc, char *argv[]) {
  GError *error;
  GOptionContext *opt_context;
  struct sigaction action;
  GMainContext *gmc = NULL;

  loop = g_main_loop_new (gmc, FALSE);
  memset (&action, 0, sizeof (struct sigaction));
  action.sa_handler = handle_term;
  sigaction (SIGINT, &action, NULL);
  error = NULL;

#ifndef _WIN32
  /* libpurple's built-in DNS resolution forks processes to perform
   * blocking lookups without blocking the main process.  It does not
   * handle SIGCHLD itself, so if the UI does not you quickly get an army
   * of zombie subprocesses marching around.
   */
  signal (SIGCHLD, SIG_IGN);
#endif

  /* Parse options */
  opt_context = g_option_context_new ("- multi-protocol file-based chat");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &error)) {
    g_warning ("Error parsing arguments: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  if (NULL == config_path) {
    config_path = DEFAULT_CONFIG_PATH;
  }
  if (! load_config (config_path, &error)) {
    g_warning ("Failed to read configuration file. Exiting.\n");
    return 1;
  }
  if (! initialize_chat ()) {
    g_warning ("Error initializing chat\n");
    return 1;
  };
  g_main_loop_run (loop);
  purple_plugins_save_loaded (PLUGIN_SAVE_PREF);

  return 0;
}
