/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ DAG card input stream
 *
 */


#ifndef CAMIO_ISTREAM_DAG_H_
#define CAMIO_ISTREAM_DAG_H_

#include "camio_istream.h"


/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/

typedef struct {
    //No params yet
} camio_istream_dag_params_t;

typedef struct {
    int is_closed;
    int dag_stream;
    void* dag_data;
    size_t data_size;
    camio_istream_t istream;
    camio_istream_dag_params_t* params;  //Parameters passed in from the outside

} camio_istream_dag_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_dag_new( const camio_descr_t* opts,  camio_istream_dag_params_t* params);


#endif /* CAMIO_ISTREAM_DAG_H_ */
