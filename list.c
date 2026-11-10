#include "list.h"

//============================= Проверка списка на валидность ============================
#define LIST_CHECK(expr)                                                      \
    do {                                                                      \
        list_err_t _e = (expr);                                               \
        if (_e != LIST_OK) {                                                  \
            printf("%s:%d: %s -> error %d\n", __FILE__, __LINE__, #expr, _e); \
        }                                                                     \
    } while (0)
//========================================================================================

list_err_t list_ctor(list_t* list, int initial_capacity) {
    if (list == NULL) return LIST_NULL_PTR;
    if (initial_capacity < 2) return LIST_BAD_CAPACITY;

    list->data     = NULL;
    list->next     = NULL;
    list->prev     = NULL;
    list->capacity = 0;
    list->size     = 0;

    list->data = (list_elem_t*)malloc((size_t)initial_capacity * sizeof(list_elem_t));
    if (list->data == NULL) return LIST_DATA_OUT_OF_MEMORY;

    list->next = malloc((size_t)initial_capacity * sizeof(int));
    if (list->next == NULL) {
        free(list->data);
        list->data = NULL;
        return LIST_NEXT_OUT_OF_MEMORY;
    }

    list->prev = malloc((size_t)initial_capacity * sizeof(int));
    if (list->prev == NULL) {
        free(list->data);
        list->data = NULL;
        free(list->next);
        list->next = NULL;
        return LIST_PREV_OUT_OF_MEMORY;
    }

    // свободные ячейки
    for (int i = 1; i < initial_capacity; i++) {
        list->next[i] = i + 1;
        list->prev[i] = -1;
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
    list->size     = 0;

    return LIST_OK;
}

list_err_t list_dtor(list_t* list) {
    if (list == NULL) return LIST_NULL_PTR;
    if (list->data == NULL) return LIST_OK;  // или не создавался или уже dtor

    list->free_addr = 0;

    free(list->data);
    free(list->next);
    free(list->prev);
    list->data     = NULL;
    list->next     = NULL;
    list->prev     = NULL;
    list->capacity = 0;
    list->size     = 0;

    return LIST_OK;
}

const int iter_ok(const list_t* list, list_iter_t it) {
    return it.idx >= 0 && it.idx < list->capacity && (it.idx = 0 || list->prev[it.idx] != -1);
}

list_iter_t list_begin(const list_t* list) {
    assert(list != NULL);
    return (list_iter_t){list->next[0]};
}

list_iter_t list_end(const list_t* list) {
    assert(list != NULL);
    return (list_iter_t){0};
}

list_iter_t list_next(const list_t* list, list_iter_t it) {
    assert(list != NULL);
    assert(iter_ok(list, it) && it.idx != 0);
    return (list_iter_t){list->next[it.idx]};
}

list_iter_t list_prev(const list_t* list, list_iter_t it) {
    assert(list != NULL);
    assert(iter_ok(list, it));
    return (list_iter_t){list->prev[it.idx]};
}

int list_iter_eq(list_iter_t a, list_iter_t b) {
    return a.idx == b.idx;
}

list_elem_t list_get(const list_t* list, list_iter_t it) {
    assert(list != NULL);
    assert(iter_ok(list, it) && it.idx != 0);

    return list->data[it.idx];
}

list_err_t list_elem_push(list_t* list, list_elem_t elem, int that, int* out_index) {
    if (list == NULL) return LIST_NULL_PTR;

    if (that < 1 || that >= list->size + 2) {
        return LIST_BAD_INDEX;
    }

    if (list->free_addr == 0) return LIST_ADD_TO_FULL_LIST;

    // теперь пользователь вводит не виртуальный номер, а логический и мы для поиска пробегаемся начиная с tail до элемента перед текущим

    int cur_elem = 0;
    for (int i = 0; i < that - 1; i++) {
        if (list->next[cur_elem] == 0) return LIST_BAD_INDEX;
        cur_elem = list->next[cur_elem];
    }

    // замена свободного элемента на следующий элемент за свободным, если след за ним несвободен - список заполнен полностью
    int free_addr_cpy = list->free_addr;
    list->free_addr   = list->next[free_addr_cpy];

    int after = list->next[cur_elem];  // перед этим нужно вставить элемент

    list->data[free_addr_cpy] = elem;  // включение свободного в двусвязный
    list->next[free_addr_cpy] = after;
    list->prev[free_addr_cpy] = cur_elem;

    list->prev[after]    = free_addr_cpy;  // теперь список знает о новом элементе
    list->next[cur_elem] = free_addr_cpy;

    list->size++;

    if (out_index != NULL) *out_index = free_addr_cpy;

    return LIST_OK;
}

list_err_t list_elem_pop(list_t* list, int that) {
    if (list == NULL) return LIST_NULL_PTR;

    if (list->prev[0] == 0) return LIST_UNDERFLOW;

    if (that <= 0 || that > list->size) return LIST_POP_UNKNOWN_ELEM;

    int cur_elem = 0;

    for (int i = 0; i < that; i++) {
        if (list->next[cur_elem] == 0) return LIST_BAD_INDEX;
        cur_elem = list->next[cur_elem];
    }

    int next_elem = list->next[cur_elem];
    int prev_elem = list->prev[cur_elem];

    list->next[prev_elem] = next_elem;
    list->prev[next_elem] = prev_elem;

    list->data[cur_elem] = LIST_POISON;
    int prev_free_addr   = list->free_addr;
    list->free_addr      = cur_elem;
    list->prev[cur_elem] = -1;
    list->next[cur_elem] = prev_free_addr;

    list->size--;

    return LIST_OK;
}

list_err_t list_verify(const list_t* list) {
    if (list == NULL) return LIST_NULL_PTR;

    if (list->data == NULL || list->next == NULL || list->prev == NULL) return LIST_CORRYPTED;

    if (list->capacity < 2 || list->size < 0 || list->size >= list->capacity) return LIST_CORRYPTED;

    // обход по кольцу от ячейки 0
    int cur   = 0;
    int count = 0;

    do {
        int nx = list->next[cur];
        if (nx < 0 || nx >= list->capacity) return LIST_CORRYPTED;
        if (list->prev[nx] != cur) return LIST_CORRYPTED;

        cur = nx;
        if (cur != 0) {
            count++;
            if (count > list->size) return LIST_CORRYPTED;  // зацикливание мимо 0
        }

    } while (cur != 0);

    if (count != list->size) return LIST_CORRYPTED;

    // теперь вместо проверки на размер стека - нужно пробежаться по списку с free начиная с free_addr и проверить размер в конце
    int free_count = 0;
    int cur_free   = list->free_addr;

    while (cur_free != 0) {
        if (cur_free < 0 || cur_free >= list->capacity) return LIST_CORRYPTED;
        if (list->prev[cur_free] != -1) return LIST_CORRYPTED;

        free_count++;
        if (free_count > list->capacity) return LIST_CORRYPTED;

        cur_free = list->next[cur_free];
    }

    // проверка, что свободных ячеек ровно capacity - size - 1
    if (free_count != list->capacity - list->size - 1) return LIST_CORRYPTED;
    return LIST_OK;
}

list_err_t list_dump(const list_t* list, FILE* out) {
    if (list == NULL || out == NULL) return LIST_NULL_PTR;
    if (list->data == NULL || list->next == NULL || list->prev == NULL) {
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
    for (int steps = 0; steps < list->capacity && cur > 0 && cur < list->capacity; steps++) {
        fprintf(out, " " LIST_ELEM_FMT "(%d)", list->data[cur], cur);
        cur = list->next[cur];
    }

    fprintf(out, "\n");
    fprintf(out, LIST_SEP);

    return LIST_OK;
}

/*
Использованные элементы графвиза:
========
    fprintf(out, "digraph list {\n"); - заголовок графа
digraph - значит граф ориентированный
list - его имя
========
    graph [dpi=100, bgcolor="#2F353B", splines=ortho, nodesep=0.6, ranksep=1.2]; - настройка всего графа
dpi - расширение картинки
bgcolor - цвет фона
splines=ortho - стрелки под прямым углом
nodesep=0.6 - расстояние меж узлами в одном ряду
ranksep - расстояние между рядами
=======
node  [shape=rectangle, style=\"rounded,filled\", fillcolor=\"#C5D0E6\","
            " color=\"#C5D0E6\", fontsize=20, width=1.6, height=2.0, fixedsize=true, penwidth=3]; - настройки для всех узлов по умолчанию
shape=rectangle - узел - прямоугольник со скругенными концами и заливкой
fillcolor - цвет заливки
color - цвет рамки
fixedsize=true - размер строго width * height (в дюймах, текст не растягивается)
penwidth - толщина рамки
=======
    edge  [penwidth=2, arrowsize=1.0]; - толщина стрелок и размер наконечника
=======
Переходим к ячейкам:
    fprintf(out, "  cell0 [color=red, label=\"header\\n\\nhead: %d\\ntail: %d\\nfree: %d\"];\n",
cell0 - объявление узла и его имя
в [] атрибуты:
color = red - перекрывает начальную рамку
label - это тест внутри
=======
    fprintf(out, "  { rank=same;");
    for (...) fprintf(out, " cell%d;", i);
    fprintf(out, " }\n");
rank=same заставляет Graphviz поставить перечисленные узлы на один горизонтальный уровень
=======
cell%d -> cell%d [style=invis, weight=100];
невидимые стрелки 0->1->2->... с большим весом. В итоге графвиз мтремится сделать эти невидимые стрелки с большим 
весом короткими и прямыми, поэтому ячейки выстраиваются не как добно графвизу, а слева направо в порядке индексов
=======
if (pv == -1) {... цепочка белых свободных ...}
для свободной ячейки рисуется только белая стрелка next
=======
if (list->prev[nx] == i) зелёная  else красная, penwidth=4
Это проверка согласованности в картинке - если next ячейки i указывает на nx то prev[nx] обязан быть i -
 - если нет то стрелка красная и толстая
 Для prev то же самое в обратную - оранжевая или красная
*/
static void gv_edge(FILE* o, gv_mode_t mode, int from, const char* fport, int to, const char* tport,
                    const char* color, const char* attrs) {
    const char* sep = attrs[0] ? ", " : "";
    if (mode == GV_ARCS)
        fprintf(o, "  cell%d:%s -> cell%d:%s [color=\"%s\"%s%s];\n",
                from, fport, to, tport, color, sep, attrs);
    else
        fprintf(o, "  cell%d -> cell%d [color=\"%s\"%s%s];\n", from, to, color, sep, attrs);
}

list_err_t list_dump_graph(const list_t* list, const char* dot_name, const char* png_name, gv_mode_t mode) {
    if (list == NULL || dot_name == NULL || png_name == NULL) return LIST_NULL_PTR;
    if (list->data == NULL || list->next == NULL || list->prev == NULL) return LIST_CORRYPTED;

    FILE* out = fopen(dot_name, "w");
    if (out == NULL) return LIST_IO_ERROR;

    // ============================ заголовок =========================
    fprintf(out, "digraph list {\n");
    const char* splines = (mode == GV_ORTHO) ? "ortho" : "true";
    double nodesep      = (mode == GV_ORTHO) ? 2.4 : 1.6;
    fprintf(out, "  graph [dpi=100, bgcolor=\"" GV_BG
                 "\", splines=%s, pad=0.5,"
                 " nodesep=%.1f, ranksep=1.5];\n",
            splines, nodesep);
    fprintf(out, "  node  [shape=rectangle, style=\"rounded,filled\", fontcolor=\"" GV_TEXT
                 "\","
                 " fontsize=16, width=1.8, height=1.7, fixedsize=true, penwidth=3];\n");
    fprintf(out, "  edge  [penwidth=2, arrowsize=1.0];\n");

    // ============================== ячейки ==========================
    for (int i = 0; i < list->capacity; i++) {
        if (i == 0)  // header - фиктивная ячейка и free_addr
        {
            fprintf(out, "  cell0 [fillcolor=\"" GV_HDR_FILL "\", color=\"" GV_HDR_BORDER
                         "\","
                         " label=\"header\\n\\nhead: %d\\ntail: %d\\nfree: %d\"];\n",
                    list->next[0], list->prev[0], list->free_addr);
            continue;
        }

        int is_free     = (list->prev[i] == -1);
        const char* tag = "";
        if (!is_free) {
            int h = (i == list->next[0]), t = (i == list->prev[0]);
            tag = (h && t) ? " [HEAD+TAIL]" : h ? " [HEAD]"
                                          : t   ? " [TAIL]"
                                                : "";
        }

        fprintf(out, "  cell%d [fillcolor=\"%s\", color=\"%s\"%s, label=\"idx: %d%s\\n\\n" LIST_ELEM_FMT "\\nnxt: %d\\nprv: %d\"];\n",
                i,
                is_free ? GV_FREE_FILL : GV_USED_FILL,
                is_free ? GV_FREE_BORDER : GV_USED_BORDER,
                is_free ? ", style=\"rounded,filled,dashed\"" : "",
                i, tag, list->data[i], list->next[i], list->prev[i]);
    }

    // =========== все ячейки в один ряд, в физическом порядке ================
    fprintf(out, "  { rank=same;");
    for (int i = 0; i < list->capacity; i++) fprintf(out, " cell%d;", i);
    fprintf(out, " }\n");

    for (int i = 0; i + 1 < list->capacity; i++)
        fprintf(out, "  cell%d -> cell%d [style=invis, weight=100];\n", i, i + 1);

    // ============================== стрелки ==================================
    for (int i = 0; i < list->capacity; i++) {
        int nx = list->next[i];
        int pv = list->prev[i];

        if (pv == -1)  // свободная ячейка - серый пунктир по низу
        {
            if (nx >= 0 && nx < list->capacity && nx != i)
                gv_edge(out, mode, i, "s", nx, "s", GV_FREE_CHAIN, "style=dashed");
            continue;
        }

        if (nx >= 0 && nx < list->capacity && nx != i) {  // next идет по верху
            if (list->prev[nx] == i)
                gv_edge(out, mode, i, "n", nx, "n", GV_NEXT, "");
            else
                gv_edge(out, mode, i, "n", nx, "n", GV_BAD, "penwidth=4");
        }

        if (pv >= 0 && pv < list->capacity && pv != i) {  // prev по низу
            if (list->next[pv] == i)
                gv_edge(out, mode, i, "s", pv, "s", GV_PREV, "");
            else
                gv_edge(out, mode, i, "s", pv, "s", GV_BAD, "penwidth=4");
        }
    }

    // ====================== указатель на первую свободную ========================
    if (list->free_addr > 0 && list->free_addr < list->capacity)
        gv_edge(out, mode, 0, "s", list->free_addr, "s", GV_FREE_PTR, "penwidth=3, style=bold");

    fprintf(out, "}\n");
    fclose(out);

    // ================================ запуск dot ===================================
    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "dot -Tpng \"%s\" -o \"%s\"", dot_name, png_name);
    if (system(cmd) != 0) return LIST_CORRYPTED;

    return LIST_OK;
}

list_err_t list_dump_graph_both(const list_t* list, const char* base_name) {
    if (list == NULL || base_name == NULL) return LIST_NULL_PTR;

    char dot[256], png[256];
    list_err_t err;

    snprintf(dot, sizeof(dot), "%s_arcs.dot", base_name);
    snprintf(png, sizeof(png), "%s_arcs.png", base_name);
    err = list_dump_graph(list, dot, png, GV_ARCS);
    if (err != LIST_OK) return err;

    snprintf(dot, sizeof(dot), "%s_ortho.dot", base_name);
    snprintf(png, sizeof(png), "%s_ortho.png", base_name);
    return list_dump_graph(list, dot, png, GV_ORTHO);
}