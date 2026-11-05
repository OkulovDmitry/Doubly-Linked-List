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


    FILE* list_dumps = fopen("list_dumps.txt", "w");
    list_dump(&list, list_dumps);

    LIST_CHECK(list_verify(&list));

    list_dump(&list, list_dumps);
    LIST_CHECK(list_verify(&list));

    fclose(list_dumps);
    return 0;
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

    /*err_t stack_err = stack_ctor(&list->free, (uint64_t)initial_capacity);
    if (stack_err != STACK_OK)
    {
        free(list->data); list->data = NULL;
        free(list->next); list->next = NULL;
        free(list->prev); list->prev = NULL;
        return LIST_FREE_FAIL;
    }*/

    // свободные ячейки
    for (int i = 1; i < initial_capacity; i++)
    {
        list->next[i] = i + 1;
        list->prev[i] = - 1;
        list->data[i] = LIST_POISON;
    }
    /*for (int i = 0; i < initial_capacity; i++)
    {
        list->next[i]  = -1;
        list->prev[i]  = -1;
        list->data[i]  = LIST_POISON;
    }*/

    // фиктивная ячейка 0
    list->data[0] = LIST_POISON;
    list->next[0] = 0;
    list->prev[0] = 0;

    // кладем адрес свободной в free_addr первую ячейку
    list->free_addr = 1;

    // кладем адреса в обратном порядке
    /*for (int i = initial_capacity - 1; i >= 1; i--)
    {
        if (stack_push(&list->free, (Elem_t)i) != STACK_OK)
        {
            stack_dtor(&list->free);
            free(list->data); list->data = NULL;
            free(list->next); list->next = NULL;
            free(list->prev); list->prev = NULL;
            return LIST_FREE_FAIL;
        }
    }*/

    list->capacity = initial_capacity;
    list->size = 0;

    return LIST_OK;
}

list_err_t list_dtor(list_t* list)
{
    if (list == NULL)       return LIST_NULL_PTR;
    if (list->data == NULL) return LIST_OK;         // или не создавался или уже dtor 

    list->free_addr = 0;
    //stack_dtor(&list->free);

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

list_err_t list_elem_push(list_t* list, list_elem_t elem, int that, int* out_index)
{
    if (list == NULL) return LIST_NULL_PTR;

    if (that < 0 || that >= list->size + 2)
    {
        return LIST_BAD_INDEX;
    }

    /*Elem_t tmp = 0;
    err_t pop_err = stack_pop(&list->free, &tmp);
    if (pop_err == STACK_UNDERFLOW) return LIST_ADD_TO_FULL_LIST;
    if (pop_err != STACK_OK)        return LIST_FREE_FAIL;*/
    // замена свободного элемента на следующий элемент за свободным, если след за ним несвободен - список заполнен полностью
    if (list->free_addr == 0)       return LIST_ADD_TO_FULL_LIST;
    int free_addr_cpy = list->free_addr;
    list->free_addr = (list->data[list->next[free_addr_cpy]] != LIST_POISON) 
                      ?  list->next[free_addr_cpy]
                      : 0;

    // теперь пользователь вводит не виртуальный номер, а реальный и мы для поиска пробегаемся начиная с tail до нужного элемента

    // случай добавления в пустой список
    int cur_elem = 0;
    if (list->next[cur_elem] == 0) { list->next[0] = cur_elem;
                                     list->prev[0] = cur_elem;}
    for (int i = 0; i < that; i++)
    {
        if (list->data[list->next[cur_elem]] == LIST_POISON)  return LIST_BAD_INDEX;
        cur_elem = list->next[cur_elem];
    }

    list->data[free_addr_cpy] = elem; 

    int after_that = list->next[cur_elem];
    int prev_that  = list->prev[cur_elem];

    list->next[free_addr_cpy]  = after_that;                // после новой ячейки - бывший next для cur_elem
    list->prev[free_addr_cpy]  = prev_that;                 // теперь перед (новой ячейкой)free_addr лежит prev для cur_elem;

    list->prev[after_that] = free_addr_cpy;                 // перед бывщим следующим для cur_elem теперь free_addr
    list->next[that]       = free_addr_cpy;                 // теперь после that лежит вставляемый элемент

    list->size++;

    if (out_index != NULL)  *out_index= free_addr_cpy;

    return LIST_OK;
}

list_err_t list_elem_pop(list_t* list, int that)
{
    if (list == NULL) return LIST_NULL_PTR;

    if (that <= 0 || that > list->size) return LIST_POP_UNKNOWN_ELEM;

    if (list->prev[0] == 0)  return LIST_UNDERFLOW;

    int cur_elem = 0;

    for (int i = 0; i < that; i++)
    {
        if (list->data[list->next[cur_elem]] == LIST_POISON)  return LIST_BAD_INDEX;
        cur_elem = list->next[cur_elem];
    }

    int next_elem = list->next[cur_elem];
    int prev_elem = list->prev[cur_elem];

    list->next[prev_elem] = next_elem;
    list->prev[next_elem] = prev_elem;

    // теперь в отличие от случая со стеком нужно перезаписать свободный элемент сохраняя последовательную свзяь свободных
    list->data[cur_elem]  = LIST_POISON;
    int prev_free_addr    = list->free_addr;
    list->free_addr       = cur_elem;
    list->prev[cur_elem]  = -1;
    list->next[cur_elem]  = prev_free_addr;

    //list->prev[that] = -1;
    //list->next[that] = -1;
    //list->data[that] = LIST_POISON;

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

    // теперь вместо проверки на размер стека - нужно пробежаться по списку с free начиная с free_addr и проверить размер в конце
    int cur_num_of_free = 0;
    int cur_free = list->free_addr;

    for (cur_num_of_free; list->next[cur_free] != LIST_POISON; cur_num_of_free++)
        cur_free = list->data[list->next[cur_free]];

    // проверка, что свободных ячеек ровно capacity - size - 1
    if (cur_num_of_free != (size_t)(list->capacity - list->size - 1))  return LIST_CORRYPTED;
    //if (stack_get_size(&list->free) != (size_t)(list->capacity - list->size - 1))  return LIST_CORRYPTED;

    return LIST_OK;
}

list_err_t list_dump(const list_t* list, FILE* out)
{
    if (list == NULL || out == NULL)  return LIST_NULL_PTR;
    if (list->data == NULL || list->next == NULL || list->prev == NULL)
    {
        fprintf(out, "I TIRED, THINK LATER");
        return LIST_CORRYPTED;
    }

    fprintf(out, LIST_SEP);

    fprintf(out, "list [%p]: size = %d, capacity = %d\n", (const void*)list, list->size, list->capacity);

    fprintf(out, "  idx : ");
    for (int i = 0; i < list->capacity; i++) fprintf(out, "%6d ", i);

    fprintf(out, "\n  data: ");
    for (int i = 0; i < list->capacity; i++) fprintf(out, "%6.1f ", list->data[i]);

    fprintf(out, "\n  next: ");
    for (int i = 0; i < list->capacity; i++) fprintf(out, "%6d ", list->next[i]);
    fprintf(out, "\n  prev: ");
    for (int i = 0; i < list->capacity; i++) fprintf(out, "%6d ", list->prev[i]);
    fprintf(out, "\n");

    // логический порядок элементов
    fprintf(out, "  order:");
    int cur = list->next[0];
    for (int steps = 0; steps < list->capacity && cur > 0 && cur < list->capacity; steps++)
    {
        fprintf(out, " " LIST_ELEM_FMT "(%d)", list->data[cur], cur);
        cur = list->next[cur];
    }

    fprintf(out, "\n");
    fprintf(out, LIST_SEP);

    return LIST_OK;
}