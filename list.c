#include "list.h"

//============================= Проверка списка на валидность ============================
#define LIST_CHECK(expr)                                                                 \
    do {                                                                                 \
        list_err_t _e = (expr);                                                          \
        if (_e != LIST_OK)                                                               \
        {                                                                                \
            printf("%s:%d: %s -> error %d\n", __FILE__, __LINE__, #expr, _e);            \
        }                                                                                \
    } while (0)
//========================================================================================

int main()
{
    list_t list;
    LIST_CHECK(list_ctor(&list, 8));
    LIST_CHECK(list_verify(&list));

    int a = 0, b = 0, c = 0;
    LIST_CHECK(list_elem_push(&list, 10, 0, &a));   // 10
    LIST_CHECK(list_elem_push(&list, 20, a, &b));   // 10 20
    LIST_CHECK(list_elem_push(&list, 15, a, &c));   // 10 15 20
    LIST_CHECK(list_elem_push(&list, 5,  0, NULL)); // 5 10 15 20
    // вот тут надо будет задампить
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_pop(&list, c));
    // снова дампим
    LIST_CHECK(list_verify(&list));
}

list_err_t list_ctor(list_t* list, int initial_capacity)
{
    if (list == NULL)           return LIST_NULL_PTR;
    if (initial_capacity  < 2)  return LIST_BAD_CAPACITY;

    list->data     = NULL;
    list->next     = NULL;
    list->prev     = NULL;
    list->capacity = 0;
    list->size     = 0;

    list->data = (list_elem_t*)malloc((size_t)initial_capacity * sizeof(list_elem_t));
    if (list->data == NULL) return LIST_DATA_OUT_OF_MEMORY;

    list->next = malloc((size_t)initial_capacity * sizeof(int));
    if (list->next == NULL)
    {
        free(list->data); list->data = NULL;
        return LIST_NEXT_OUT_OF_MEMORY;
    }

    list->prev = malloc((size_t)initial_capacity * sizeof(int));
    if (list->prev == NULL)
    {
        free(list->data); list->data = NULL;
        free(list->next); list->next = NULL;
        return LIST_PREV_OUT_OF_MEMORY;
    }

    err_t stack_err = stack_ctor(&list->free, (uint64_t)initial_capacity);
    if (stack_err != STACK_OK)
    {
        free(list->data); list->data = NULL;
        free(list->next); list->next = NULL;
        free(list->prev); list->prev = NULL;
        return LIST_FREE_FAIL;
    }

    // свободные ячейки
    for (int i = 0; i < initial_capacity; i++)
    {
        list->next[i]  = -1;
        list->prev[i]  = -1;
        list->data[i]  = LIST_POISON;
    }

    // фиктивная ячейка 0
    list->next[0] = 0;
    list->prev[0] = 0;

    // кладем адреса в обратном порядке
    for (int i = initial_capacity - 1; i >= 1; i--)
    {
        if (stack_push(&list->free, (Elem_t)i) != STACK_OK)
        {
            stack_dtor(&list->free);
            free(list->data); list->data = NULL;
            free(list->next); list->next = NULL;
            free(list->prev); list->prev = NULL;
            return LIST_FREE_FAIL;
        }
    }

    list->capacity = initial_capacity;
    list->size = 0;

    return LIST_OK;
}

list_err_t list_dtor(list_t* list)
{
    if (list == NULL)       return LIST_NULL_PTR;
    if (list->data == NULL) return LIST_OK;         // или не создавался или уже dtor 

    stack_dtor(&list->free);

    free(list->data);
    free(list->next);
    free(list->prev);
    list->data = NULL;
    list->next = NULL;
    list->prev = NULL;
    list->capacity = 0;
    list->size     = 0;

    return LIST_OK;
}

list_err_t list_elem_push(list_t* list, list_elem_t elem, int after_that, int* out_index)
{
    if (list == NULL) return LIST_NULL_PTR;

    if (after_that < 0 || after_that >= list->capacity || list->prev[after_that] == -1)
    {
        return LIST_BAD_INDEX;
    }

    Elem_t tmp = 0;
    err_t pop_err = stack_pop(&list->free, &tmp);
    if (pop_err == STACK_UNDERFLOW) return LIST_ADD_TO_FULL_LIST;
    if (pop_err != STACK_OK)        return LIST_FREE_FAIL;
    int free_addr = (int)tmp;

    list->data[free_addr]  = elem;

    int after_next = list->next[after_that];

    list->next[free_addr]  = after_next;                // после новой ячейки - бывший next

    list->prev[free_addr]  = after_that;                // теперь перед (новой ячейкой)free_addr лежит after_that;
    list->prev[after_next] = free_addr;                 // перед бывшим next теперь новая ячейка

    list->next[after_that] = free_addr;                 // теперь после after_that лежит всавляемый элемент

    list->size++;

    if (out_index != NULL)  *out_index= free_addr;

    return LIST_OK;
}

list_err_t list_elem_pop(list_t* list, int that)
{
    if (list == NULL) return LIST_NULL_PTR;

    if (that <= 0 || that >= list->capacity || list->prev[that] == -1) return LIST_POP_UNKNOWN_ELEM;

    // сначала возвращаем ячейку в стек и если не получится - список не изменится
    if (stack_push(&list->free, (Elem_t)that) != STACK_OK)             return LIST_FREE_FAIL;

    int next_elem = list->next[that];
    int prev_elem = list->prev[that];

    list->next[prev_elem] = next_elem;
    list->prev[next_elem] = prev_elem;

    list->prev[that] = -1;
    list->next[that] = -1;
    list->data[that] = LIST_POISON;

    list->size--;

    return LIST_OK;
}

list_err_t list_verify(const list_t* list)
{
    if (list == NULL)        return LIST_NULL_PTR;

    if (list->data == NULL || list->next == NULL || list->prev == NULL)        return LIST_CORRYPTED;

    if (list->capacity < 2 || list->size < 0 || list->size >= list->capacity)  return LIST_CORRYPTED;

    // обход по кольцу от ячейки 0
    int cur   = 0;
    int count = 0;

    do
    {
        int nx = list->next[cur];
        if (nx < 0 || nx >= list->capacity)  return LIST_CORRYPTED;
        if (list->prev[nx] != cur)           return LIST_CORRYPTED;

        cur = nx;
        if (cur != 0)
        {
            count++;
            if (count > list->size) return LIST_CORRYPTED; // зацикливание мимо 0
        }

    } while (cur != 0);

    if (count != list->size) return LIST_CORRYPTED;

    // проверка, что свободных ячеек ровно capacity - size - 1
    if (stack_get_size(&list->free) != (size_t)(list->capacity - list->size - 1))  return LIST_CORRYPTED;

    return LIST_OK;
}

list_err_t list_dump(const list_t* list, FILE* out)
{

}