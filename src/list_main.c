#include "list.h"

//============================= Проверка списка на валидность ============================
#define LIST_CHECK(expr)                                                \
    do {                                                                \
        list_err_t _e = (expr);                                         \
        if (_e != LIST_OK) {                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);  \
            list_perror("  reason", _e);                                \
        }                                                               \
    } while (0)
//========================================================================================

int main() {
    log_init();

    list_t list;
    LIST_CHECK(LIST_CTOR(list, 27));
    LIST_CHECK(list_verify(&list));

    list_iter_t a, b, c, d;
    LIST_CHECK(list_elem_push(&list, 10, 1, &a));    // 10
    LIST_CHECK(list_elem_push(&list, 20, 2, &b));    // 10 20
    LIST_CHECK(list_elem_push(&list, 15, 2, &c));    // 10 15 20
    LIST_CHECK(list_elem_push(&list, 5, 1, NULL));   // 5 10 15 20
    LIST_CHECK(list_elem_push(&list, 30, 5, NULL));  // 5 10 15 20 30
    LIST_CHECK(list_elem_push(&list, 40, 3, NULL));  // 5 10 40 15 20 30     (ячейка 6, вставка в середину)
    for (int i = 0; i < 14; i++) {
        LIST_CHECK(list_elem_push(&list, 50 + 10 * i, list.size + 1, NULL));
    }
    // ---- нагрузка стрелками при надобности ----
    LIST_CHECK(list_elem_push(&list, 111, 10, NULL));
    LIST_CHECK(list_elem_push(&list, 222, 15, NULL));
    LIST_CHECK(list_elem_push(&list, 333, 5, NULL));

    FILE* list_dumps = fopen("list_dumps.txt", "w");
    if (list_dumps == NULL) return 1;
    //list.prev[1] = -123;
    list_dump(&list, list_dumps);  // order: 5(4) 10(1) 15(3) 20(2) ...
    list_dump_graph_both(&list, "dump1");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_pop(&list, 2));  // 5 15 20  (убрали 10, ячейка 1 свободна)
    list_dump(&list, list_dumps);         // order: 5(4) 15(3) 20(2) 30(5) ..., free_addr = 1
    list_dump_graph_both(&list, "dump2");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_push(&list, 99, 4, &d));
    if (d.idx != 1) printf("expected reuse of cell 1, got %d\n", d.idx);
    if (list_elem_push(&list, 7, 0, NULL) != LIST_BAD_INDEX) printf("push(0) must fail\n");

    // ---- ломают список ----
    //list.prev[3] = 7;    // prev у 15 указывает не на 40 а на 7
    list.next[6] = 4;  // next у 40 указывает на голову и элементы теряются из порядка
    //list.prev[2] = -1;  // занятая ячейка 2 помечена как свободная ну и дополнительно в свободных написано POISON а тут будет 20.00000

    // ---- для проверки дампа с ошибкой внутри функции списка ----
    LIST_CHECK(list_elem_pop(&list, 20));

    list_dump(&list, list_dumps);  // order: 5(4) 15(3) 20(2) 99(1)
    list_dump_graph_both(&list, "dump3");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_dtor(&list));
    fclose(list_dumps);

    system("start chrome \"%CD%\\log.html\"");

    return 0;
}