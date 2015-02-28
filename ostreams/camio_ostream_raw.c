/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ raw (newline separated) output stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <fcntl.h>

#include "../camio_errors.h"

#include "camio_ostream_raw.h"



int camio_ostream_raw_open(camio_ostream_t* this, const camio_descr_t* descr ){
    camio_ostream_raw_t* priv = this->priv;
    const char* iface = descr->query;
    int raw_sock_fd;

    if(descr->opt_head){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(!descr->query){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No interface supplied\n");
    }

    priv->buffer = malloc(getpagesize()); //Allocate 1 page
    if(!priv->buffer){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "Failed to allocate message buffer\n");
    }
    priv->buffer_size = getpagesize();

    /* Open the raw socket MAC/PHY layer output stage */
    if ( !(raw_sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) ){
        eprintf_exit(CAMIO_ERR_SOCKET,"Could not open raw socket. Error = %s\n",strerror(errno));
    }

    /* get the interface num */
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, iface, IFNAMSIZ-1);
    if (ioctl(raw_sock_fd, SIOCGIFINDEX, &if_idx) < 0){
        eprintf_exit(CAMIO_ERR_IOCTL,"Could not get interface name. Error = %s\n",strerror(errno));
    }

    struct sockaddr_ll socket_address;
    socket_address.sll_family   = PF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    socket_address.sll_ifindex  =  if_idx.ifr_ifindex;

    if( bind(raw_sock_fd, (struct sockaddr *)&socket_address, sizeof(socket_address)) ){
        eprintf_exit(CAMIO_ERR_BIND,"Could not bind raw socket. Error = %s\n",strerror(errno));
    }

    int SNDBUFF_SIZE = 512 * 1024 * 1024;
    if (setsockopt(raw_sock_fd, SOL_SOCKET, SO_SNDBUF, &SNDBUFF_SIZE, sizeof(SNDBUFF_SIZE)) < 0) {
        eprintf_exit(CAMIO_ERR_SOCK_OPT,"Could not set socket option. Error = %s\n",strerror(errno));
    }

    this->fd = raw_sock_fd;
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}

void camio_ostream_raw_close(camio_ostream_t* this){
    camio_ostream_raw_t* priv = this->priv;
    close(this->fd);
    priv->is_closed = 1;
}



//Returns a pointer to a space of size len, ready for data
uint8_t* camio_ostream_raw_start_write(camio_ostream_t* this, size_t len ){
    camio_ostream_raw_t* priv = this->priv;

    //Grow the buffer if it's not big enough
    if(len > priv->buffer_size){
        priv->buffer = realloc(priv->buffer, len);
        if(!priv->buffer){
            eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not grow message buffer\n");
        }
        priv->buffer_size = len;
    }

    return priv->buffer;
}

//Returns non-zero if a call to start_write will be non-blocking
int camio_ostream_raw_ready(camio_ostream_t* this){
    //Not implemented
    eprintf_exit(CAMIO_ERR_NOT_IMPL, "\n");
    return 0;
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

//Commit the data to the buffer previously allocated
//Len must be equal to or less than len called with start_write
uint8_t* camio_ostream_raw_end_write(camio_ostream_t* this, size_t len){
    camio_ostream_raw_t* priv = this->priv;
    int result = 0;

    set_fd_blocking(this->fd,1);
    if(priv->assigned_buffer){
        result = send(this->fd,priv->assigned_buffer,len,0);
        if(result < 1){
            if(errno == EAGAIN){
                eprintf_exit(CAMIO_ERR_SEND, "Could not send on socket, non-blocking. Error = %s\n", strerror(errno));
            }
            eprintf_exit(CAMIO_ERR_SEND, "Could not send on raw socket. Error = %s\n", strerror(errno));
        }

        priv->assigned_buffer    = NULL;
        priv->assigned_buffer_sz = 0;
        return NULL;
    }

    result = send(this->fd,priv->buffer,len,0);
    if(result < 1){
        eprintf_exit(CAMIO_ERR_SEND, "Could not send on raw socket. Error = %s\n", strerror(errno));
    }
    return NULL;
}


void camio_ostream_raw_delete(camio_ostream_t* ostream){
    ostream->close(ostream);
    camio_ostream_raw_t* priv = ostream->priv;
    free(priv);
}

//Is this stream capable of taking over another stream buffer
int camio_ostream_raw_can_assign_write(camio_ostream_t* this){
    return 1;
}

//Assign the write buffer to the stream
int camio_ostream_raw_assign_write(camio_ostream_t* this, uint8_t* buffer, size_t len){
    camio_ostream_raw_t* priv = this->priv;

    if(!buffer){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"Assigned buffer is null.");
    }

    priv->assigned_buffer    = buffer;
    priv->assigned_buffer_sz = len;

    return 0;
}


/* ****************************************************
 * Construction heavy lifting
 */

camio_ostream_t* camio_ostream_raw_construct(camio_ostream_raw_t* priv, const camio_descr_t* descr,  camio_ostream_raw_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"raw stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed             = 1;
    priv->buffer_size           = 0;
    priv->buffer                = NULL;
    priv->assigned_buffer       = NULL;
    priv->assigned_buffer_sz    = 0;
    priv->params                = params;


    //Populate the function members
    priv->ostream.priv              = priv; //Lets us access private members from public functions
    priv->ostream.open              = camio_ostream_raw_open;
    priv->ostream.close             = camio_ostream_raw_close;
    priv->ostream.start_write       = camio_ostream_raw_start_write;
    priv->ostream.end_write         = camio_ostream_raw_end_write;
    priv->ostream.ready             = camio_ostream_raw_ready;
    priv->ostream.delete            = camio_ostream_raw_delete;
    priv->ostream.can_assign_write  = camio_ostream_raw_can_assign_write;
    priv->ostream.assign_write      = camio_ostream_raw_assign_write;
    priv->ostream.fd                = -1;

    //Call open, because its the obvious thing to do now...
    priv->ostream.open(&priv->ostream, descr);

    //Return the generic ostream interface for the outside world
    return &priv->ostream;

}

camio_ostream_t* camio_ostream_raw_new( const camio_descr_t* descr,  camio_ostream_raw_params_t* params){
    camio_ostream_raw_t* priv = malloc(sizeof(camio_ostream_raw_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for ostream raw creation\n");
    }
    return camio_ostream_raw_construct(priv, descr,  params);
}



