#ifndef _list_h_
#define _list_h_

#include <stdlib.h>

struct ListNode;
typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;
    void *value;
} ListNode;

typedef struct List {
    int count;
    ListNode *first;
    ListNode *last;
} List;

List *List_create();
void List_destroy(List *list);
void List_clear(List *list);
void List_clear_destroy(List *list);

#define List_len(A) ((A)->count)
#define List_first(A) ((A)->first != NULL ? (A)->first->value : NULL)
#define List_last(A) ((A)->last != NULL ? (A)->last->value : NULL)

void List_push(List *list, void *value);
void *List_pop(List *list);
void List_push_node(List *list, ListNode *node);
ListNode* List_head(List *list);

void List_unshift(List *list, void *value);
void *List_shift(List *list);

void *List_remove(List *list, ListNode *node);

void List_swap(ListNode *a, ListNode *b);
// int List_len(List *list);

#define LIST_FOREACH(List, First, Next, Val) \
    for(ListNode *Val = List->First; Val != NULL; Val = Val->Next) \

#endif
