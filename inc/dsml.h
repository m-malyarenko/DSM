/*****************************************************************************
 * 
 * @file dsml.h
 * @date 30 June 2021
 * @author Mikhail Malyarenko <malyarenko.md@gmail.com>
 * 
 * @brief Determined State Machine Language (DSML) parser header file
 * 
 *****************************************************************************/


#ifndef __DSML_H__
#define __DSML_H__

#include <stdint.h>
#include <stdbool.h>

/* Constants ----------------------------------------------------------------*/

/**
 * @var DSML reserved words.
 * You can not use them as an DSML script entity symbol. 
 */
static const char* DSML_KEYWORDS[] = {
    "state",
    "final",
    "entry",
    "input",
    "output",
    "trans"
};

/**
 * @var
 */
static const char* DSML_SYMBOL_DELIM = " ";

/**
 * @var
 */
static const char* DSML_TRANS_DELIM = ":";

/**
 * @var
 */
static const char* DSML_EMPTY_OUTPUT_SYMBOL = "-";

/* Define -------------------------------------------------------------------*/

/**
 * @def
 */
#define DSML_KEYWORDS_NUM ((size_t) 6)

/**
 * @def
 */
#define INIT_CAP            ((size_t) 5)
#define CAP_INCR            ((size_t) 5)
#define MAX_STRING_LEN      ((size_t) 255)
#define TRANS_OPERANDS_NUM  ((size_t) 4)

/**
 * @def
 */
#define TRANS_FROM_STATE    ((size_t) 0)
#define TRANS_INPUT         ((size_t) 1)
#define TRANS_TO_STATE      ((size_t) 2)
#define TRANS_OUTPUT        ((size_t) 3)

/* Enum ---------------------------------------------------------------------*/

/**
 * @enum
 */
enum dsml_keyword_index {
    DSML_STATE_KEYWORD_INDEX,
    DSML_FINAL_KEYWORD_INDEX,
    DSML_ENTRY_KEYWORD_INDEX,
    DSML_INPUT_KEYWORD_INDEX,
    DSML_OUTPUT_KEYWORD_INDEX,
    DSML_TRANS_KEYWORD_INDEX,
};

/**
 * @enum
 */
enum dsml_lexeme_type {
    DSML_LEXEME_STATE,
    DSML_LEXEME_INPUT,
    DSML_LEXEME_OUTPUT,
    DSML_LEXEME_TRANS,
    DSML_LEXEME_UNDEF,
};

/**
 * @enum
 */
enum dsml_status {
    DSML_STATUS_SUCCESS,
    DSML_STATUS_NULL_PARAM,
    DSML_STATUS_INVAL_PARAM,
    DSML_STATUS_EMPTY_SYMBOL,
    DSML_STATUS_INVAL_SYMBOL,
    DSML_STATUS_UNDEF_SYMBOL,
    DSML_STATUS_REDEF_SYMBOL,
    DSML_STATUS_UNDEF_KEYWORD,
    DSML_STATUS_REDEF_KEYWORD,
    DSML_STATUS_INVAL_PARAM_NUM,
    DSML_STATUS_INVAL_SYMBOL_NUM,
    DSML_STATUS_NO_ENTRY,
    DSML_STATUS_MULT_ENTRY,
    DSML_STATUS_EMPTY_DSM,
    DSML_STATUS_STATIC_DSM,
    DSML_STATUS_INDETERM_TRANS,
    DSML_STATUS_UNDEF_ERROR,
};

/* Structures ---------------------------------------------------------------*/

/**
 * @enum 
 */
struct dsml_parser {
    size_t state_list_size;
    size_t state_list_cap;
    
    size_t input_list_size;
    size_t input_list_cap;

    size_t output_list_size;
    size_t output_list_cap;

    size_t trans_list_size;
    size_t trans_list_cap;

    bool has_estate;

    struct dsml_state** state_list;
    struct dsml_io** input_list;
    struct dsml_io** output_list;
    struct dsml_trans** trans_list;    
};

/**
 * @struct
 */
struct dsml_state {
    const char* symbol;
    bool is_entry;
    bool is_final;
};

/**
 * @struct
 */
struct dsml_io {
    const char* symbol;
};

/**
 * @struct
 */
struct dsml_trans {  
    struct dsml_state* from_state;
    struct dsml_state* to_state;
    struct dsml_io* input;
    struct dsml_io* output;
};

/* Function Definitions -----------------------------------------------------*/

/* Interface Functions */

/**
 * 
 */
struct dsml_parser* dsml_parse_script(const char* filename);

/* Initialisation/Destruction of Structures */

/**
 * 
 */
enum dsml_status dsml_parser_init(struct dsml_parser* parser);

/**
 * 
 */
enum dsml_status dsml_parser_free(struct dsml_parser* parser);

/* Source Parsing */

/**
 * 
 */
enum dsml_lexeme_type dsml_parse_lexeme_keyword(const char* str);

/**
 * 
 */
enum dsml_status dsml_parse_state(struct dsml_parser* parser, const char* str);

/**
 * 
 */
enum dsml_status dsml_parse_io(struct dsml_parser* parser, const char* str, bool is_input);

/**
 * 
 */
enum dsml_status dsml_parse_trans(struct dsml_parser* parser, const char* str);

/* State/IO/Transition Adding */

/**
 * 
 */
enum dsml_status dsml_add_state(struct dsml_parser* parser, const char* symbol, bool is_final, bool is_entry);

/**
 * 
 */
enum dsml_status dsml_add_input(struct dsml_parser* parser, const char* symbol);

/**
 * 
 */
enum dsml_status dsml_add_output(struct dsml_parser* parser, const char* symbol);

/**
 * 
 */
enum dsml_status dsml_add_trans(struct dsml_parser* parser, struct dsml_trans* trans);

/**
 * 
 */
bool dsml_symbol_exists(struct dsml_parser* parser, const char* symbol, enum dsml_lexeme_type type);

/**
 * 
 */
void* dsml_get_entity(struct dsml_parser* parser, const char* symbol, enum dsml_lexeme_type type);

/**
 * 
 */
struct dsml_trans* dsml_get_trans(struct dsml_parser* parser, const char* from_state_symbol, const char* input_symbol);

/* Support Functions */

/**
 *
 */
bool dsml_validate_symbol(const char* symbol);

/**
 *
 */
enum dsml_status dsml_trim_symbol(const char* symbol, char* buffer, size_t buffer_size);

/**
 * 
 */
enum dsml_status dsml_validate_dsm(struct dsml_parser* parser);

/**
 * 
 */
bool dsml_is_comment(const char* str);

/* Error Handling */

const char* dsml_status_message(enum dsml_status status);

/* Debug Functions */

#ifndef NDEBUG

/**
 * 
 */
void dsml_parser_print(struct dsml_parser* parser);

#endif

#endif /* __DSML_H__ */