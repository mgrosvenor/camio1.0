/*
 * camio_ostream_log.h
 *
 *  Created on: Nov 15, 2012
 *      Author: root
 */

#ifndef CAMIO_OSTREAM_LOG_H_
#define CAMIO_OSTREAM_LOG_H_

#include "camio_ostream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    int fd; //Allow creator to bypass file open stage and supply their own FD;
} camio_ostream_log_params_t;

typedef struct {
    camio_ostream_t ostream;
    int is_closed;                          //Has close be called?
    int escape;                             //Should binary be represented by escape sequences eg \x00)
    uint8_t* buffer;                        //Space to build output
    size_t buffer_size;                     //Size of output buffer
    uint8_t* assigned_buffer;               //Assigned write buffer
    size_t assigned_buffer_sz;              //Assigned write buffer size
    camio_ostream_log_params_t* params;      //Parameters from the outside world

} camio_ostream_log_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_ostream_t* camio_ostream_log_new( const camio_descr_t* opts,  camio_ostream_log_params_t* params);



#endif /* CAMIO_OSTREAM_LOG_H_ */
