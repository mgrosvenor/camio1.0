/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ periodic timeout input stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "camio_istream_periodic_timeout.h"
#include "../camio_errors.h"


int64_t camio_istream_periodic_timeout_open(camio_istream_t* this, const camio_descr_t* opts ){
    camio_istream_periodic_timeout_t* priv = this->priv;
    uint64_t seconds = 0;
    uint64_t nanoseconds = 0;


    if(opts->opt_head){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    //Parse the time spec
    if(!opts->query){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No timer specification supplied expected 'seconds.nanoseconds' format\n");
    }

    uint64_t temp = 0;
    size_t i = 0;
    for(; opts->query[i] != '\0'; i++){
        if(opts->query[i] == '.'){
            seconds = temp;
            temp = 0;
            continue;
        }
        temp *= 10;
        temp += (opts->query[i] - '0');
    }
    nanoseconds = temp;

    //Just in case the nanoseconds arg overflows
    nanoseconds %= 1000 * 1000 * 1000;
    seconds     += nanoseconds / (1000 * 1000 * 1000);

    //Set the time spec
    struct itimerspec new = { .it_value = { seconds , nanoseconds }, .it_interval = { seconds , nanoseconds } };

    //Grab a file descriptor
    this->fd = timerfd_create(CLOCK_MONOTONIC,0);
    if(this->fd == -1){
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open monotonic clock");
    }

    //set the periodic timeout
    if(timerfd_settime(this->fd,0,&new,NULL) < -1){
        eprintf_exit(CAMIO_ERR_IOCTL, "Could not set monotonic clock", opts->query);
    }

    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}


void camio_istream_periodic_timeout_close(camio_istream_t* this){
    camio_istream_periodic_timeout_t* priv = this->priv;

    //Stop the timer
    struct itimerspec new = { .it_value = {0,0}, .it_interval = {0 , 0 } };
    timerfd_settime(this->fd,0,&new,NULL);

    //Close the file
    close(this->fd);
    priv->is_closed = 1;
}


static void set_fd_blocking(int fd, int blocking){
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1){
        eprintf_exit(CAMIO_ERR_FILE_FLAGS, "Could not get file flags\n");
    }

    if (blocking){
        flags &= ~O_NONBLOCK;
    }
    else{
        flags |= O_NONBLOCK;
    }

    if( fcntl(fd, F_SETFL, flags) == -1){
        eprintf_exit(CAMIO_ERR_FILE_FLAGS, "Could not set file flags\n");
    }
}


static int prepare_next(camio_istream_periodic_timeout_t* priv, int blocking){
    //Set the file blocking mode as requested
    if(blocking != priv->blocking){
        set_fd_blocking(priv->istream.fd,blocking);
        priv->blocking = blocking;
    }

    //Read the data
    int bytes = read(priv->istream.fd,&priv->expiries,8);

    //Was there some error
    if(bytes < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return 0; //Reading would have blocked, that's fine.
        }

        //Uh ohh, some other error! Eek! Die!
        eprintf_exit(CAMIO_ERR_FILE_READ, "Could not read periodic_timeout input error no=%i (%s)\n", errno, strerror(errno));
    }

    //Woot
    priv->read_size = bytes;
    return bytes;
}

int64_t camio_istream_periodic_timeout_ready(camio_istream_t* this){
    camio_istream_periodic_timeout_t* priv = this->priv;
    if(priv->read_size || priv->is_closed){
        return 1;
    }

    return prepare_next(priv,CAMIO_ISTREAM_PERIODIC_TIMEOUT_NONBLOCKING);
}

int64_t camio_istream_periodic_timeout_start_read(camio_istream_t* this, uint8_t** out){
    *out = NULL;

    camio_istream_periodic_timeout_t* priv = this->priv;
    if(priv->is_closed){
        return 0;
    }

    //Called read without calling ready, they must want to block
    if(!priv->read_size){
        if(!prepare_next(priv,CAMIO_ISTREAM_PERIODIC_TIMEOUT_BLOCKING)){
            return 0;
        }
    }

    *out = (uint8_t*)&priv->expiries;
    size_t result = priv->read_size;

    priv->read_size          = 0; //Reset the read size for next time
    return result;
}


int64_t camio_istream_periodic_timeout_end_read(camio_istream_t* this, uint8_t* free_buff){
    return 0;
}


void camio_istream_periodic_timeout_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_periodic_timeout_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_periodic_timeout_construct(camio_istream_periodic_timeout_t* priv, const camio_descr_t* opts,  camio_istream_periodic_timeout_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"periodic_timeout stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed         = 1;
    priv->read_size         = 0;
    priv->expiries          = 0;
    priv->blocking          = 1;
    priv->params            = params;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_periodic_timeout_open;
    priv->istream.close         = camio_istream_periodic_timeout_close;
    priv->istream.start_read    = camio_istream_periodic_timeout_start_read;
    priv->istream.end_read      = camio_istream_periodic_timeout_end_read;
    priv->istream.ready         = camio_istream_periodic_timeout_ready;
    priv->istream.delete        = camio_istream_periodic_timeout_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, opts);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_periodic_timeout_new( const camio_descr_t* opts,  camio_istream_periodic_timeout_params_t* params){
    camio_istream_periodic_timeout_t* priv = malloc(sizeof(camio_istream_periodic_timeout_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for periodic_timeout istream creation\n");
    }
    return camio_istream_periodic_timeout_construct(priv, opts,  params);
}





