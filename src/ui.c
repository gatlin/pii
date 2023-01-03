/***
 * ui.c
 * Tools for manipulating our UI of directories, files, and pipes.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include "ui.h"
#include <glib.h>

/* creates directories bottom-up, if necessary */
void
create_dirtree(const char *dir) {
  char tmp[PATH_MAX], *p;
  struct stat st;
  size_t len;

  g_strlcpy(tmp, dir, sizeof(tmp));
  len = strlen(tmp);
  if (len > 0 && tmp[len - 1] == '/')
    tmp[len - 1] = '\0';

  if ((stat(tmp, &st) != -1) && S_ISDIR(st.st_mode))
    return; /* dir exists */

  for (p = tmp + 1; *p; p++) {
    if (*p != '/')
      continue;
    *p = '\0';
    mkdir(tmp, S_IRWXU);
    *p = '/';
  }
  mkdir(tmp, S_IRWXU);
}

/* Construct an input FIFO pipe at the given path. */
int
create_input_pipe (const char *inpipe) {
  int fd;
  struct stat st;
  /* make "in" fifo if it doesn't exist already. */
  if (lstat(inpipe, &st) != -1) {
    if (!(st.st_mode & S_IFIFO))
      return -1;
  } else if (mkfifo(inpipe, S_IRWXU)) {
    return -1;
  }
  fd = open(inpipe, O_RDONLY | O_NONBLOCK, 0);
  return fd;
}

void
destroy_input_pipe (int fd) {
  close (fd);
}

void
write_to_file (const char *outfile, const char *buf) {
  FILE *fp = NULL;
  time_t t = time(NULL);

  if (!(fp = fopen(outfile, "a")))
    return;
  fprintf(fp, "%lu %s\n", (unsigned long)t, buf);
  fclose(fp);
}
