/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ seq socket input stream
 *
 */

#ifndef CAMIO_SELECTOR_SEQ_H_
#define CAMIO_SELECTOR_SEQ_H_

#include "camio_selector.h"
#include "../istreams/camio_istream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/

typedef struct {
    //No params at this stage
} camio_selector_seq_params_t;


typedef struct {
    camio_istream_t* istream;
    size_t index;
} camio_selector_seq_stream_t;

#define CAMIO_SELECTOR_SEQ_MAX_STREAMS 32

typedef struct {
    camio_selector_t selector;                         //Underlying selector interface
    camio_selector_seq_params_t* params;              //Parameters passed in from the outside
    camio_selector_seq_stream_t streams[CAMIO_SELECTOR_SEQ_MAX_STREAMS]; //Statically allow up to n streams on this (simple) selector
    size_t stream_count;                             //Number of streams added to the selector
    size_t stream_avail;                             //Number of streams still available in the selector
} camio_selector_seq_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_selector_t* camio_selector_seq_new(  camio_selector_seq_params_t* params);


#endif /* CAMIO_SELECTOR_SEQ_H_ */
