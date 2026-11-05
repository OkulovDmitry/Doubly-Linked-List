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

    int a = 0, b = 0, c = 0, d = 0;
    LIST_CHECK(list_elem_push(&list, 10, 1, &a));   // 10
    LIST_CHECK(list_elem_push(&list, 20, 2, &b));   // 10 20
    LIST_CHECK(list_elem_push(&list, 15, 2, &c));   // 10 15 20
    LIST_CHECK(list_elem_push(&list, 5,  1, NULL)); // 5 10 15 20

    FILE* list_dumps = fopen("list_dumps.txt", "w");
    if (list_dumps == NULL) return 1;
    list_dump(&list, list_dumps);                   // order: 5(4) 10(1) 15(3) 20(2)
    list_dump_graph(&list, "dump1.dot", "dump1.png");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_pop(&list, 2));            // 5 15 20  (убрали 10, ячейка 1 свободна)
    list_dump(&list, list_dumps);                   // order: 5(4) 15(3) 20(2), free_addr = 1
    list_dump_graph(&list, "dump2.dot", "dump2.png");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_push(&list, 99, 4, &d));   // 5 15 20 99, d должен быть 1 (переиспользовали ячейку)
    if (d != 1) printf("expected reuse of cell 1, got %d\n", d);
    if (list_elem_push(&list, 7, 0, NULL) != LIST_BAD_INDEX) printf("push(0) must fail\n");
    list_dump(&list, list_dumps);                   // order: 5(4) 15(3) 20(2) 99(1)
    list_dump_graph(&list, "dump3.dot", "dump3.png");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_dtor(&list));
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

    // свободные ячейки
    for (int i = 1; i < initial_capacity; i++)
    {
        list->next[i] = i + 1;
        list->prev[i] = - 1;
        list->data[i] = LIST_POISON;
    }
    list->next[initial_capacity - 1] = 0;  // без этого последняя указывает за грницу списка

    // фиктивная ячейка 0
    list->data[0] = LIST_POISON;
    list->next[0] = 0;
    list->prev[0] = 0;

    // кладем адрес свободной в free_addr первую ячейку
    list->free_addr = 1;

    list->capacity = initial_capacity;
    list->size = 0;

    return LIST_OK;
}

list_err_t list_dtor(list_t* list)
{
    if (list == NULL)       return LIST_NULL_PTR;
    if (list->data == NULL) return LIST_OK;         // или не создавался или уже dtor 

    list->free_addr = 0;

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

    if (that < 1 || that >= list->size + 2)
    {
        return LIST_BAD_INDEX;
    }

    if (list->free_addr == 0)       return LIST_ADD_TO_FULL_LIST;

    // теперь пользователь вводит не виртуальный номер, а логический и мы для поиска пробегаемся начиная с tail до элемента перед текущим

    int cur_elem = 0;
    for (int i = 0; i < that - 1; i++)
    {
        if (list->next[cur_elem] == 0) return LIST_BAD_INDEX;
        cur_elem = list->next[cur_elem];
    }

    // замена свободного элемента на следующий элемент за свободным, если след за ним несвободен - список заполнен полностью
    int free_addr_cpy = list->free_addr;
    list->free_addr   = list->next[free_addr_cpy];

    int after = list->next[cur_elem];        // перед этим нужно вставить элемент

    list->data[free_addr_cpy] = elem;        // включение свободного в двусвязный
    list->next[free_addr_cpy] = after;
    list->prev[free_addr_cpy] = cur_elem;

    list->prev[after]    = free_addr_cpy;    // теперь список знает о новом элементе
    list->next[cur_elem] = free_addr_cpy;

    list->size++;

    if (out_index != NULL)  *out_index= free_addr_cpy;

    return LIST_OK;
}

list_err_t list_elem_pop(list_t* list, int that)
{
    if (list == NULL) return LIST_NULL_PTR;

    if (list->prev[0] == 0)  return LIST_UNDERFLOW;

    if (that <= 0 || that > list->size) return LIST_POP_UNKNOWN_ELEM;

    int cur_elem = 0;

    for (int i = 0; i < that; i++)
    {
        if (list->next[cur_elem] == 0) return LIST_BAD_INDEX;
        cur_elem = list->next[cur_elem];
    }

    int next_elem = list->next[cur_elem];
    int prev_elem = list->prev[cur_elem];

    list->next[prev_elem] = next_elem;
    list->prev[next_elem] = prev_elem;

    list->data[cur_elem]  = LIST_POISON;
    int prev_free_addr    = list->free_addr;
    list->free_addr       = cur_elem;
    list->prev[cur_elem]  = -1;
    list->next[cur_elem]  = prev_free_addr;

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
    int free_count = 0;
    int cur_free = list->free_addr;

    while(cur_free != 0)
    {
        if (cur_free < 0 || cur_free >= list->capacity)  return LIST_CORRYPTED;
        if (list->prev[cur_free] != -1)                  return LIST_CORRYPTED;

        free_count++;
        if (free_count > list->capacity)                 return LIST_CORRYPTED;

        cur_free = list->next[cur_free];
    }

    // проверка, что свободных ячеек ровно capacity - size - 1
    if (free_count != list->capacity - list->size - 1)   return LIST_CORRYPTED;
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

list_err_t list_dump_graph(const list_t* list, const char* dot_name, const char* png_name)
{
    if (list == NULL || dot_name == NULL || png_name == NULL) return LIST_NULL_PTR;
    if (list->data == NULL || list->next == NULL || list->prev == NULL) return LIST_CORRYPTED;

    FILE* out = fopen(dot_name, "w");
    if (out == NULL) return LIST_NULL_PTR;

    fprintf(out, "digraph list {\n");
    fprintf(out, "  rankdir=LR;\n");
    fprintf(out, "  node [shape=record, style=\"rounded,filled\"];\n");
    fprintf(out, "  edge [arrowsize=0.7];\n");
    fprintf(out, "  nodesep=0.5;\n");
    fprintf(out, "  ranksep=0.9;\n");

    for (int i = 0; i < list->capacity; i++)
    {
        const char* color = "lightblue";                    // занятая
        if (i == 0) color = "lightgrey";                    // фиктивная
        else if (list->prev[i] == -1) color = "lightgreen"; // свободная

        fprintf(out, " cell%d [fillcolor=\"%s\", label=\"idx %d | data " LIST_ELEM_FMT " | <nx> next %d | <pv> prev %d\"];\n",
                           i,              color,            i,         list->data[i],              list->next[i], list->prev[i]);
    }

    // заставляет dot ставить ячейки слева направо по номерам
    for (int i = 0; i + 1 < list->capacity; i++)
    fprintf(out, "  cell%d -> cell%d [style=invis, weight=100];\n", i, i + 1);

    for (int i = 0; i < list->capacity; i++)
    {
        if (list->prev[i] != -1)   // фиктивная или занятая
        {
            fprintf(out, "  cell%d:nx -> cell%d [color=blue, constraint=false];\n", i, list->next[i]);
            fprintf(out, "  cell%d:pv -> cell%d [color=red,  constraint=false];\n", i, list->prev[i]);
        }
        else if (list->next[i] != 0) // свободная: next ведет к следующей свободной
        {
            fprintf(out, " cell%d:nx -> cell%d [color=darkgreen, style=dashed, constraint=false];\n",
                               i,            list->next[i]);
        }
    }

    if (list->free_addr != 0)
    {
        fprintf(out, "  FREE [shape=plaintext, style=\"\", label=\"free_addr\"];\n");
        fprintf(out, "  FREE -> cell%d [color=darkgreen];\n", list->free_addr);
    }

    fprintf(out, "}\n");
    fclose(out);

    char cmd[256] = {};
    snprintf(cmd, sizeof(cmd), "dot -Tpng %s -o %s", dot_name, png_name);
    if (system(cmd) != 0) return LIST_CORRYPTED;

    return LIST_OK;
}