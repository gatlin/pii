/***
 * chat.c
 * Sets up libpurple and its event loop.
 */

#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include "win32/win32dep.h"
#endif

#include "purple.h"
#include <glib.h>

#include "config.h"
#include "chat.h"
#include "defines.h"
#include "ui.h"

/**
 * TODO
 * Maintain a collection of accounts, and create a directory in workspace for
 * each.
 * Commands doing stuff with accounts can simply map a string identifier to
 * their index in the collection and require explicit mention in the command.
 */
PurpleAccount *account;
static int client_infd = -1;

typedef struct {
  int infd;
  PurpleConversation *pconv;
} Conv;

static Conv *
pii_conv_new (int infd, PurpleConversation *pconv) {
  Conv *conv = malloc (sizeof (Conv));
  conv->infd = infd;
  conv->pconv = pconv;
  return conv;
}

static void
pii_conv_destroy (Conv *c) {
  free (c);
  c = NULL;
}

static void
pii_client_write_out (const char *msg) {
  gchar *outf = g_build_path ("/", cfg.workspace, "out", NULL);
  write_to_file (outf, msg);
  g_free (outf);
}

/*** GLib event loop hooks. ***/
#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

typedef struct _PurpleGLibIOClosure {
  PurpleInputFunction function;
  guint result;
  gpointer data;
} PurpleGLibIOClosure;

static void
purple_glib_io_destroy(gpointer data) {
  g_free (data);
}

static gboolean
purple_glib_io_invoke (GIOChannel *source,
                       GIOCondition condition,
                       gpointer data) {
  PurpleGLibIOClosure *closure = data;
  PurpleInputCondition purple_cond = 0;

  if (condition & PURPLE_GLIB_READ_COND) {
    purple_cond |= PURPLE_INPUT_READ;
  }
  if (condition & PURPLE_GLIB_WRITE_COND) {
    purple_cond |= PURPLE_INPUT_WRITE;
  }

  closure->function
    (closure->data, g_io_channel_unix_get_fd(source), purple_cond);

  return TRUE;
}

static guint
glib_input_add (gint fd, PurpleInputCondition condition,
                PurpleInputFunction function, gpointer data) {
  PurpleGLibIOClosure *closure = g_new0 (PurpleGLibIOClosure, 1);
  GIOChannel *channel;
  GIOCondition cond = 0;

  closure->function = function;
  closure->data = data;

  if (condition & PURPLE_INPUT_READ) {
    cond |= PURPLE_GLIB_READ_COND;
  }
  if (condition & PURPLE_INPUT_WRITE) {
    cond |= PURPLE_GLIB_WRITE_COND;
  }

#if defined _WIN32 && !defined WINPIDGIN_USE_GLIB_IO_CHANNEL
  channel = wpurple_g_io_channel_win32_new_socket (fd);
#else
  channel = g_io_channel_unix_new (fd);
#endif
  closure->result = g_io_add_watch_full
    (channel, G_PRIORITY_DEFAULT, cond,
     purple_glib_io_invoke, closure,
     purple_glib_io_destroy);

  g_io_channel_unref (channel);
  return closure->result;
}

static PurpleEventLoopUiOps glib_eventloops =
  { g_timeout_add,
    g_source_remove,
    glib_input_add,
    g_source_remove,
    NULL,
#if GLIB_CHECK_VERSION(2,14,0)
    g_timeout_add_seconds,
#else
    NULL,
#endif

    /* padding */
    NULL,
    NULL,
    NULL };

/*** End of the eventloop functions. ***/
/*** Begin UiOps. ***/

/* Callback which receives messages and logs them. */
static void
pii_write_conv (
  PurpleConversation *conv,
  const char *who,
  const char *alias,
  const char *message,
  PurpleMessageFlags flags,
  time_t mtime
) {
  const char *name;
  char *line, *outf;
  if (alias && *alias) {
    name = alias;
  }
  else if (who && *who) {
    name = who;
  }
  else {
    name = NULL;
  }
  outf = g_build_path ("/", cfg.workspace, conv->name, "out", NULL);
  line = g_strdup_printf ("%s %s", name, message);
  write_to_file (outf, line);
  g_free (outf);
  g_free (line);
}

/* Callback which creates the directory and out-log for a conversation. */
static void
pii_create_conversation (PurpleConversation *pconv) {
  gchar *convpath = g_build_path ( "/", cfg.workspace, pconv->name, NULL );
  gchar *outf = g_build_path ("/", cfg.workspace, "out", NULL);
  gchar *convout = g_build_path ("/", convpath, "out", NULL);
  create_dirtree (convpath);
  gchar *msg = g_strdup_printf ("creating conversation: %s", pconv->name);
  write_to_file (convout , "\n");
  write_to_file (outf, msg);
  g_free (msg);
  g_free (convout);
  g_free (convpath);
  g_free (outf);
}

static PurpleConversationUiOps pii_conv_uiops =
  { pii_create_conversation,   /* create_conversation  */
    NULL,                      /* destroy_conversation */
    NULL,                      /* write_chat           */
    NULL,                      /* write_im             */
    pii_write_conv,            /* write_conv           */
    NULL,                      /* chat_add_users       */
    NULL,                      /* chat_rename_user     */
    NULL,                      /* chat_remove_users    */
    NULL,                      /* chat_update_user     */
    NULL,                      /* present              */
    NULL,                      /* has_focus            */
    NULL,                      /* custom_smiley_add    */
    NULL,                      /* custom_smiley_write  */
    NULL,                      /* custom_smiley_close  */
    NULL,                      /* send_confirm         */
    NULL,
    NULL,
    NULL,
    NULL };

static void
process_client_input (const char *input) {
  int len = 0, start = 0;
  while (' ' == input[start]) { start++; }
  if (0 == memcmp (input+start, "msg", 3)) {
    start += 3;
    /* skip whitespace */
    while (' ' == input[start]) { start++; }
    /* capture non-whitespace */
    while (0 != input[start+len] && ' ' != input[start+len]) { len++; }
    /* copy out the name of the buddy and attempt to start a conversation */
    char *name = g_strndup (input+start, len);
    (void) purple_conversation_new (PURPLE_CONV_TYPE_IM, account, name);
    g_free (name);
  }
  else if (0 == memcmp (input+start, "ls", 2)) {
    GSList *buddies = purple_find_buddies (account, NULL);
    PurpleBuddy *buddy = NULL;
    pii_client_write_out ("Begin buddies");
    while (NULL != buddies) {
      buddy = buddies->data;
      if (NULL != buddy) {
        pii_client_write_out (buddy->name);
      }
      buddies = buddies->next;
    }
    pii_client_write_out ("End buddies");
    g_slist_free (buddies);
  }
  else if (0 == memcmp (input+start, "add", 3)) {
    start += 3;
    /* skip whitespace */
    while (' ' == input[start]) { start++; }
    /* capture non-whitespace */
    while (0 != input[start+len] && ' ' != input[start+len]) { len++; }
    if (0 == len) { return; }
    char *name = g_strndup (input+start, len);
    PurpleBuddy *buddy = purple_buddy_new (account, name, NULL);
    purple_blist_add_buddy (buddy, NULL, NULL, NULL);
    char *reply = g_strdup_printf ("added buddy %s", name);
    pii_client_write_out (reply);
    g_free (reply);
    g_free (name);
  }
  else if (0 == memcmp (input+start, "rm", 2)) {
    start += 2;
    /* skip whitespace */
    while (' ' == input[start]) { start++; }
    /* capture non-whitespace */
    while (0 != input[start+len] && ' ' != input[start+len]) { len++; }
    if (0 == len) { return; }
    char *name = g_strndup (input+start, len);
    PurpleBuddy *buddy = purple_find_buddy (account, name);
    if (NULL == buddy) {
      return;
    }
    purple_blist_remove_buddy (buddy);
    char *reply = g_strdup_printf ("removed buddy %s", name);
    pii_client_write_out (reply);
    g_free (reply);
    g_free (name);
  }
  else {
    g_message ("Unrecognized input: %s\n", input);
  }
}

static gboolean
client_input_cb (GIOChannel *ch, GIOCondition cond, gpointer data) {
  gsize length, term_pos;
  char *buffer;
  GError *error = NULL;
  GIOStatus status = g_io_channel_read_line
    ( ch,
      &buffer,
      &length,
      &term_pos,
      &error );
  if (NULL != error) {
    g_warning ("Error: %s\n", error->message);
    g_error_free (error);
    g_io_channel_shutdown (ch, TRUE, NULL);
    return FALSE;
  }
  switch (status) {
    case G_IO_STATUS_NORMAL:
      {
        buffer[strcspn (buffer, "\n")] = 0;
        process_client_input (buffer);
        free (buffer);
        break;
      }
    default:
      break;
  }
  return TRUE;
}

void
client_destroy_cb (gpointer data) {
  if (client_infd > -1) {
    destroy_input_pipe (client_infd);
    client_infd = -1;
  }
}

static void
pii_ui_init (void) {
  gchar *outf = g_build_path ("/", cfg.workspace, "out", NULL);
  gchar *inpipe = g_build_path ("/", cfg.workspace, "in", NULL);
  purple_conversations_set_ui_ops (&pii_conv_uiops);
  create_dirtree (cfg.workspace);
  pii_client_write_out ("\n"); // an empty string causes a hang at cleanup
  client_infd = create_input_pipe (inpipe);
  g_io_add_watch_full (
    g_io_channel_unix_new (client_infd),
    G_PRIORITY_DEFAULT,
    G_IO_IN | G_IO_HUP,
    client_input_cb,
    NULL,
    client_destroy_cb
  );
  g_free (outf);
  g_free (inpipe);
}

static PurpleCoreUiOps null_core_uiops =
  { NULL,
    NULL,
    pii_ui_init,
    NULL,

    /* padding */
    NULL,
    NULL,
    NULL,
    NULL };

static void
setup_libpurple () {
  /* Set a custom user directory (optional) */
  purple_util_set_user_dir (cfg.purple_data);

  /* We do not want any debugging for now to keep the noise to a minimum. */
  purple_debug_set_enabled (FALSE);

  /* Set the core-uiops, which is used to
   *  - initialize the ui specific preferences.
   *  - initialize the debug ui.
   *  - initialize the ui components for all the modules.
   *  - uninitialize the ui components for all the modules when the core terminates.
   */
  purple_core_set_ui_ops (&null_core_uiops);

  /* Set the uiops for the eventloop. If your client is glib-based, you can safely
   * copy this verbatim. */
  purple_eventloop_set_ui_ops (&glib_eventloops);

  /* Set path to search for plugins. The core (libpurple) takes care of loading the
   * core-plugins, which includes the protocol-plugins. So it is not essential to add
   * any path here, but it might be desired, especially for ui-specific plugins. */
  /* purple_plugins_add_search_path (CUSTOM_PLUGIN_PATH); */

  /* Now that all the essential stuff has been set, let's try to init the core. It's
   * necessary to provide a non-NULL name for the current ui to the core. This name
   * is used by stuff that depends on this ui, for example the ui-specific plugins. */
  if (!purple_core_init (UI_ID)) {
    /* Initializing the core failed. Terminate. */
    g_error ("libpurple initialization failed. Dumping core.\n"
             "Please report this!\n");
    /* Goodbye! */
  }

  /* Create and load the buddylist. */
  purple_set_blist (purple_blist_new ());
  purple_blist_load ();

  /* Load the preferences. */
  purple_prefs_init ();
  purple_prefs_load ();

  /* Load the desired plugins. The client should save the list of loaded plugins in
   * the preferences using purple_plugins_save_loaded(PLUGIN_SAVE_PREF) */
  purple_plugins_load_saved (PLUGIN_SAVE_PREF);

  /* Load the pounces. */
  purple_pounces_load ();
}

/*** End UiOps. ***/
/*** Begin signals. ***/

/* Subscribed to the "signed-on" libpurple signal. */
static void
signed_on(PurpleConnection *gc, gpointer null) {
  PurpleAccount *account = purple_connection_get_account (gc);
  g_debug ("Account connected: %s %s",
             account->username, account->protocol_id);
}

static gboolean
conv_input_cb (GIOChannel *ch, GIOCondition cond, gpointer data) {
  gsize length, term_pos;
  char *buffer;
  Conv *conv = data;
  PurpleConvIm *im = purple_conversation_get_im_data (conv->pconv);
  GError *error = NULL;
  GIOStatus status = g_io_channel_read_line
    ( ch,
      &buffer,
      &length,
      &term_pos,
      &error );
  if (NULL != error) {
    g_warning ("Error: %s\n", error->message);
    g_error_free (error);
    g_io_channel_shutdown (ch, TRUE, NULL);
    return FALSE;
  }
  switch (status) {
    case G_IO_STATUS_NORMAL:
      {
        buffer[strcspn (buffer, "\n")] = 0;
        purple_conv_im_send (im, buffer);
        free (buffer);
        break;
      }
    default:
      break;
  }
  return TRUE;
}

static void
conv_destroy_cb (gpointer data) {
  Conv *c = data;
  destroy_input_pipe (c->infd);
  pii_conv_destroy (c);
}


/* Subscribed to the "conversation-created" libpurple signal. */
static void
conversation_created (PurpleConversation *pconv, gpointer data) {
  int infd;
  gchar *convpath = g_build_path ( "/", cfg.workspace, pconv->name, NULL );
  gchar *inpipe = g_build_path ("/", convpath, "in", NULL);
  infd = create_input_pipe (inpipe);
  g_io_add_watch_full (
    g_io_channel_unix_new (infd),
    G_PRIORITY_DEFAULT,
    G_IO_IN | G_IO_HUP,
    conv_input_cb,
    pii_conv_new (infd, pconv),
    conv_destroy_cb );
  g_free (convpath);
  g_free (inpipe);
}

static void
connect_to_signals () {
  static int signed_on_handle, conversation_created_handle;

  purple_signal_connect (purple_connections_get_handle (),
                         "signed-on", &signed_on_handle,
                         PURPLE_CALLBACK(signed_on), NULL);

  purple_signal_connect (purple_conversations_get_handle (),
                         "conversation-created", &conversation_created_handle,
                         PURPLE_CALLBACK(conversation_created), NULL);
}

/*** End signals. ***/

/* Authenticate with the server and begin listening for messages. */
gboolean
initialize_chat () {
  PurpleSavedStatus *status;

  setup_libpurple ();

  if (NULL != cfg.protocol) {
    /* Authenticate with the server */
    account = purple_account_new (cfg.username, cfg.protocol);
    if (NULL != cfg.password) {
      purple_account_set_password (account, cfg.password);
    }
    purple_account_set_enabled (account, UI_ID, TRUE);
  }

  else if (cfg.bonjour_enabled) {
    /* Also set up a bonjour user because why not */
    g_message ("Using bonjour\n");
    account = purple_account_new ("pii", "prpl-bonjour");
    purple_account_set_alias (account, cfg.username);
    purple_account_set_enabled (account, UI_ID, TRUE);
  }

  else {
    g_error ("No valid account, halting setup.\n");
    return FALSE;
  }

  /* Set our status */
  status = purple_savedstatus_new (NULL, PURPLE_STATUS_AVAILABLE);
  purple_savedstatus_activate (status);

  connect_to_signals ();
  return TRUE;
}
