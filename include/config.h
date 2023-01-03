#ifndef __PII_CONFIG_H
#define __PII_CONFIG_H

#include <glib.h>

/**
 * A Context is essentially global data for the program.
 *
 * In addition to containing execution-specific configuration data, Context
 * contains a key-value store that can be read from and written to by the user
 * and also queried by spawned processes.
 *
 * This allows sensitive information such as passwords to be stored in memory
 * and updated during program execution by the user.
 */
typedef struct {
  char *protocol;
  char *username;
  char *password;
  char *purple_data; /* Path to store purple account data */
  char *workspace; /* Path where session files are located */
  gboolean bonjour_enabled;
} PiiConfig;

extern PiiConfig cfg;

gboolean load_config (char *, GError **);

#endif /* __PII_CONFIG_H */
