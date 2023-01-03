#ifndef __PII_UI_H
#define __PII_UI_H

#include <stddef.h>

void create_dirtree (const char *);
void write_to_file (const char *, const char *);
int create_input_pipe (const char *);
void destroy_input_pipe (int);

#endif /* __PII_UI_ H*/
