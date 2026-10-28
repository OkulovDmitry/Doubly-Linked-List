#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "list.h"

typedef double list_elem_t

typedef struct
{
    list_elem_t* data;
    int* next;
    int* prev;
    my_stack_t* free;
    int tail;
    int head;
} list_t;

int main()
{
    list_t first_list;
    int initial_capacity = 13;
    list_ctor(&first_list, );

    list_elem_t* data = {0, 10, 20, 30, 40, 50, 60, 70 80, 90, -1, -1, -1};

    int* next = {0, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1};
    int tail = 1;
    int head = 9;
    int* prev = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1};
    my_stack_t free;
    stack_ctor(&free, 6 STACK_PLACE_OUT);
    stack_push(&free, 10);
    stack_push(&free, 11);
    stack_push(&free, 12);


    elem_add(data, next, &tail, &head, 35, 3);

}

list_err_t list_dtor(list_t* list)
{
    free(data);
    free(next);
    free(prev);
    free(free);
    list->data = NULL;
    list->next = NULL;
    list->prev = NULL;
    list->free = NULL;
    tail = -1;
    head = -1;
    free(list);

    return LIST_OK;
}

list_err_t list_ctor(list_t* list, int initial_capacity)
{
    list_elem_t* temp_data = malloc(initial_capacity * sizeof(list_elem_t));
    if (temp_data == NULL) { free(list); return LIST_DATA_OUT_OF_MEMORY; }
    list->data = temp_data;

    int* temp_next    = malloc(initial_capacity * sizeof(int));
    if (temp_next == NULL) { list_dtor(list); return LIST_NEXT_OUT_OF_MEMORY; }
    list->next = temp_next;

    int* temp_prev    = malloc(initial_capacity) * sizeof(int);
    if (temp_prev == NULL) { list_dtor(list); return LIST_PREV_OUT_OF_MEMORY; }
    list->prev = temp_prev;

    err_t stack_err = stack_ctor(list->free, initial_capacity);
    if (stack_err != STACK_OK) { list_dtor(list); return LIST_FREE_FAIL; }

    for (int i = 0; i < initial_capacity; i++)
    {
        list->next[i]  = -1;
        list->prev[i]  = -1;
        list->data[i]  = -1;
    }

    for (int i = 1; i < initial_capacity; i++)
    {
        err_t push_err = stack_push(&free, initial_capacity - i);
        if (push_err != STACK_OK) {free(data); free(next); free(prev); free(list); return LIST_FREE_FAIL; }
    }

    list->tail = 0;
    list->head = 0;

    return LIST_OK;
}

list_err_t list_elem_push(list_t* list, list_elem_t elem, int after_that)
{
    int free_addr = -1;
    err_t pop_err = stack_pop(list->free, &free_addr);
    if (pop_err != STACK_OK) { return LIST_ADD_TO_FULL_LIST; }

    list->data[free_addr]  = elem;

    list->next[free_addr]  = list->next[after_that];    // теперь после free_addr лежит next для after_that
    list->prev[free_addr]  = after_that;                // теперь перед free_addr лежит after_that;
    
    if (list->next[free_addr] != 0)                     // если free_addr - не голова
    {
        list->prev[list->next[free_addr]] = free_addr; // теперь перед элементом, который лежал перед after_that лежит free_addr
    }
    else
    {
        list->head = free_addr;
    }

    list->next[after_that] = free_addr;                 // теперь после after_that лежит всавляемый элемент

    return LIST_OK;
}

list_err_t list_elem_pop(list* list, int that)
{
    int next_elem = list->next[that];
    int prev_elem = list->prev[that];

    if (that == 0 || prev_elem == -1 || next_elem == -1) { return LIST_POP_UNKNOWN_ELEM; }

    if (prev_elem != -1)                                     // если that не хвост
    {
        list->next[prev_elem] = next_elem;  // теперь после элемента перед that лежит следующий для that   
    }
    else
    {
        list->head = list->next[that];
    }

    if (next_elem != -1)                                     // если that не голова
    {
        list->prev[next_elem] = prev_elem;   // теперь перед элементом после that лежит задний для that
    }
    else
    {
        list->tail = list->prev[that];
    }

    list->prev[that] = -1;
    list->next[that] = -1;
    list->data[that] = -1;

    err_t push_err = stack_push(&free, that);
    if (push_err != LIST_OK) { return LIST_FREE_FAIL; }list_err_t 

    return LIST_OK;
}

lisr_err_t list_dump()
{

}

lisr_err_t list_verify(const list_t* list)
{
    if (list == NULL)        return LIST_NULL_PTR;

    if (list->data == NULL || head == -1 || tail == -1)  return LIST_CORRYPTED;

    // max_memory_on_list

    int now_elem = list->tail;
    for (int i = 0; i < MAX_DATA_SIZE; i++)
    {
        if (list->next[now_elem] != -1 && (now_elem = list->next[now_elem]) == head)
        {
            if (list->next[now_elem] == 0)
            {
                return LIST_OK;
            }
        }
    }

    return LIST_CORRYPTED;
}