/*****************************************************************************
 * 
 * @file machine.h
 * @date 17 Jule 2021
 * @author Mikhail Malyarenko <malyarenko.md@gmail.com>
 * 
 * @brief Determined State Machine program emulation
 * 
 *****************************************************************************/

#ifndef __MACHINE_H__
#define __MACHINE_H__

#include <stdint.h>

/* Define -------------------------------------------------------------------*/

/* Enum ---------------------------------------------------------------------*/

enum machine_status {
    MACHINE_STATUS_SUCCESS,
    MACHINE_STATUS_NULL_PARAM,
    MACHINE_STATUS_INVAL_PARSER,
};

/* Structures ---------------------------------------------------------------*/

/**
 * @struct
 */
struct machine_trans {
    struct machine_state* next_state;
    const char* output;
};

/**
 * @struct
 */
struct machine_state {
    const char* symbol;
    struct machine_trans* trans_table;
    bool is_final;
};

/**
 * @struct
 */
struct machine_instance {
    int input_list_size;
    int state_list_size;
    int output_list_size;

    const char** input_list;
    struct machine_state** state_list;
    const char** output_list;

    struct machine_state* entry_state;
};

/* Function Definitions -----------------------------------------------------*/

/**
 * 
 */
enum machine_status machine_init(struct machine_instance* machine, struct dsml_parser* parser);

/**
 * 
 */
enum machine_status machine_free(struct machine_instance* machine);

#endif /* __MACHINE_H__ */