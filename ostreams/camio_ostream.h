/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Input stream definition
 *
 */

#ifndef OSTREAM_H_
#define OSTREAM_H_

#include <unistd.h>

#include "../camio_descr.h"
#include "../selectors/camio_selector.h"

struct camio_ostream;
typedef struct camio_ostream camio_ostream_t;

struct camio_ostream{
     int(*open)(camio_ostream_t* this, const camio_descr_t* dscr );                //Open the stream and prepare for writing, return 0 if it succeeds
     void(*close)(camio_ostream_t* this);                                        //Close the stream
     int (*ready)(camio_ostream_t* this);                                        //Returns non-zero if a call to start_write will be non-blocking
     uint8_t* (*start_write)(camio_ostream_t* this, size_t len );                //Returns a pointer to a space of size len, ready for data
     uint8_t* (*end_write)(camio_ostream_t* this, size_t len);                   //Commit the data to the buffer previously allocated, if the write was "assigned" and write want's to keep the buffer, optionally return a fresh one
     void(*flush)(camio_ostream_t* this);                                       //Flush any outstanding data
     void(*delete)(camio_ostream_t* this);                                       //Close the stream and free all memory
     int (*can_assign_write)(camio_ostream_t*);                                  //Is this stream capable of taking over another stream buffer
     int (*assign_write)(camio_ostream_t* this, uint8_t* buffer, size_t len);       //Assign the write buffer to the stream
     int fd;
     void* priv;                                                                //For stream specific structures.
};


camio_ostream_t* camio_ostream_new( char* description,  void* params);

#endif /* OSTREAM_H_ */
