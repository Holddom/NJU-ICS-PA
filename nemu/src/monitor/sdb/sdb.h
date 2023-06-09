#ifndef __SDB_H__
#define __SDB_H__

typedef unsigned int uint32_t;

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char condition[32];
  uint32_t prev;
} WP;


#include <common.h>

word_t expr(char *e, bool *success);

WP* new_wp(char *condation, bool *success);

void free_wp(int NO);

void watchpoint_display();

bool check_watchpoint(WP **point);

#endif
