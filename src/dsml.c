#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "dsml.h"
#include "util.h"

struct dsml_parser* dsml_parse_script(const char* filename) {
    if (filename == NULL) {
        fprintf(stderr, "DSML> ERROR: Script filename is NULL\n");
        return NULL;
    }

    FILE* fin = fopen(filename, "r");

    if (fin == NULL) {
        perror("DSML> ERROR: Failed to open script file");
        return NULL;
    }

    struct dsml_parser* parser = (struct dsml_parser*) malloc(sizeof(struct dsml_parser));
    enum dsml_status status = dsml_parser_init(parser);

    if (status != DSML_STATUS_SUCCESS) {
        fprintf(stderr, "DSML> ERROR: Failed to create parser: %s\n", dsml_status_message(status));
        return NULL;
    }

    char buffer[MAX_STRING_LEN + 1] = { 0 };
    unsigned int line_count = 0;

    while (fgets(buffer, MAX_STRING_LEN, fin) != NULL) {
        line_count++;
        char* newline_char_ptr = NULL;

        /* Zero-out newline character */
        if ((newline_char_ptr = strchr(buffer, '\n')) != NULL) {
            *newline_char_ptr = '\0';
        }

        /* Check if string is blank */
        if (is_blank(buffer) || (strlen(buffer) == 0)) {
            continue;
        }

        /* Check if string is comment */
        if (dsml_is_comment(buffer)) {
            continue;
        }

        char* next_string = strdup(buffer);
        char* keyword = strtok(next_string, DSML_SYMBOL_DELIM);

        if (strlen(buffer) == strlen(keyword)) {
            fprintf(stderr, "DSML> ERROR at line %u: Expected expression\n", line_count);
            free(next_string);
            goto PARSER_ERROR;
        }

        char* lexeme = next_string + strlen(keyword) + 1;
        enum dsml_lexeme_type lexeme_type = dsml_parse_lexeme_keyword(keyword);

        switch (lexeme_type) {
        case DSML_LEXEME_STATE:
            status = dsml_parse_state(parser, lexeme);
            break;

        case DSML_LEXEME_INPUT:
            status = dsml_parse_io(parser, lexeme, true);
            break;  

        case DSML_LEXEME_OUTPUT:
            status = dsml_parse_io(parser, lexeme, false);
            break;

        case DSML_LEXEME_TRANS:
            status = dsml_parse_trans(parser, lexeme);
            break;

        default:
            fprintf(stderr, "DSML> ERROR at line %u: Unknown keyword\n", line_count);
            free(next_string);
            goto PARSER_ERROR;
            break;
        }

        if (status != DSML_STATUS_SUCCESS) {
            fprintf(stderr, "DSML> ERROR at line %u: %s\n", line_count, dsml_status_message(status));
            free(next_string);
            goto PARSER_ERROR;
        }

        free(next_string);
    }

    if (feof(fin)) {
        status = dsml_validate_dsm(parser);

        if (status == DSML_STATUS_SUCCESS) {
            fprintf(stdout, "DSML> Script is parsed successfully\n");
            fclose(fin);
            return parser;
        }
        else {
            fprintf(stderr, "DSML> ERROR: %s\n", dsml_status_message(status));
        }
    }
    else if (ferror(fin)) {
        perror("DSML> ERROR: Failed to read from the script file");
    }
    else {
        fprintf(stderr, "DSML> ERROR: Failed to read from the script file: Unknown error\n");
    }

PARSER_ERROR:

    fclose(fin);
    dsml_parser_free(parser);
    free(parser);
    return NULL;
}

enum dsml_status dsml_parser_init(struct dsml_parser* parser) {
    if (parser == NULL) {
        return DSML_STATUS_NULL_PARAM;
    }

    parser->state_list_cap = INIT_CAP;
    parser->state_list_size = 0;

    parser->input_list_cap = INIT_CAP;
    parser->input_list_size = 0;

    parser->output_list_cap = INIT_CAP;
    parser->output_list_size = 0;

    parser->trans_list_cap = INIT_CAP;
    parser->trans_list_size = 0;

    parser->has_estate = false;

    parser->state_list = (struct dsml_state**) malloc(INIT_CAP * sizeof(struct dsml_state*));
    parser->input_list = (struct dsml_io**) malloc(INIT_CAP * sizeof(struct dsml_io*));
    parser->output_list = (struct dsml_io**) malloc(INIT_CAP * sizeof(struct dsml_io*));
    parser->trans_list = (struct dsml_trans**) malloc(INIT_CAP * sizeof(struct dsml_trans*));

    return DSML_STATUS_SUCCESS;
}

enum dsml_status dsml_parser_free(struct dsml_parser* parser) {
    if (parser == NULL) {
        return DSML_STATUS_NULL_PARAM;
    }

    for (size_t i = 0; i < parser->state_list_size; i++) {
        free((char*) parser->state_list[i]->symbol);
        free(parser->state_list[i]);
    }

    for (size_t i = 0; i < parser->input_list_size; i++) {
        free((char*) parser->input_list[i]->symbol);
        free(parser->input_list[i]);
    }

    for (size_t i = 0; i < parser->output_list_size; i++) {
        free((char*) parser->output_list[i]->symbol);
        free(parser->output_list[i]);
    }

    for (size_t i = 0; i < parser->trans_list_size; i++) {
        free(parser->trans_list[i]);
    }

    free(parser->state_list);
    free(parser->input_list);
    free(parser->output_list);
    free(parser->trans_list);

    parser->state_list = NULL;
    parser->input_list = NULL;
    parser->output_list = NULL;
    parser->trans_list = NULL;

    parser->state_list_size = 0;
    parser->input_list_size = 0;
    parser->output_list_size = 0;
    parser->trans_list_size = 0;

    parser->state_list_cap = 0;
    parser->input_list_cap = 0;
    parser->output_list_cap = 0;
    parser->trans_list_cap = 0;

    return DSML_STATUS_SUCCESS;
}

enum dsml_lexeme_type dsml_parse_lexeme_keyword(const char* str) {
    if (str == NULL) {
        return DSML_LEXEME_UNDEF;
    }

    if (strcmp(str, DSML_KEYWORDS[DSML_STATE_KEYWORD_INDEX]) == 0) {
        return DSML_LEXEME_STATE;
    }
    else if (strcmp(str, DSML_KEYWORDS[DSML_INPUT_KEYWORD_INDEX]) == 0) {
        return DSML_LEXEME_INPUT;
    }
    else if (strcmp(str, DSML_KEYWORDS[DSML_OUTPUT_KEYWORD_INDEX]) == 0) {
        return DSML_LEXEME_OUTPUT;
    }
    else if (strcmp(str, DSML_KEYWORDS[DSML_TRANS_KEYWORD_INDEX]) == 0) {
        return DSML_LEXEME_TRANS;
    }
    else {
        return DSML_LEXEME_UNDEF;
    }
}

enum dsml_status dsml_parse_state(struct dsml_parser* parser, const char* str) {
    if ((parser == NULL) || (str == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (strlen(str) == 0) {
        return DSML_STATUS_EMPTY_SYMBOL;
    }

    char* str_mutable = strdup(str);

    enum dsml_status status = 0;
    bool is_final = false;
    bool is_entry = false;

    char* next_symbol = strtok(str_mutable, DSML_SYMBOL_DELIM);

    if (next_symbol == NULL) {
        status = DSML_STATUS_EMPTY_SYMBOL;
        goto EXIT;
    }

    /* Parse 'state' keyword modificators */
    for (int i = 0; i < 2; i++) {
        if (strcmp(next_symbol, DSML_KEYWORDS[DSML_FINAL_KEYWORD_INDEX]) == 0) {
            if (!is_final) {
                is_final = true;
                next_symbol = strtok(NULL, DSML_SYMBOL_DELIM);
            }
            else {
                status = DSML_STATUS_REDEF_KEYWORD;
                goto EXIT;
            }
        }
        else if (strcmp(next_symbol, DSML_KEYWORDS[DSML_ENTRY_KEYWORD_INDEX]) == 0) {
            if (!is_entry) {
                if (parser->has_estate) {
                    status = DSML_STATUS_MULT_ENTRY;
                    goto EXIT;
                }
                else {
                    is_entry = true;
                    parser->has_estate = true;
                    next_symbol = strtok(NULL, DSML_SYMBOL_DELIM);
                }
            }
            else {
                status = DSML_STATUS_REDEF_KEYWORD;
                goto EXIT;
            }
        }
        else {
            break;
        }

        if (next_symbol == NULL) {
            break;
        }
    }

    size_t symbol_count = 0;

    /* Parse state symbols */
    while (next_symbol != NULL) {
        symbol_count++;

        if (is_entry && (symbol_count > 1)) {
            status = DSML_STATUS_MULT_ENTRY;
            goto EXIT;
        }

        if (!dsml_validate_symbol(next_symbol)) {
            status = DSML_STATUS_INVAL_SYMBOL;
            goto EXIT;
        }

        if (dsml_symbol_exists(parser, next_symbol, DSML_LEXEME_STATE)) {
            status = DSML_STATUS_REDEF_SYMBOL;
            goto EXIT;
        }

        dsml_add_state(parser, next_symbol, is_final, is_entry);
        next_symbol = strtok(NULL, DSML_SYMBOL_DELIM);
    }

    if (symbol_count == 0) {
        status = DSML_STATUS_EMPTY_SYMBOL;
    }

EXIT:

    free(str_mutable);
    return status;
}

enum dsml_status dsml_parse_io(struct dsml_parser* parser, const char* str, bool is_input) {
    if ((parser == NULL) || (str == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (strlen(str) == 0) {
        return DSML_STATUS_EMPTY_SYMBOL;
    }

    char* str_mutable = strdup(str);

    enum dsml_status status = 0;
    size_t symbol_count = 0;
    enum  dsml_lexeme_type lexeme_type = is_input ? DSML_LEXEME_INPUT : DSML_LEXEME_OUTPUT;

    char* next_symbol = strtok(str_mutable, DSML_SYMBOL_DELIM);

    if (next_symbol == NULL) {
        status = DSML_STATUS_EMPTY_SYMBOL;
        goto EXIT;
    }

    /* Parse io symbols */
    while (next_symbol != NULL) {
        symbol_count++;

        if (!dsml_validate_symbol(next_symbol)) {
            status = DSML_STATUS_INVAL_SYMBOL;
            goto EXIT;
        }

        if (dsml_symbol_exists(parser, next_symbol, lexeme_type)) {
            status = DSML_STATUS_REDEF_SYMBOL;
            goto EXIT;
        }

        if (is_input) {
            dsml_add_input(parser, next_symbol);
        }
        else {
            dsml_add_output(parser, next_symbol);
        }

        next_symbol = strtok(NULL, DSML_SYMBOL_DELIM);
    }

    if (symbol_count == 0) {
        status = DSML_STATUS_EMPTY_SYMBOL;
    }

EXIT:

    free(str_mutable);
    return status;
}

enum dsml_status dsml_parse_trans(struct dsml_parser* parser, const char* str) {
    if ((parser == NULL) || (str == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (strlen(str) == 0) {
        return DSML_STATUS_INVAL_PARAM;
    }

    char* str_mutable = strdup(str);
    char* str_mutable_tail_ptr = str_mutable + strlen(str_mutable) + 1;

    enum dsml_status status = 0;
    char buffer[MAX_STRING_LEN + 1] = { 0 };
    const size_t buffer_size = MAX_STRING_LEN + 1;

    char* next_symbol = strtok(str_mutable, DSML_TRANS_DELIM);

    /* From State symbol */
    struct dsml_state* from_state = NULL;

    if (next_symbol != NULL) {
        status = dsml_trim_symbol(next_symbol, buffer, buffer_size);

        if (status != DSML_STATUS_SUCCESS) {
            goto EXIT;
        }

        from_state = dsml_get_entity(parser, buffer, DSML_LEXEME_STATE);

        if (from_state == NULL) {
            status = DSML_STATUS_UNDEF_SYMBOL;
            goto EXIT;
        }
    }
    else {
        status = DSML_STATUS_INVAL_PARAM_NUM;
        goto EXIT;
    }

    next_symbol = strtok(NULL, DSML_TRANS_DELIM);

    /* Input symbols */
    struct dsml_io** inputs = (struct dsml_io**) malloc(parser->input_list_size * sizeof(struct dsml_io*));
    size_t input_count = 0;
    char* backup_ptr = NULL;
    
    if (next_symbol != NULL) {
        backup_ptr = next_symbol + strlen(next_symbol) + 1;
        if (backup_ptr == str_mutable_tail_ptr) {
            status = DSML_STATUS_INVAL_PARAM_NUM;
            goto EXIT;
        }

        char* input_symbol_list = strdup(next_symbol);
        char* input_symbol = strtok(input_symbol_list, DSML_SYMBOL_DELIM);
        struct dsml_io* input = NULL;

        while (input_symbol != NULL) {
            if ((input = dsml_get_entity(parser, input_symbol, DSML_LEXEME_INPUT)) == NULL) {
                status = DSML_STATUS_UNDEF_SYMBOL;
                free(input_symbol_list);
                goto EXIT;
            }
            else {
                /* Check if input symbol was already used */
                for (size_t i = 0; i < input_count; i++) {
                    if (strcmp(input_symbol, inputs[i]->symbol) == 0) {
                        status = DSML_STATUS_REDEF_SYMBOL;
                        free(input_symbol_list);
                        goto EXIT;
                    }
                }

                /* Check if transition with this From State and Input was already defined */
                if (dsml_get_trans(parser, from_state->symbol, input_symbol) != NULL) {
                    status = DSML_STATUS_INDETERM_TRANS;
                    free(input_symbol_list);
                    goto EXIT;
                }

                inputs[input_count++] = input;
                input_symbol = strtok(NULL, DSML_SYMBOL_DELIM);
            }
        }

        free(input_symbol_list);

        if (input_count == 0) {
            status = DSML_STATUS_EMPTY_SYMBOL;
            goto EXIT;
        }
    }
    else {
        status = DSML_STATUS_INVAL_PARAM_NUM;
        goto EXIT;
    }

    next_symbol = strtok(backup_ptr, DSML_TRANS_DELIM);

    /* To State */
    struct dsml_state* to_state = NULL;

    if (next_symbol != NULL) {
        status = dsml_trim_symbol(next_symbol, buffer, buffer_size);

        if (status != DSML_STATUS_SUCCESS) {
            goto EXIT;
        }

        to_state = dsml_get_entity(parser, buffer, DSML_LEXEME_STATE);

        if (to_state == NULL) {
            status = DSML_STATUS_UNDEF_SYMBOL;
            goto EXIT;
        }
    }
    else {
        status = DSML_STATUS_INVAL_PARAM_NUM;
        goto EXIT;
    }

    next_symbol = strtok(NULL, DSML_TRANS_DELIM);

    /* Output */
    struct dsml_io* output = NULL;

    if (next_symbol != NULL) {
        status = dsml_trim_symbol(next_symbol, buffer, buffer_size);

        if (status != DSML_STATUS_SUCCESS) {
            goto EXIT;
        }

        /* Check if Output is not an Empty Output */
        if (strcmp(buffer, DSML_EMPTY_OUTPUT_SYMBOL) != 0) {
            output = dsml_get_entity(parser, buffer, DSML_LEXEME_OUTPUT);

            if (output == NULL) {
                status = DSML_STATUS_UNDEF_SYMBOL;
                goto EXIT;
            }
        }
    }
    else {
        status = DSML_STATUS_INVAL_PARAM_NUM;
        goto EXIT;
    }
    
    /* Create new Transition(s) */
    for (size_t i = 0; i < input_count; i++) {
        struct dsml_trans* new_trans = (struct dsml_trans*) malloc(sizeof(struct dsml_trans));

        new_trans->from_state = from_state;
        new_trans->input = inputs[i];
        new_trans->to_state = to_state;
        new_trans->output = output;

        status = dsml_add_trans(parser, new_trans);
        if (status != DSML_STATUS_SUCCESS) {
            break;
        }
    }

EXIT:

    free(str_mutable);
    free(inputs);
    return status;
}

enum dsml_status dsml_add_state(struct dsml_parser* parser, const char* symbol, bool is_final, bool is_entry) {
    if ((parser == NULL) || (symbol == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (parser->state_list_size == parser->state_list_cap) {
        parser->state_list_cap += CAP_INCR;
        parser->state_list = 
            (struct dsml_state**) realloc(parser->state_list, parser->state_list_cap * sizeof(struct dsml_state*));
    }

    struct dsml_state* new_state = (struct dsml_state*) malloc(sizeof(struct dsml_state));
    new_state->symbol = strdup(symbol);
    new_state->is_final = is_final;
    new_state->is_entry = is_entry;
    parser->state_list[parser->state_list_size] = new_state;
    parser->state_list_size++;
    return DSML_STATUS_SUCCESS;
}

enum dsml_status dsml_add_input(struct dsml_parser* parser, const char* symbol) {
    if ((parser == NULL) || (symbol == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (parser->input_list_size == parser->input_list_cap) {
        parser->input_list_cap += CAP_INCR;
        parser->input_list = 
            (struct dsml_io**) realloc(parser->input_list, parser->input_list_cap * sizeof(struct dsml_io*));
    }

    parser->input_list[parser->input_list_size] = (struct dsml_io*) malloc(sizeof(struct dsml_io));
    parser->input_list[parser->input_list_size]->symbol = strdup(symbol);
    parser->input_list_size++;
    return DSML_STATUS_SUCCESS;
}

enum dsml_status dsml_add_output(struct dsml_parser* parser, const char* symbol) {
    if ((parser == NULL) || (symbol == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (parser->output_list_size == parser->output_list_cap) {
        parser->output_list_cap += CAP_INCR;
        parser->output_list = 
            (struct dsml_io**) realloc(parser->output_list, parser->output_list_cap * sizeof(struct dsml_io*));
    }

    parser->output_list[parser->output_list_size] = (struct dsml_io*) malloc(sizeof(struct dsml_io));
    parser->output_list[parser->output_list_size]->symbol = strdup(symbol);
    parser->output_list_size++;
    return DSML_STATUS_SUCCESS;
}

enum dsml_status dsml_add_trans(struct dsml_parser* parser, struct dsml_trans* trans) {
    if ((parser == NULL) || (trans == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (parser->trans_list_size == parser->trans_list_cap) {
        parser->trans_list_cap += CAP_INCR;
        parser->trans_list = 
            (struct dsml_trans**) realloc(parser->trans_list, parser->trans_list_cap * sizeof(struct dsml_trans*));
    }

    parser->trans_list[parser->trans_list_size++] = trans;
    return DSML_STATUS_SUCCESS;
}

bool dsml_validate_symbol(const char* symbol) {
    if (symbol == NULL) {
        return false;
    }

    size_t symbol_len = strlen(symbol);

    /* Check if symbol is alphanumeric */
    for (size_t i = 0; i < symbol_len; i++) {
        if (!isalnum(symbol[i])) {
            return false;
        }
    }

    /* Check if symbol is not a DSML keyword */
    for (size_t i = 0; i < DSML_KEYWORDS_NUM; i++) {
        if (strcmp(symbol, DSML_KEYWORDS[i]) == 0) {
            return false;
        }
    }
    
    return true;
}

enum dsml_status dsml_trim_symbol(const char* symbol, char* buffer, size_t buffer_size) {
    if ((symbol == NULL) || (buffer == NULL)) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (strlen(symbol) == 0) {
        return DSML_STATUS_EMPTY_SYMBOL;
    }

    if (buffer_size == 0) {
        return DSML_STATUS_INVAL_PARAM;
    }

    size_t symbol_len = strlen(symbol);
    size_t c_count = 0;

    if (symbol_len >= buffer_size) {
        return DSML_STATUS_INVAL_PARAM;
    }

    const char* c_ptr_front = symbol;
    while ((*c_ptr_front != '\0') && isspace(*c_ptr_front)) {
        c_ptr_front++;
    }

    if (*c_ptr_front == '\0') {
        goto EXIT;
    }

    const char* c_ptr_back = symbol + strlen(symbol);
    while (((c_ptr_back - 1) != c_ptr_front) && isspace(*(c_ptr_back - 1))) {
        c_ptr_back--;
    }

    do {
        buffer[c_count++] = *c_ptr_front;
        c_ptr_front++;
    } while (c_ptr_front != c_ptr_back);

EXIT:

    buffer[c_count] = '\0';
    return DSML_STATUS_SUCCESS;
}

bool dsml_symbol_exists(struct dsml_parser* parser, const char* symbol, enum dsml_lexeme_type type) {
    assert((parser != NULL) && (symbol != NULL));
    assert((type != DSML_LEXEME_TRANS) && (type != DSML_LEXEME_UNDEF));

    bool exists = false;

    switch (type) {
    case DSML_LEXEME_STATE:
        for (size_t i = 0; i < parser->state_list_size; i++) {
            if (strcmp(symbol, parser->state_list[i]->symbol) == 0) {
                exists = true;
                break;
            }
        }
        break;

    case DSML_LEXEME_INPUT:
        for (size_t i = 0; i < parser->input_list_size; i++) {
            if (strcmp(symbol, parser->input_list[i]->symbol) == 0) {
                exists = true;
                break;
            }
        }
        break;

    case DSML_LEXEME_OUTPUT:
        for (size_t i = 0; i < parser->output_list_size; i++) {
            if (strcmp(symbol, parser->output_list[i]->symbol) == 0) {
                exists = true;
                break;
            }
        }
        break;
    
    default:
        break;
    }

    return exists;
}

void* dsml_get_entity(struct dsml_parser* parser, const char* symbol, enum dsml_lexeme_type type) {
    assert((parser != NULL) && (symbol != NULL));
    assert((type != DSML_LEXEME_TRANS) && (type != DSML_LEXEME_UNDEF));

    void* entity_ptr = NULL;

    switch (type) {
    case DSML_LEXEME_STATE:
        for (size_t i = 0; i < parser->state_list_size; i++) {
            if (strcmp(symbol, parser->state_list[i]->symbol) == 0) {
                entity_ptr = (void*) parser->state_list[i];
                break;
            }
        }
        break;

    case DSML_LEXEME_INPUT:
        for (size_t i = 0; i < parser->input_list_size; i++) {
            if (strcmp(symbol, parser->input_list[i]->symbol) == 0) {
                entity_ptr = (void*) parser->input_list[i];
                break;
            }
        }
        break;

    case DSML_LEXEME_OUTPUT:
        for (size_t i = 0; i < parser->output_list_size; i++) {
            if (strcmp(symbol, parser->output_list[i]->symbol) == 0) {
                entity_ptr = (void*) parser->output_list[i];
                break;
            }
        }
        break;
    
    default:
        break;
    }

    return entity_ptr;
}

struct dsml_trans* dsml_get_trans(struct dsml_parser* parser, const char* from_state_symbol, const char* input_symbol) {
    assert((parser != NULL) && (from_state_symbol != NULL) && (input_symbol != NULL));

    struct dsml_trans* trans_ptr = NULL;

    for (size_t i = 0; i < parser->trans_list_size; i++) {
        if ((strcmp(parser->trans_list[i]->from_state->symbol, from_state_symbol) == 0) &&
            (strcmp(parser->trans_list[i]->input->symbol, input_symbol) == 0))
        {
            trans_ptr = parser->trans_list[i];
            break;
        }
    }

    return trans_ptr;
}

enum dsml_status dsml_validate_dsm(struct dsml_parser* parser) {
    if (parser == NULL) {
        return DSML_STATUS_NULL_PARAM;
    }

    if (parser->state_list_size == 0) {
        return DSML_STATUS_EMPTY_DSM;
    }

    if (!parser->has_estate) {
        return DSML_STATUS_NO_ENTRY;
    }

    if (parser->input_list_size == 0) {
        return DSML_STATUS_STATIC_DSM;
    }

    bool is_dsm_determined = true;
    const char* state_symbol = NULL;

    for (size_t i = 0; i < parser->state_list_size; i++) {
        state_symbol = parser->state_list[i]->symbol;

        for (size_t j = 0; j < parser->input_list_size; j++) {
            const char* input_symbol = parser->input_list[j]->symbol;

            if (dsml_get_trans(parser, state_symbol, input_symbol) == NULL) {
                is_dsm_determined = false;
                goto EXIT_CYCLE;
            }
        }
    }

EXIT_CYCLE:

    if (!is_dsm_determined) {
        fprintf(stderr, "Not all input reactions for the state '%s' are defined\n", state_symbol);
        return DSML_STATUS_INDETERM_TRANS;
    }

    return DSML_STATUS_SUCCESS;
}

bool dsml_is_comment(const char* str) {
    if (str == NULL) {
        return false;
    }

    if (strlen(str) == 0) {
        return false;
    }

    char buffer[MAX_STRING_LEN + 1] = { 0 };
    dsml_trim_symbol(str, buffer, MAX_STRING_LEN);

    return buffer[0] == '#';
}

const char* dsml_status_message(enum dsml_status status) {
    const char* message = NULL;

    switch (status) {
    case DSML_STATUS_SUCCESS:
        message = "Success";
        break;
    case DSML_STATUS_NULL_PARAM:
        message = "Runtime error: Passed parameter is NULL pointer";
        break;
    case DSML_STATUS_INVAL_PARAM:
        message = "Runtime error: Passed parameter is invalid";
        break;
    case DSML_STATUS_EMPTY_SYMBOL:
        message = "Syntax error: Entity symbol is empty or absent";
        break;
    case DSML_STATUS_INVAL_SYMBOL:
        message = "Syntax error: Entity symbol is invalid. Please, use only alphanumerical characters";
        break;
    case DSML_STATUS_UNDEF_SYMBOL:
        message = "Syntax error: Undefined entity symbol is referenced";
        break;
    case DSML_STATUS_REDEF_SYMBOL:
        message = "Syntax error: Entity symbol is redefined";
        break;
    case DSML_STATUS_UNDEF_KEYWORD:
        message = "Syntax error: Undefined keyword";
        break;
    case DSML_STATUS_REDEF_KEYWORD:
        message = "Syntax error: Incorrect usage of keyword";
        break;
    case DSML_STATUS_INVAL_PARAM_NUM:
        message = "Syntax error: Incorrect number of parameters in the statement";
        break;
    case DSML_STATUS_INVAL_SYMBOL_NUM:
        message = "Syntax error: Incorrect number of entity symbols in the statement";
        break;
    case DSML_STATUS_NO_ENTRY:
        message = "DSM error: Entry state is not declared";
        break;
    case DSML_STATUS_MULT_ENTRY:
        message = "DSM error: Multiple entry states";
        break;
    case DSML_STATUS_EMPTY_DSM:
        message = "DSM error: DSM is empty (no states declared)";
        break;
    case DSML_STATUS_STATIC_DSM:
        message = "DSM error: DSM is static (no inputs declared)";
        break;
    case DSML_STATUS_INDETERM_TRANS:
        message = "DSM error: Indetermined transition";
        break;
    case DSML_STATUS_UNDEF_ERROR:
        message = "Unknown error";
        break;
    default:
        message = "No information";
        break;
    }

    return message;
}

#ifndef NDEBUG

void dsml_parser_print(struct dsml_parser* parser) {
    if (parser == NULL) {
        return;
    }

    printf("\nParser Symbols:\n");

    for (uint32_t i = 0; i < (uint32_t) parser->state_list_size; i++) {
        printf("\tState %u: %s ", i + 1, parser->state_list[i]->symbol);

        if (parser->state_list[i]->is_entry) {
            printf("entry ");
        }

        if (parser->state_list[i]->is_final) {
            printf("final");
        }

        printf("\n");
    }

    printf("\n");

    for (uint32_t i = 0; i < (uint32_t) parser->input_list_size; i++) {
        printf("\tInput %u: %s\n", i + 1, parser->input_list[i]->symbol);
    }

    printf("\n");

    for (uint32_t i = 0; i < (uint32_t) parser->output_list_size; i++) {
        printf("\tOutput %u: %s\n", i + 1, parser->output_list[i]->symbol);
    }

    printf("\n");

    for (uint32_t i = 0; i < (uint32_t) parser->trans_list_size; i++) {
        printf("\tTransition %u:\n", i + 1);
        printf("\t\tFrom State: %s\n", parser->trans_list[i]->from_state->symbol);
        printf("\t\tInput: %s\n", parser->trans_list[i]->input->symbol);
        printf("\t\tTo State: %s\n", parser->trans_list[i]->to_state->symbol);
        if (parser->trans_list[i]->output == NULL) {
             printf("\t\tOutput: -\n");
        }
        else {
            printf("\t\tOutput: %s\n", parser->trans_list[i]->output->symbol);
        }
    }

    printf("\n");
}

#endif /* !NDEBUG */