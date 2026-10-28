#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stack.h"

typedef double        list_elem_t;
#define LIST_ELEM_FMT "%f"
#define LIST_POISON   ((list_elem_t) - 1)   // значение в свободных ячейках

typedef enum 
{
    LIST_OK                 = 0,
    LIST_NULL_PTR           = 1,
    LIST_OUT_OF_MEMORY      = 2,
    LIST_DATA_OUT_OF_MEMORY = 3,
    LIST_NEXT_OUT_OF_MEMORY = 4,
    LIST_PREV_OUT_OF_MEMORY = 5,
    LIST_FREE_FAIL          = 6,
    LIST_ADD_TO_FULL_LIST   = 7,
    LIST_POP_UNKNOWN_ELEM   = 8,
    LIST_CORRYPTED          = 9,
    LIST_BAD_INDEX          = 10,
    LIST_BAD_CAPACITY       = 11,
} list_err_t;

/*
    Параметры списка:
        Список на массивах
        Ячейка 0 - фиктивный элемент - кольцо
        next[0] - первый элемент    (tail)
        prev[0] - последний элемент (head)
        пустой список: next[0] == prev[0] == 0
        Свободная ячейка: next == prev == -1, data == LIST_POISON
        Адреса свободных ячеек храним в стеке free
*/

typedef struct
{
    list_elem_t* data;
    int*         next;
    int*         prev;
    stack_t      free;
    int          capacity;   //  вместе с фиктивной ячейкой
    int          size;       //  количество несвободных ячеек (без фиктивной)
} list_t;

list_err_t list_ctor(list_t* list, int initial_capacity);
list_err_t list_dtor(list_t* list);

list_err_t list_elem_push(list_t* list, list_elem_t elem, int after_that, int* out_index);
list_err_t list_elem_pop(list_t* list, int that);

list_err_t list_verify(const list_t* list);
list_err_t list_dump(const list_t* list, FILE* out);

#endif //LIST_H