/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Parse fe2+ style command line options.
 *
 * Options have the following format:
 *
 * protocol:[file],[opt1=value],[opt2=value]
 *
 */


#ifndef CAMIO_OPTS_H_
#define CAMIO_OPTS_H_

#include <stdint.h>

struct camio_opt_t {
    char* name;
    char* value;
    struct camio_opt_t* next;
};


typedef struct  {
    char* protocol;
    char* query;
    struct camio_opt_t* opt_head;
}camio_descr_t;

int camio_descr_parse(const char* description, camio_descr_t* out_descr);
void camio_descr_destroy(camio_descr_t* out_descr);
void camio_descr_construct(camio_descr_t* descr);


#endif /* CAMIO_OPTS_H_ */
