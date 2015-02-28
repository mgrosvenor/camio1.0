/*
 * camio_log.h
 *
 *  Created on: Nov 14, 2012
 *      Author: root
 */

#ifndef CAMIO_ISTREAM_LOG_H_
#define CAMIO_ISTREAM_LOG_H_

#include "camio_istream.h"

#define CAMIO_ISTREAM_LOG_BLOCKING    1
#define CAMIO_ISTREAM_LOG_NONBLOCKING 0

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    int fd; //Allow creator to bypass file open stage and supply their own FD;
} camio_istream_log_params_t;

typedef struct {
    camio_istream_t istream;
    int escape;                         //Escape sequences(eg \x00)  be interpreted as binary
    int is_closed;                      //Has close be called?
    uint8_t* line_buffer;               //Space to build up lines
    size_t line_buffer_size;            //Size of the space
    size_t line_buffer_count;           //Amount of data in the buffer
    size_t read_size;                   //Size of a line that is ready for start_read
    uint8_t* data_head_ptr;             //Place to read data from in by calling start_read
    camio_istream_log_params_t* params;  //Parameters passed in from the outside

} camio_istream_log_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_log_new( const camio_descr_t* descr,  camio_istream_log_params_t* params);


#endif /* CAMIO_ISTREAM_LOG_H_ */
