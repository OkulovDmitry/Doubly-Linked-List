#ifndef LIST_H
#define LIST_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include "stack.h"

typedef double list_elem_t;
#define LIST_ELEM_FMT "%g"
#define LIST_POISON ((list_elem_t)0xDEADBEEF)  // значение в свободных ячейках

#define LIST_SEP "#######################################################################################\n"

#define LIST_DEBUG
#define LIST_ABORT_ON_CORRUPT

#ifdef LIST_DEBUG
#    define LIST_PLACE_IN , const char *name, const char *function, const char *file, int line
#    define LIST_PLACE_OUT(l) , #l, __func__, __FILE__, __LINE__
#else
#    define LIST_PLACE_IN
#    define LIST_PLACE_OUT(l)
#endif

typedef enum { GV_ARCS,
               GV_ORTHO } gv_mode_t;

typedef enum {
    LIST_OK = 0,
    //WARNING
    LIST_NULL_PTR           = 1,
    LIST_BAD_CAPACITY       = 2,
    LIST_ADD_TO_FULL_LIST   = 3,
    LIST_BAD_INDEX          = 4,
    LIST_UNDERFLOW          = 5,
    LIST_POP_UNKNOWN_ELEM   = 6,
    LIST_IO_ERROR           = 7,
    LIST_DATA_OUT_OF_MEMORY = 8,
    LIST_NEXT_OUT_OF_MEMORY = 9,
    LIST_PREV_OUT_OF_MEMORY = 10,
    //ERROR
    LIST_ERROR_BASE        = 100,
    LIST_CORRYPTED         = 101,  // нулевые массивы, плохие capacity/size
    LIST_NEXT_BROKEN       = 102,  // next вышел за границы или prev[next[i]] != i
    LIST_SIZE_MISMATCH     = 103,  // длина кольца != size
    LIST_FREE_CHAIN_BROKEN = 104,  // цепочка свободных испорчена
} list_err_t;

#define LIST_IS_WARNING(e) ((e) != LIST_OK && (e) < LIST_ERROR_BASE)
#define LIST_IS_ERROR(e) ((e) >= LIST_ERROR_BASE)

/*
//
    Параметры списка:
        Список на массивах
        Ячейка 0 - фиктивный элемент - кольцо
        next[0] - первый элемент    (tail)
        prev[0] - последний элемент (head)
        пустой список: next[0] == prev[0] == 0
        Свободная ячейка: data == LIST_POISON prev = -1 next  = адрес следующей свободной ячейки - теперь свободные образуют односвзный список
        Адреса свободных ячеек лежат друг за другом по next в списке, следующая свободная - free_addr
*/

typedef struct
{
    list_elem_t* data;
    int* next;
    int* prev;
    int free_addr;
    //stack_t      free;
    int capacity;  //  вместе с фиктивной ячейкой
    int size;      //  количество несвободных ячеек (без фиктивной)
#ifdef LIST_DEBUG
    const char* name;
    const char* function;
    const char* file;
    int line;
#endif
} list_t;

typedef struct
{
    int idx;
} list_iter_t;

/* ---- создание и удаление ---- */
list_err_t list_ctor(list_t* list, int initial_capacity LIST_PLACE_IN);
#define LIST_CTOR(l, cap) list_ctor(&(l), (cap) LIST_PLACE_OUT(l))
list_err_t list_dtor(list_t* list);

/* ---- итераторы за O(1) ---- */
list_iter_t list_begin(const list_t* list);
list_iter_t list_end(const list_t* list);
list_iter_t list_next(const list_t* list, list_iter_t it);
list_iter_t list_prev(const list_t* list, list_iter_t it);
int list_iter_eq(list_iter_t a, list_iter_t b);
list_elem_t list_get(const list_t* list, list_iter_t it);

/* ---- вставка/удаление по итератору за O(1) ---- */
list_err_t list_insert_after(list_t* list, list_iter_t it, list_elem_t elem, list_iter_t* out);
list_err_t list_insert_before(list_t* list, list_iter_t it, list_elem_t elem, list_iter_t* out);
list_err_t list_erase(list_t* list, list_iter_t it, list_iter_t* out_next);
list_err_t list_push_front(list_t* list, list_elem_t elem, list_iter_t* out);
list_err_t list_push_back(list_t* list, list_elem_t elem, list_iter_t* out);

/* ---- по логическому номер (с 1) за O(n) на поиск позиции ---- */
list_err_t list_nth(const list_t* list, int that, list_iter_t* out);
list_err_t list_elem_push(list_t* list, list_elem_t elem, int that, list_iter_t* out_index);
list_err_t list_elem_pop(list_t* list, int that);

/* ---- отладка ---- */
const char* list_strerror(list_err_t err);
void list_perror(const char* prefix, list_err_t err);

void log_error(const char* file, int line, const char* expr, list_err_t err);

list_err_t list_verify(const list_t* list);
void list_dump_html(const list_t* list, list_err_t err,
                    const char* func, const char* file, int line);
list_err_t list_dump(const list_t* list, FILE* out);

void log_init();

list_err_t list_dump_graph(const list_t* list, const char* dot_name, const char* png_name, gv_mode_t mode);
list_err_t list_dump_graph_both(const list_t* list, const char* base_name);

#endif  //LIST_H