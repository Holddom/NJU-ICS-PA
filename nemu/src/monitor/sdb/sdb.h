#ifndef __SDB_H__
#define __SDB_H__

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char condation[32];

} WP;


#include <common.h>

word_t expr(char *e, bool *success);

WP* new_wp(char *condation, bool *success);

void free_wp(int NO);

void watchpoint_display();

#endif
