#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dsml.h"
#include "machine.h"

enum machine_status machine_init(struct machine_instance* machine, struct dsml_parser* parser) {
    if ((machine == NULL) || (parser == NULL)) {
        return MACHINE_STATUS_NULL_PARAM;
    }

    if (!parser->has_estate ||
        (parser->input_list_size == 0) ||
        (parser->state_list_size == 0) ||
        (parser->trans_list_size != (parser->state_list_size * parser->input_list_size)))
    {
        return MACHINE_STATUS_INVAL_PARSER;
    }

    machine->input_list_size = parser->input_list_size;
    machine->state_list_size = parser->state_list_size;
    machine->output_list_size = parser->output_list_size;

    /* Allocate & initialise Machine Inputs */
    machine->input_list = (const char**) malloc(machine->input_list_size * sizeof(const char*));
    for (int i = 0; i < parser->input_list_size; i++) {
        machine->input_list[i] = strdup(parser->input_list[i]->symbol);
    }

    /* Allocate & initialise Machine States */
    machine->state_list = (struct machine_state**) malloc(machine->state_list_size * sizeof(struct machine_state*));
    for (int i = 0; i < machine->state_list_size; i++) {
        machine->state_list[i] = (struct machine_state*) malloc(sizeof(struct machine_state));
        machine->state_list[i]->symbol = strdup(parser->state_list[i]->symbol);
        machine->state_list[i]->trans_table = 
            (struct machine_trans*) malloc(machine->input_list_size * sizeof(struct machine_trans));
        machine->state_list[i]->is_final = parser->state_list[i]->is_final;

        /* Set Entry State for the Machine */
        if (parser->state_list[i]->is_entry) {
            machine->entry_state = machine->state_list[i];
        }
    }

    /* Allocate & initialise Machine Outputs */
    machine->output_list = (const char**) malloc(machine->output_list_size * sizeof(const char*));
    for (int i = 0; i < machine->output_list_size; i++) {
        machine->output_list[i] = strdup(parser->output_list[i]);
    }

    /* Connect Machine states via transitions and set symbols */
    struct dsml_trans* trans = NULL;
    for (int i = 0; i < machine->state_list_size; i++) {
        struct machine_state* current_state = machine->state_list[i];
        const char* state_symbol = current_state->symbol;
        
        for (int j = 0; j < machine->input_list; j++) {
            const char* input_symbol = machine->input_list[i];
            trans = dsml_get_trans(parser, state_symbol, input_symbol);

            const char* next_state_symbol = trans->to_state->symbol;
            
            for (int  k = 0; k < machine->state_list_size; k++) {
                if (strcmp(next_state_symbol, machine->state_list[k]->symbol) == 0) {
                    current_state->trans_table[j].next_state = machine->state_list[k];
                    break;
                }
            }

            const char* output_symbol = trans->output->symbol;
            if (output_symbol == NULL) {
                current_state->trans_table[j].output = NULL;
            }
            else {
                for (int  k = 0; k < machine->output_list_size; k++) {
                    if (strcmp(output_symbol, machine->output_list[k]) == 0) {
                        current_state->trans_table[j].output = output_symbol;
                        break;
                    }
                }
            }
        }
    }

    return MACHINE_STATUS_SUCCESS;
}

enum machine_status machine_free(struct machine_instance* machine) {
    if (machine == NULL) {
        return MACHINE_STATUS_NULL_PARAM;
    }

    for (int  i = 0; i < machine->input_list_size; i++) {
        free((char*) machine->input_list[i]);
    }
    free(machine->input_list);

    for (int i = 0; i < machine->state_list_size; i++) {
        free((char*) machine->state_list[i]->symbol);
        free(machine->state_list[i]->trans_table);
    }
    free(machine->state_list);

    for (int i = 0; i < machine->output_list_size; i++) {
        free((char*) machine->output_list[i]);
    }
    free(machine->output_list);

    return MACHINE_STATUS_SUCCESS;
}