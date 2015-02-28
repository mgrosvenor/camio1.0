/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Selector definition
 *
 */

#ifndef SELECTOR_H_
#define SELECTOR_H_

#include <unistd.h>

#include "../camio_descr.h"
#include "../istreams/camio_istream.h"

struct camio_selector;
typedef struct camio_selector camio_selector_t;

struct camio_selector{
     int(*init)(camio_selector_t* this);                                          //Construct and init the selector
     int(*insert)(camio_selector_t* this, camio_istream_t* istream, size_t index); //Insert an istream at index specified
     int(*remove)(camio_selector_t* this, size_t index);                          //Remove the istream at index specified
     size_t(*select)(camio_selector_t* this);                                     //Block waiting for a change on a given istream
     void(*delete)(camio_selector_t* this);                                       //Closes the stream and deletes the memory used
     size_t (*count)(camio_selector_t* this);                                      //Returns the number of streams in this selctor
     void* priv;
};

camio_selector_t* camio_selector_new(const char* description,  void* parameters);

#endif /* SELECTOR_H_ */
