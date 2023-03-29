#include "sdb.h"

#define NR_WP 32


static WP wp_pool[NR_WP] = {}; //监视点池 用于存储正在使用的监视点
static WP *head = NULL, *free_ = NULL; //free链表用于存放空闲的监视点

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

static int number =1;
/* TODO: Implement the functionality of watchpoint */

WP* new_wp(char * condition, bool *success){//从free链表中返回一个空闲的监视点结构
  if(free_->next==NULL) //把free_当作头结点
  {
    assert(0);
  }

    WP* ret= free_->next;
    ret->NO= number++;
    free_->next=ret->next;
    ret->next=NULL;
    expr(condition,success);
    strcpy(ret->condition,condition);

    if(head==NULL)  //head链表无头结点
    {
      head=ret;
    }
    else{
      ret->next=head->next;
      head->next=ret;
    }

    return ret;
}

static void insert_free(WP *wp)
{
  wp->next=free_->next;
  free_->next=wp;
}

void free_wp(int NO){//将wp返还给free链表
  if(head->NO==NO)
  {
    WP* node = head->next;
    insert_free(head);
    head=node;
    return ;
  }

  WP* node =head;
  while(node->next!=NULL)
  {
    if(node->next->NO==NO)
    {
      WP* wp=node->next->next;
      insert_free(node->next);
      node->next=wp;
      return ;
    }
    node=node->next;
  }

  printf("未找到 \e[1;36mWatchPoint(NO.%d)\e[0m\n", NO);

}

void watchpoint_display()
{
  printf("NO.\tCondation\n");
  WP* cur = head;
  while (cur){
    printf("\e[1;36m%d\e[0m\t\e[0;32m%s\e[0m\n", cur->NO, cur->condition);
    cur = cur->next;
  }
}

bool check_watchpoint(WP **point)
{
  WP* cur=head;
  bool success=true;
  while(cur)
  {
    if(expr(cur->condition,&success))
    {
      *point = cur;
      return true;
    }
    cur= cur->next;
  }
  return false;
}