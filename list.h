#ifndef LIST_H
#define LIST_H

#define MAX_DATA_SIZE 200

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
} list_err_t;

#endif //LIST_H