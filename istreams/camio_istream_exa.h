/*
 * Copyright  (C) Matthew P. Grosvenor, 2016, All Rights Reserved
 *
 * Camio ExaNIC card input stream
 *
 */


#ifndef CAMIO_ISTREAM_EXA_H_
#define CAMIO_ISTREAM_EXA_H_

#include "camio_istream.h"
#include <exanic/exanic.h>
#include <exanic/exanic.h>
#include <exanic/fifo_tx.h>
#include <exanic/fifo_rx.h>
#include <exanic/util.h>

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/

#define EXANIC_PORTS 4 //HACK! Assumes ExaNIC x4
#define DATA_BUFF 2048

typedef struct {
    //No params yet
} camio_istream_exa_params_t;

typedef struct {
    int is_closed;
    //int exa_stream;
    exanic_t* exanic;
    exanic_rx_t* exanic_rx[EXANIC_PORTS];
    int port;

    char exa_data[DATA_BUFF];
    size_t data_size;

    camio_istream_t istream;
    camio_istream_exa_params_t* params;  //Parameters passed in from the outside

} camio_istream_exa_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_exa_new( const camio_descr_t* opts,  camio_istream_exa_params_t* params);


#endif /* CAMIO_ISTREAM_EXA_H_ */
