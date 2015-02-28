/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Parse Fe2+ style command line options.
 *
 * Quick and dirty parser for stream descriptions. They have the following format:
 *
 * protocol[:query][,opt_name1=value][,opt_name2=value][...]
 * - protocol name must not include the ":" symbol
 * - query must not include the "," symbol
 * - option name must not include the '=' symbol
 */


#include "camio_descr.h"
#include "camio_errors.h"


///eg "udp:127.0.0.1,opt1=1,opt2=4"
int camio_descr_parse(const char* descr_str, camio_descr_t* out_descr){
    if(!out_descr){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No output struct supplied\n");
    }

    struct camio_opt_t* opt = NULL;
    struct camio_opt_t* prev = NULL;
    int done = 0;

    //Get the protocol name
    const char* head_ptr = descr_str;
    const char* tail_ptr = descr_str;
    if(*head_ptr == '\0'){
        eprintf_exit(CAMIO_ERR_INCOMPLETE_OPT, "Unexpected end of description string \"%s\"\n", descr_str);
    }
    for(; *head_ptr != ':' &&  *head_ptr != ',' && *head_ptr != '\0'; head_ptr++){} //Find the ':' or ',' character

    char* protocol = calloc(1, head_ptr - tail_ptr + 1);
    if(!protocol){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not allocate memory for protocol name\n");
    }
    out_descr->protocol = protocol;
    memcpy(out_descr->protocol,tail_ptr,head_ptr - tail_ptr);

    if(*head_ptr == '\0' ){
        return CAMIO_ERR_NONE; //There is nothing left to parse
    }

    //Get the query string
    if(*head_ptr == ',' ){
        goto parse_options; //There is no query sting
    }


    //Parse the query sting
    head_ptr++; //skip over the ':' character
    tail_ptr = head_ptr; //Reset for the next iteration

    for(; *head_ptr != ',' && *head_ptr != '\0'; head_ptr++){} //Find the ',' character
    char* query = calloc(1, head_ptr - tail_ptr + 1);
    if(!query){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not allocate memory for query string\n");
    }
    out_descr->query = query;
    memcpy(out_descr->query,tail_ptr,head_ptr - tail_ptr);

parse_options:

    //Parse the options string
    while(1){
        if(*head_ptr == '\0'){
            break; //at the end of the options list
        }

        head_ptr++; //skip over the ',' character
        tail_ptr = head_ptr; //Reset for the next iteration

        //Get the opt name
        for(; *head_ptr != '=' && *head_ptr != '\0'; head_ptr++){} //Find the '=' character
        if(*head_ptr == '\0'){
            eprintf_exit(CAMIO_ERR_INCOMPLETE_OPT, "Unexpected end of option string \"%s\"\n", descr_str);
        }

        opt = calloc(1, sizeof(struct camio_opt_t));
        if(!opt){
            eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not allocate memory for option\n");
        }
        if(out_descr->opt_head == NULL){
            out_descr->opt_head = opt;
        }


        char* opt_name = calloc(1, head_ptr - tail_ptr + 1);
        if(!opt_name){
            eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not allocate memory for option name\n");
        }
        opt->name = opt_name;
        memcpy(opt->name,tail_ptr,head_ptr - tail_ptr);
        head_ptr++; //skip over the '=' character
        tail_ptr = head_ptr; //Reset for the next iteration

        //Get the opt value
        for(; *head_ptr != ',' && *head_ptr != '\0'; head_ptr++){} //Find the ',' character
        if(*head_ptr == '\0'){
            done = 1;
        }

        char* opt_value = calloc(1, head_ptr - tail_ptr + 1);
        if(!opt_value){
            eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not allocate memory for option name\n");
        }
        opt->value = opt_value;
        memcpy(opt->value,tail_ptr,head_ptr - tail_ptr);

        if(prev){
            prev->next = opt;
        }

        prev = opt;

        if(done)
            break;

    }
    if(opt)
        opt->next = NULL;

    return CAMIO_ERR_NONE;
}


void camio_descr_construct(camio_descr_t* descr){
    descr->protocol = NULL;
    descr->query    = NULL;
    descr->opt_head = NULL;
}

//Recursively free the linked list of options
void camio_descr_free(struct camio_opt_t* opt){
    if(!opt)
        return;

    if(opt->next){
        camio_descr_free(opt->next);
    }

    free(opt->name);
    free(opt->value);
    free(opt);
}


void camio_descr_destroy(camio_descr_t* opts){
    free(opts->protocol);
    if(opts->query) free(opts->query);
    camio_descr_free(opts->opt_head);
}
