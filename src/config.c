/***
 * config.c
 * This code is responsible for acquiring any and all configuration data from
 * the environment.
 */

#include <gmodule.h>
#include "config.h"
#include "defines.h"

#include "purple.h"

PiiConfig cfg;

gboolean
load_config (char *config_path, GError **caller_error) {
  GError *error = NULL;
  GKeyFile *keyfile = g_key_file_new ();
  GKeyFileFlags flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

  //cfg = g_slice_new (PiiConfig);
  if (!g_key_file_load_from_file
      ( keyfile,
        config_path,
        flags,
        &error )) {
    g_propagate_error (caller_error, error);
    return FALSE;
  }

  cfg.protocol = g_key_file_get_string
    (keyfile, "credentials", "protocol", NULL);

  cfg.username = g_key_file_get_string
    (keyfile, "credentials", "username", NULL);

  cfg.password = g_key_file_get_string
    (keyfile, "credentials", "password", NULL);

  cfg.bonjour_enabled = g_key_file_get_boolean
    (keyfile, "pii", "bonjour", NULL);

  cfg.purple_data = g_key_file_get_string
    (keyfile, "pii", "libpurpledata", NULL);

  cfg.workspace = g_key_file_get_string
    (keyfile, "pii", "workspace", NULL);

  cfg.clientout = g_build_path ("/", cfg.workspace, "out", NULL);

  g_key_file_free (keyfile);
  return TRUE;
}
