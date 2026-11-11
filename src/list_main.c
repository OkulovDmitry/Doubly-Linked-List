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

int main() {
    log_init();

    list_t list;
    LIST_CHECK(list_ctor(&list, 8));
    LIST_CHECK(list_verify(&list));

    list_iter_t a, b, c, d;
    LIST_CHECK(list_elem_push(&list, 10, 1, &a));   // 10
    LIST_CHECK(list_elem_push(&list, 20, 2, &b));   // 10 20
    LIST_CHECK(list_elem_push(&list, 15, 2, &c));   // 10 15 20
    LIST_CHECK(list_elem_push(&list, 5, 1, NULL));  // 5 10 15 20

    FILE* list_dumps = fopen("list_dumps.txt", "w");
    if (list_dumps == NULL) return 1;
    //list.prev[1] = -123;
    list_dump(&list, list_dumps);  // order: 5(4) 10(1) 15(3) 20(2)
    list_dump_graph_both(&list, "dump1");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_pop(&list, 2));  // 5 15 20  (убрали 10, ячейка 1 свободна)
    list_dump(&list, list_dumps);         // order: 5(4) 15(3) 20(2), free_addr = 1
    list_dump_graph_both(&list, "dump2");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_elem_push(&list, 99, 4, &d));  // 5 15 20 99, d должен быть 1 (переиспользовали ячейку)
    if (d.idx != 1) printf("expected reuse of cell 1, got %d\n", d.idx);
    if (list_elem_push(&list, 7, 0, NULL) != LIST_BAD_INDEX) printf("push(0) must fail\n");
    list_dump(&list, list_dumps);  // order: 5(4) 15(3) 20(2) 99(1)
    list_dump_graph_both(&list, "dump3");
    LIST_CHECK(list_verify(&list));

    LIST_CHECK(list_dtor(&list));
    fclose(list_dumps);

    system("start log.html");

    return 0;
}