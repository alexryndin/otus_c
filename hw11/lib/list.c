#include "list.h"
#include "dbg.h"

List *List_create() { return calloc(1, sizeof(List)); }

void List_destroy(List *list) {
    CHECK(list != NULL, "Invalid list.");
    LIST_FOREACH(list, first, next, cur) {
        if (cur->prev) {
            free(cur->prev);
        }
    }

    free(list->last);
    free(list);
error:
    return;
}

void List_clear(List *list) {
    LIST_FOREACH(list, first, next, cur) { free(cur->value); }
}

//int List_len(List *list) {
//    CHECK(list != NULL, "Invalid list.");
//    return list->count;
//error:
//    return -1;
//}

void List_clear_destroy(List *list) {
    LIST_FOREACH(list, first, next, cur) {
        if (cur->prev) {
            free(cur->prev->value);
            free(cur->prev);
        }
    }
    if (list->last != NULL) {
        free(list->last->value);
        free(list->last);
    }
    free(list);
    // List_clear(list);
    // List_destroy(list);
}

void List_push(List *list, void *value) {
    CHECK(list != NULL, "Invalid list.");
    ListNode *node = calloc(1, sizeof(ListNode));
    CHECK_MEM(node);

    node->value = value;

    if (list->last == NULL) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        node->prev = list->last;
        list->last = node;
    }

    list->count++;

error:
    return;
}

void List_push_node(List *list, ListNode *node) {
    CHECK(list != NULL, "Invalid list.");

    if (list->last == NULL) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        node->prev = list->last;
        list->last = node;
    }

    list->count++;

error:
    return;
}

ListNode *List_head(List *list) {
    ListNode *ret = NULL;
    CHECK(list != NULL, "Invalid list.");
    if (List_len(list) == 0)
        return ret;
    ret = list->first;
    list->count--;
    if (List_len(list) == 0) {
        list->first = NULL;
        list->last = NULL;
        ret->next = NULL;
        return ret;
    }
    list->first = ret->next;
    list->first->prev = NULL;
    ret->next = NULL;
error:
    return ret;
}

void *List_pop(List *list) {
    CHECK(list != NULL, "Invalid list.");
    ListNode *node = list->last;
    return node != NULL ? List_remove(list, node) : NULL;

error:
    return NULL;
}

void List_unshift(List *list, void *value) {
    CHECK(list != NULL, "Invalid list.");
    ListNode *node = calloc(1, sizeof(ListNode));
    CHECK_MEM(node);

    node->value = value;

    if (list->first == NULL) {
        list->first = node;
        list->last = node;
    } else {
        node->next = list->first;
        list->first->prev = node;
        list->first = node;
    }

    list->count++;

error:
    return;
}

void *List_shift(List *list) {
    ListNode *node = list->first;
    return node != NULL ? List_remove(list, node) : NULL;
}

inline void List_swap(ListNode *a, ListNode *b) {
    CHECK(a != NULL && b != NULL, "Invalid node.");
    void *tmp = b->value;
    b->value = a->value;
    a->value = tmp;
error:
    return;
}

void *List_remove(List *list, ListNode *node) {
    void *result = NULL;

    CHECK(list != NULL, "Invalid list.");
    CHECK(list->first && list->last, "List is empty.");
    CHECK(node, "node can't be NULL");

    if (node == list->first && node == list->last) {
        list->first = NULL;
        list->last = NULL;
    } else if (node == list->first) {
        list->first = node->next;
        CHECK(list->first != NULL,
              "Invalid list, somehow got a first that is NULL.");
        list->first->prev = NULL;
    } else if (node == list->last) {
        list->last = node->prev;
        CHECK(list->last != NULL,
              "Invalid list, somehow got a next that is NULL.");
        list->last->next = NULL;
    } else {
        ListNode *after = node->next;
        ListNode *before = node->prev;
        after->prev = before;
        before->next = after;
    }

    list->count--;
    result = node->value;
    free(node);

error:
    return result;
}
