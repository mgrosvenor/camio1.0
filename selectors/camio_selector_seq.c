/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ seq selector
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "../camio_errors.h"
#include "../camio_util.h"
#include "camio_selector_seq.h"


int camio_selector_seq_init(camio_selector_t* this){
    //camio_selector_seq_t* priv = this->priv;
    return 0;
}

//Insert an istream at index specified
int camio_selector_seq_insert(camio_selector_t* this, camio_istream_t* istream, size_t index){
    camio_selector_seq_t* priv = this->priv;
    if(!istream){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No istream supplied\n");
    }

    if(priv->stream_count >= CAMIO_SELECTOR_SEQ_MAX_STREAMS){
        wprintf(CAMIO_ERR_STREAMS_OVERRUN, "Cannot insert more than %u streams in this selector\n", CAMIO_SELECTOR_SEQ_MAX_STREAMS);
        return -1;
    }

    priv->streams[priv->stream_count].index = index;
    priv->streams[priv->stream_count].istream = istream;
    priv->stream_count++;
    priv->stream_avail++;

    return 0;
}

size_t camio_selector_seq_count(camio_selector_t* this){
    camio_selector_seq_t* priv = this->priv;
    return priv->stream_avail;
}



//Remove the istream at index specified
//For performance reasons, this stream only removes from the tail
int camio_selector_seq_remove(camio_selector_t* this, size_t index){
    camio_selector_seq_t* priv = this->priv;

    size_t i = 0;
      for(i = 0; i < CAMIO_SELECTOR_SEQ_MAX_STREAMS; i++ ){
          if(priv->streams[i].index == index){
              priv->streams[i].istream = NULL;
              priv->stream_avail--;
              return 0;
          }
      }

    if(index >= CAMIO_SELECTOR_SEQ_MAX_STREAMS){
        wprintf(CAMIO_ERR_STREAMS_OVERRUN, "Cannot remove this stream (%lu) from this selector. The index could not be found.\n", index);
        return -1;
    }

    //Unreachable
    return 0;
}

//Block waiting for a change on a given istream
//return the stream number that changed
size_t camio_selector_seq_select(camio_selector_t* this){
    camio_selector_seq_t* priv = this->priv;

    size_t i = 0; //Start from zero every time to ensure starvation
    while(1){
        for(; i < priv->stream_count; i++){
            if(likely(priv->streams[i].istream != NULL)){
                if(likely(priv->streams[i].istream->ready(priv->streams[i].istream))){
                    return priv->streams[i].index;
                }
            }
        }
        i = 0;
    }
    return ~0; //Unreachable
}


void camio_selector_seq_delete(camio_selector_t* this){
    camio_selector_seq_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_selector_t* camio_selector_seq_construct(camio_selector_seq_t* priv, camio_selector_seq_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"seq stream supplied is null\n");
    }
    //Initialize the local variables
    priv->params           = params;
    priv->stream_count     = 0;
    priv->stream_avail     = 0;
    bzero(&priv->streams,sizeof(camio_selector_seq_stream_t) * CAMIO_SELECTOR_SEQ_MAX_STREAMS) ;



    //Populate the function members
    priv->selector.priv          = priv; //Lets us access private members
    priv->selector.init          = camio_selector_seq_init;
    priv->selector.insert        = camio_selector_seq_insert;
    priv->selector.remove        = camio_selector_seq_remove;
    priv->selector.select        = camio_selector_seq_select;
    priv->selector.delete        = camio_selector_seq_delete;
    priv->selector.count         = camio_selector_seq_count;

    //Call init, because its the obvious thing to do now...
    priv->selector.init(&priv->selector);

    //Return the generic selector interface for the outside world to use
    return &priv->selector;

}

camio_selector_t* camio_selector_seq_new( camio_selector_seq_params_t* params){
    camio_selector_seq_t* priv = malloc(sizeof(camio_selector_seq_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for seq selector creation\n");
    }
    return camio_selector_seq_construct(priv,  params);
}





