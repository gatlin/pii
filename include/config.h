#ifndef __PII_CONFIG_H
#define __PII_CONFIG_H

#include <glib.h>

typedef struct {
  char *protocol;
  char *username;
  char *password;
  char *purple_data; /* Path to store purple account data */
  char *workspace; /* Path where session files are located */
  char *clientout; /* Path to client outfile (trading memory for cycles */
  gboolean bonjour_enabled;
} PiiConfig;

extern PiiConfig cfg;

gboolean load_config (char *, GError **);

#endif /* __PII_CONFIG_H */
