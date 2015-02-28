/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ poll socket input stream
 *
 */

#ifndef CAMIO_SELECTOR_POLL_H_
#define CAMIO_SELECTOR_POLL_H_

#include <sys/poll.h>

#include "camio_selector.h"
#include "../istreams/camio_istream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/

typedef struct {
    //No params at this stage
} camio_selector_poll_params_t;


typedef struct {
    camio_istream_t* istream;
    size_t index;
} camio_selector_poll_stream_t;

#define CAMIO_SELECTOR_POLL_MAX_STREAMS 32

typedef struct {
    camio_selector_t selector;                         //Underlying selector interface
    camio_selector_poll_params_t* params;              //Parameters passed in from the outside
    camio_selector_poll_stream_t streams[CAMIO_SELECTOR_POLL_MAX_STREAMS]; //Statically allow up to n streams on this (simple) selector
    size_t stream_count;                              //Number of streams added to the slector
    size_t stream_avail;                              //Number of streams that are non null in the selector
    size_t last;
    struct pollfd fds[CAMIO_SELECTOR_POLL_MAX_STREAMS];
} camio_selector_poll_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_selector_t* camio_selector_poll_new(  camio_selector_poll_params_t* params);


#endif /* CAMIO_SELECTOR_POLL_H_ */
