/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ poll selector
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include "../camio_errors.h"
#include "../camio_util.h"

#include "camio_selector_poll.h"


int camio_selector_poll_init(camio_selector_t* this){
    //camio_selector_poll_t* priv = this->priv;
    return 0;
}

//Insert an istream at index specified
int camio_selector_poll_insert(camio_selector_t* this, camio_istream_t* istream, size_t index){
    camio_selector_poll_t* priv = this->priv;
    if(!istream){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No istream supplied\n");
    }

    if(priv->stream_count >= CAMIO_SELECTOR_POLL_MAX_STREAMS){
        wprintf(CAMIO_ERR_STREAMS_OVERRUN, "Cannot insert more than %u streams in this selector\n", CAMIO_SELECTOR_POLL_MAX_STREAMS);
        return -1;
    }

    priv->streams[priv->stream_count].index = index;
    priv->streams[priv->stream_count].istream = istream;
    priv->fds[priv->stream_count].fd          = istream->fd;
    priv->fds[priv->stream_count].events      = POLLIN;

    priv->stream_count++;
    priv->stream_avail++;

    return 0;
}



size_t camio_selector_poll_count(camio_selector_t* this){
    camio_selector_poll_t* priv = this->priv;
    return priv->stream_avail;
}


//Remove the istream at index specified
//For performance reasons, this stream only removes from the tail
int camio_selector_poll_remove(camio_selector_t* this, size_t index){
    camio_selector_poll_t* priv = this->priv;

    size_t i = 0;
      for(i = 0; i < CAMIO_SELECTOR_POLL_MAX_STREAMS; i++ ){
          if(priv->streams[i].index == index){
              priv->streams[i].istream = NULL;
              priv->fds[i].fd          = -1;
              priv->stream_avail--;
              return 0;
          }
      }

    if(index >= CAMIO_SELECTOR_POLL_MAX_STREAMS){
        wprintf(CAMIO_ERR_STREAMS_OVERRUN, "Cannot remove this stream (%lu) from this selector. The index could not be found.\n", index);
        return -1;
    }

    //Unreachable
    return 0;
}

//Block waiting for a change on a given istream
//return the stream number that changed
size_t camio_selector_poll_select(camio_selector_t* this){
    camio_selector_poll_t* priv = this->priv;

    int result = poll(priv->fds,priv->stream_count,-1);
    if(result < 0){
        eprintf_exit(CAMIO_ERR_FILE_READ, "Poll failed with error =%s", strerror(errno));
    }

    size_t i = (priv->last + 1) % priv->stream_count; //Start from the next stream to avoid starvation
    for(; i < priv->stream_count; i++){
        if(likely(priv->streams[i].istream != NULL && priv->fds[i].fd != -1 && (priv->fds[i].revents & POLLIN) )){
            priv->last = i;
            return priv->streams[i].index;
        }
    }

    return ~0; //Unreachable
}


void camio_selector_poll_delete(camio_selector_t* this){
    camio_selector_poll_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_selector_t* camio_selector_poll_construct(camio_selector_poll_t* priv, camio_selector_poll_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"poll stream supplied is null\n");
    }
    //Initialize the local variables
    priv->params           = params;
    priv->stream_count     = 0;
    priv->stream_avail     = 0;
    priv->last			   = 0;
    bzero(&priv->streams,sizeof(camio_selector_poll_stream_t) * CAMIO_SELECTOR_POLL_MAX_STREAMS) ;



    //Populate the function members
    priv->selector.priv          = priv; //Lets us access private members
    priv->selector.init          = camio_selector_poll_init;
    priv->selector.insert        = camio_selector_poll_insert;
    priv->selector.remove        = camio_selector_poll_remove;
    priv->selector.select        = camio_selector_poll_select;
    priv->selector.delete        = camio_selector_poll_delete;
    priv->selector.count         = camio_selector_poll_count;

    //Call init, because its the obvious thing to do now...
    priv->selector.init(&priv->selector);

    //Return the generic selector interface for the outside world to use
    return &priv->selector;

}

camio_selector_t* camio_selector_poll_new(camio_selector_poll_params_t* params){
    camio_selector_poll_t* priv = malloc(sizeof(camio_selector_poll_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for poll selector creation\n");
    }
    return camio_selector_poll_construct(priv,  params);
}





