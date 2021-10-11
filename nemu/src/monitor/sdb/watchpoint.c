#include <sdb/watchpoint.h>
#include "sdb.h"
#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  word_t last_val;
  char expr_str[64];
} WP;

WP* new_wp();
void free_wp(WP *wp);

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
  WP* wp_ptr = free_;
  if(wp_ptr == NULL){
    assert(0);  //! 没有空闲WP
  }
  free_ = wp_ptr->next;
  wp_ptr->next = head;
  head = wp_ptr;
  return head;
}
void free_wp(WP *wp){
  WP* wp_ptr_pre = NULL;
  WP* wp_ptr = head;
  while(wp_ptr != NULL && wp_ptr != wp){
    wp_ptr_pre = wp_ptr;
    wp_ptr = wp_ptr->next;
  }

  if(wp_ptr == NULL){
    assert(0);  //! 没在head链表中找到这个wp
  }
  if(wp_ptr_pre){
    wp_ptr_pre->next = wp_ptr->next;
  }
  else{
    head = head->next;
  }

  wp_ptr->next = free_;
  free_ = wp_ptr;
  return;
}

//! 提供给sdb的API
void set_wp(char *wp_expr){
  WP* wp_ptr = new_wp();
  strcpy(wp_ptr->expr_str, wp_expr);
  bool if_success = true;
  wp_ptr->last_val = expr(wp_expr, &if_success);
  if(wp_expr == NULL || !if_success){
    printf("Error: Set watch point failed, unknown expr.\n");
    free_wp(wp_ptr);
  }
}
//! 提供给sdb的API
void del_wp(int wp_num){
  printf("Now delete the %d wp\n", wp_num);
  WP* wp_ptr = head;
  while(wp_ptr && wp_ptr->NO != wp_num){
    wp_ptr = wp_ptr->next;
  }
  if(wp_ptr == NULL){
    printf("Error: Del watch point failed, unknown number.\n");
  }
  free_wp(wp_ptr);
}
//! 提供给sdb的API
void info_wp(){
  WP* wp_ptr = head;
  printf("NUM\tVAL\tEXPR\n");
  while(wp_ptr){
    printf("%d\t0x%lx\t%s\n",wp_ptr->NO, wp_ptr->last_val, wp_ptr->expr_str);
    wp_ptr = wp_ptr->next;
  }
}
//! 提供给sdb的API
bool check_wp(){
  WP* wp_ptr = head;
  bool if_changed = false;
  while(wp_ptr){
    bool if_success = true;
    word_t now_val = expr(wp_ptr->expr_str, &if_success);
    if(now_val != wp_ptr->last_val){
      printf("Watch point %d %s has changed, old val: 0x%lx, new val: 0x%lx\n", wp_ptr->NO, wp_ptr->expr_str, wp_ptr->last_val, now_val);
      wp_ptr->last_val = now_val;
      if_changed = true;
    }
    wp_ptr = wp_ptr->next;
  }
  return if_changed;
}