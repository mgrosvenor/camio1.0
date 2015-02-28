/*
 * camio_blob.h
 *
 *  Created on: Nov 14, 2012
 *      Author: root
 */

#ifndef CAMIO_ISTREAM_BLOB_H_
#define CAMIO_ISTREAM_BLOB_H_

#include "camio_istream.h"

#define CAMIO_ISTREAM_BLOB_BLOCKING    1
#define CAMIO_ISTREAM_BLOB_NONBLOCKING 0

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    //No params yet
} camio_istream_blob_params_t;

typedef struct {
    camio_istream_t istream;
    int is_closed;                       //Has close be called?
    uint8_t* blob;                       //Pointer to the head of the blob
    size_t blob_size;                    //Size of the blob buffer
    size_t read_size;                    //Size of the current read waiting (if any)
    uint64_t offset;                     //Current offset into the blob
    camio_istream_blob_params_t* params;  //Parameters passed in from the outside

} camio_istream_blob_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_blob_new( const camio_descr_t* opts, camio_istream_blob_params_t* params);


#endif /* CAMIO_ISTREAM_BLOB_H_ */
