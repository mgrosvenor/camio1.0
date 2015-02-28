/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Input stream definition
 *
 */

#ifndef CAMIO_ISTREAM_H_
#define CAMIO_ISTREAM_H_

#include <unistd.h>

#include "../camio_descr.h"

struct camio_istream;
typedef struct camio_istream camio_istream_t;

struct camio_istream{
     int64_t (*open)(camio_istream_t* this, const camio_descr_t* desc ); //Open the stream and prepare for reading, return 0 if it succeeds
     void(*close)(camio_istream_t* this);                         //Close the stream
     int64_t (*ready)(camio_istream_t* this);                         //Returns non-zero if a call to start_read will be non-blocking
     int64_t (*start_read)(camio_istream_t* this, uint8_t** out_bytes);  //Returns the number of bytes available to read, this can be 0. If bytes available is non-zero, out_bytes has a pointer to the start of the bytes to read
     int64_t (*end_read)(camio_istream_t* this, uint8_t* free_buff);     //Returns 0 if the contents of out_bytes have NOT changed since the call to start_read. For buffers this may fail, if this is the case, data read in start_read maybe corrupt.
     void(*delete)(camio_istream_t* this);                        //Closes the stream and deletes the memory used
     int64_t fd;                                                     //Expose the file descriptor to the outside world, useful for selectors
     void* priv;
};

camio_istream_t* camio_istream_new(const char* description,  void* parameters);

#endif /* CAMIO_ISTREAM_H_ */
