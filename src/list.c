#include "list.h"

#define GV_BG "#1B1F24"         // фон
#define GV_TEXT "#E6EDF3"       // текст
#define GV_USED_FILL "#2F5D8A"  // занятая ячейка
#define GV_USED_BORDER "#6CA6E0"
#define GV_FREE_FILL "#2B3036"  // свободная ячейка
#define GV_FREE_BORDER "#6E7681"
#define GV_HDR_FILL "#5e4592"  // ячейка 0
#define GV_HDR_BORDER "#B392F0"
#define GV_NEXT "#3FB950"        // next
#define GV_PREV "#F0883E"        // prev
#define GV_BOTH "#187a06"        // согласованная связь next + prev
#define GV_FREE_CHAIN "#8B949E"  // цепочка свободных
#define GV_FREE_PTR "#D29922"    // указатель free
#define GV_BAD "#F85149"         // рассогласование

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

int iter_ok(const list_t* list, list_iter_t it) {
    return it.idx >= 0 && it.idx < list->capacity && (it.idx == 0 || list->prev[it.idx] != -1);
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

// вставка после it. вставка после end - вставка в начало
list_err_t list_insert_after(list_t* list, list_iter_t it, list_elem_t elem, list_iter_t* out) {
    if (list == NULL) return LIST_NULL_PTR;
    assert(iter_ok(list, it));
    if (list->free_addr == 0) return LIST_ADD_TO_FULL_LIST;

    int fresh       = list->free_addr;
    list->free_addr = list->next[fresh];

    int after = list->next[it.idx];

    list->data[fresh] = elem;
    list->next[fresh] = after;
    list->prev[fresh] = it.idx;

    list->prev[after]  = fresh;
    list->next[it.idx] = fresh;

    list->size++;
    if (out != NULL) out->idx = fresh;
    return LIST_OK;
}

// вставка перед it. вставка перед end - втсавка в конец
list_err_t list_insert_before(list_t* list, list_iter_t it, list_elem_t elem, list_iter_t* out) {
    if (list == NULL) return LIST_NULL_PTR;
    assert(iter_ok(list, it));
    return list_insert_after(list, (list_iter_t){list->prev[it.idx]}, elem, out);
}

list_err_t list_erase(list_t* list, list_iter_t it, list_iter_t* out_next) {
    if (list == NULL) return LIST_NULL_PTR;
    assert(iter_ok(list, it) && it.idx != 0);

    int pv = list->prev[it.idx];
    int nx = list->next[it.idx];

    list->next[pv] = nx;
    list->prev[nx] = pv;

    list->data[it.idx] = LIST_POISON;
    list->prev[it.idx] = -1;
    list->next[it.idx] = list->free_addr;
    list->free_addr    = it.idx;

    list->size--;
    if (out_next != NULL) out_next->idx = nx;
    return LIST_OK;
}

list_err_t list_push_front(list_t* list, list_elem_t elem, list_iter_t* out) {
    if (list == NULL) return LIST_NULL_PTR;
    return list_insert_after(list, list_end(list), elem, out);
}

list_err_t list_push_back(list_t* list, list_elem_t elem, list_iter_t* out) {
    if (list == NULL) return LIST_NULL_PTR;
    return list_insert_before(list, list_end(list), elem, out);
}

list_err_t list_nth(const list_t* list, int that, list_iter_t* out) {
    if (list == NULL || out == NULL) return LIST_NULL_PTR;
    if (that < 1 || that > list->size) return LIST_BAD_INDEX;

    list_iter_t it = list_begin(list);
    for (int i = 1; i < that; i++) it = list_next(list, it);

    *out = it;
    return LIST_OK;
}

list_err_t list_elem_push(list_t* list, list_elem_t elem, int that, list_iter_t* out_index) {
    if (list == NULL) return LIST_NULL_PTR;

    if (that < 1 || that > list->size + 1) {
        return LIST_BAD_INDEX;
    }

    list_iter_t pos = list_end(list);  // that == size + 1 - вставка в конец
    if (that <= list->size) {
        list_err_t err = list_nth(list, that, &pos);
        if (err != LIST_OK) return err;
    }

    return list_insert_before(list, pos, elem, out_index);
}

list_err_t list_elem_pop(list_t* list, int that) {
    if (list == NULL) return LIST_NULL_PTR;

    if (list->prev[0] == 0) return LIST_UNDERFLOW;

    list_iter_t pos = list_end(list);
    list_err_t err  = list_nth(list, that, &pos);
    if (err != LIST_OK) return LIST_POP_UNKNOWN_ELEM;

    return list_erase(list, pos, NULL);
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
    for (int i = 0; i < list->capacity; i++) {
        if (list->data[i] == LIST_POISON)
            fprintf(out, "POISON ");
        else
            fprintf(out, "%6.1g ", list->data[i]);
    }

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

void log_init() {
    FILE* log = fopen("log.html", "w");
    if (!log) return;

    /* 1 строка - говорит браузеру чт осейчас будет современный HTML5 и его нужно читать по актуальным правилам
       meta - это для текста на русском
       4 строка - задает текст, который будет написан на самой вкладке в браузере рядом с иконкой
       далее в стайле задается правила внешнего вида для тегов:
           выбираем тег боди и пишем условия: цвета окружения текста отступы и прочее
           далее следующий тег - натсройка внешнего вида главного заголовка страницы и самой карточки дампа
           и следующий тег - натсройка дампа самой картинки
           и далее настройка текста внутри карточки  - где и почему вызван дамп, таблички с переменными окраска и прочее
           и завершающий блок - чтобы картинка не ломала верстку и читалась на темном фоне
       */

    fprintf(log,
            "<!DOCTYPE html>\n"
            "<html>\n<head>\n"
            " <meta charset=\"UTF-8\">\n"
            " <title>List Lifecycle Log</title>\n"
            "<style>\n"
            "   body {\n"
            "       font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
            "       background-color: #1e1e1e;\n"
            "       color: #d4d4d4;\n"
            "       padding: 30px;\n"
            "       margin: 0;\n"
            "   }\n"
            "   h1 {\n"
            "       color: #ffffff;\n"
            "       border-bottom: 2px solid #3c3c3c;\n"
            "       padding-bottom: 10px;\n"
            "   }\n"
            "   .dump-card {\n"
            "       background-color: #252526;\n"
            "       border: 1px solid #3c3c3c;\n"
            "       border-left: 5px solid #569cd6;\n"
            "       border-radius: 6px;\n"
            "       padding: 20px;\n"
            "       margin-bottom: 25px;\n"
            "       box-shadow: 0 4px 6px rgba(0,0,0,0.3);\n"
            "   }\n"
            "   .meta {\n"
            "       font-size: 0.95em;\n"
            "       color: #85c1e9;\n"
            "       margin-bottom: 10px;\n"
            "   }\n"
            "   .meta-func { color: #dcdcaa; font-weight: bold; }\n"
            "   .meta-file { color: #ce9178; }\n"
            "   .stats-table {\n"
            "       border-collapse: collapse;\n"
            "       margin: 10px;\n"
            "   }\n"
            "   .stats-table td {\n"
            "       padding: 4px 15px 4px 0;\n"
            "   }\n"
            "   .var-name { color: #9cdcfe; font-weight: bold; }\n"
            "   .var-val { color: #b5cea8; }\n"
            "   .graph-container {\n"
            "       background-color: #ffffff;\n"
            "       padding: 15px;\n"
            "       border-radius: 4px;\n"
            "       margin-top: 15px;\n"
            "       display: inline-block;\n"
            "       max-width: 100%%;\n"
            "       overflow-x: auto;\n"
            "   }\n"
            "   img {\n"
            "       max-height: 600px;\n"
            "       display: block;\n"
            "   }\n"
            "</style>\n"
            "</head>\n<body>\n<h1>List Execution History</h1>\n");
    fclose(log);
}

static void gv_edge(FILE* o, gv_mode_t mode, int from, const char* fport, int to, const char* tport,
                    const char* color, const char* attrs) {
    const char* sep = attrs[0] ? ", " : "";
    if (mode == GV_ARCS)
        fprintf(o, "  cell%d:%s -> cell%d:%s [color=\"%s\"%s%s];\n",
                from, fport, to, tport, color, sep, attrs);
    else {
        fprintf(o, "  cell%d -> cell%d [color=\"%s\"%s%s];\n", from, to, color, sep, attrs);

        int span   = from > to ? from - to : to - from;
        int copies = span > 1 ? (span > 4 ? 2 : 1) : 0;  // 0 для соседних, 1-2 для остальных
        if (span > 0) {
            // невидимые двойники
            for (int k = 0; k < copies; k++)
                fprintf(o, "  cell%d -> cell%d [style=invis];\n", from, to);
        }
    }
}

list_err_t list_dump_graph(const list_t* list, const char* dot_name, const char* png_name, gv_mode_t mode) {
    if (list == NULL || dot_name == NULL || png_name == NULL) return LIST_NULL_PTR;
    if (list->data == NULL || list->next == NULL || list->prev == NULL) return LIST_CORRYPTED;
    static int dump_id = 0;

    FILE* out = fopen(dot_name, "w");
    if (out == NULL) return LIST_IO_ERROR;

    dump_id++;

    FILE* log_file = fopen("log.html", "a");
    if (log_file == NULL) {
        fclose(out);
        return LIST_IO_ERROR;
    }

    const char* mode_str = (mode == GV_ORTHO) ? "ORTHO" : "ARCS";

    fprintf(log_file, "<hr>\n");                        // разделительная линия
    fprintf(log_file, "<h2> DUMP %d</h2>\n", dump_id);  // в будущем добавить макрос на еще откуда он вызван
    // и еще не 1 2 3 ... дампы а 1.1 1.2 2.1 ...
    fprintf(log_file, "<p>Size: %d, Capacity: %d</p>\n", list->size, list->capacity);

    // ============================ заголовок =========================
    fprintf(out, "digraph list {\n");
    const char* splines = (mode == GV_ORTHO) ? "ortho" : "true";
    double nodesep      = (mode == GV_ORTHO) ? 2.0 : 1.6;
    double ranksep      = (mode == GV_ORTHO) ? 2.5 : 1.5;
    fprintf(out, "  graph [dpi=110, bgcolor=\"" GV_BG
                 "\", splines=%s, pad=0.5,"
                 " nodesep=%.1f, ranksep=%.1f];\n",
            splines, nodesep, ranksep);
    fprintf(out, "  node  [shape=rectangle, style=\"rounded,filled\", fontcolor=\"" GV_TEXT
                 "\","
                 " fontsize=22, width=2.2, height=2.0, fixedsize=true, penwidth=3];\n");
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

        if (list->data[i] != LIST_POISON) {
            fprintf(out, "  cell%d [fillcolor=\"%s\", color=\"%s\"%s, label=\"idx: %d%s\\n\\n" LIST_ELEM_FMT "\\nnxt: %d\\nprv: %d\"];\n",
                    i,
                    is_free ? GV_FREE_FILL : GV_USED_FILL,
                    is_free ? GV_FREE_BORDER : GV_USED_BORDER,
                    is_free ? ", style=\"rounded,filled,dashed\"" : "",
                    i, tag, list->data[i], list->next[i], list->prev[i]);
        } else {
            fprintf(out, "  cell%d [fillcolor=\"%s\", color=\"%s\"%s, label=\"idx: %d%s\\n\\n%s\\nnxt: %d\\nprv: %d\"];\n",
                    i,
                    is_free ? GV_FREE_FILL : GV_USED_FILL,
                    is_free ? GV_FREE_BORDER : GV_USED_BORDER,
                    is_free ? ", style=\"rounded,filled,dashed\"" : "",
                    i, tag, "POISON", list->next[i], list->prev[i]);
        }
    }

    // =========== все ячейки в один ряд, в физическом порядке ================
    fprintf(out, "  { rank=same;");
    for (int i = 0; i < list->capacity; i++) fprintf(out, " cell%d;", i);
    fprintf(out, " }\n");

    for (int i = 0; i + 1 < list->capacity; i++)
        fprintf(out, "  cell%d -> cell%d [style=invis, weight=100];\n", i, i + 1);

    // ---- невидимые узлы дают высоту каналу для ortho ----
    if (mode == GV_ORTHO) {
        fprintf(out, "  top [style=invis, shape=point, width=0.01, label=\"\"];\n");
        fprintf(out, "  bot [style=invis, shape=point, width=0.01, label=\"\"];\n");
        fprintf(out, "  top -> cell0 [style=invis, weight=0];\n");
        fprintf(out, "  cell0 -> bot [style=invis, weight=0];\n");
    }

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
                gv_edge(out, mode, i, "n", nx, "n", GV_BOTH, "dir=both");
            else
                gv_edge(out, mode, i, "n", nx, "n", GV_BAD, "penwidth=4");
        }

        if (pv >= 0 && pv < list->capacity && pv != i) {  // prev по низу
            if (list->next[pv] == i) {
            }
            //gv_edge(out, mode, i, "s", pv, "s", GV_PREV ":" GV_PREV, "dir=both");
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
    if (system(cmd) != 0) {
        fprintf(log_file, "<img src=\"%s\" alt=\"List corrypted\"><br>\n", png_name);
        fclose(log_file);
        return LIST_CORRYPTED;
    }
    fprintf(log_file, "<img src=\"%s\" alt=\"List State\"><br>\n", png_name);
    fclose(log_file);
    return LIST_OK;
}

list_err_t list_dump_graph_both(const list_t* list, const char* base_name) {
    if (list == NULL || base_name == NULL) return LIST_NULL_PTR;

    char dot[256], png[256];
    list_err_t err;

    snprintf(dot, sizeof(dot), "./dot_of_dumps/%s_arcs.dot", base_name);
    snprintf(png, sizeof(png), "./png_dumps/%s_arcs.png", base_name);
    err = list_dump_graph(list, dot, png, GV_ARCS);
    if (err != LIST_OK) return err;

    snprintf(dot, sizeof(dot), "./dot_of_dumps/%s_ortho.dot", base_name);
    snprintf(png, sizeof(png), "./png_dumps/%s_ortho.png", base_name);
    return list_dump_graph(list, dot, png, GV_ORTHO);
}