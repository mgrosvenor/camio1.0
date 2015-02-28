/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ udp (newline separated) output stream
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
#include <arpa/inet.h>

#include "../camio_errors.h"
#include "../camio_util.h"

#include "camio_ostream_udp.h"


int camio_ostream_udp_open(camio_ostream_t* this, const camio_descr_t* descr ){
    camio_ostream_udp_t* priv = this->priv;
    char ip_addr[17]; //IP addr is worst case, 16 bytes long (255.255.255.255)
    char udp_port[6]; //UDP port is wost case, 5 bytes long (65536)
    int udp_sock_fd;

    if(descr->opt_head){
         eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
     }

    if(!descr->query){
        eprintf_exit(CAMIO_ERR_SOCKET, "No address supplied\n");
    }

    const size_t query_len = strlen(descr->query);
    if(query_len > 22){
        eprintf_exit(CAMIO_ERR_SOCKET, "IP Address supplied is >22 chars \"%s\"\n", descr->query);
    }

    //Find the IP:port
    size_t i = 0;
    for(; i < query_len; i++ ){
        if(descr->query[i] == ':'){
            memcpy(ip_addr,descr->query,i);
            ip_addr[i] = '\0';
            memcpy(udp_port,&descr->query[i+1],query_len - i -1);
            udp_port[query_len - i -1] = '\0';
            break;
        }
    }


    if(query_len > 22){
        eprintf_exit(CAMIO_ERR_SOCKET, "IP Address supplied does not have a \":\" for the port number \"%s\"\n", descr->query);
    }


    priv->buffer = malloc(getpagesize()); //Allocate 1 page for the buffer
    if(!priv->buffer){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "Failed to allocate message buffer\n");
    }
    priv->buffer_size = getpagesize();

    /* Open the udp socket MAC/PHY layer output stage */
    udp_sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if (udp_sock_fd < 0 ){
        eprintf_exit(CAMIO_ERR_SOCKET,"Could not open udp socket. Error = %s\n",strerror(errno));
    }


    int SNDBUFF_SIZE = 512 * 1024 * 1024;
    if (setsockopt(udp_sock_fd, SOL_SOCKET, SO_SNDBUF, &SNDBUFF_SIZE, sizeof(SNDBUFF_SIZE)) < 0) {
        eprintf_exit(CAMIO_ERR_SOCK_OPT,"Could not set socket option. Error = %s\n",strerror(errno));
    }

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port        = htons(strtol(udp_port,NULL,10));

    priv->addr = addr;
    this->fd = udp_sock_fd;
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}

void camio_ostream_udp_close(camio_ostream_t* this){
    camio_ostream_udp_t* priv = this->priv;
    close(this->fd);
    priv->is_closed = 1;
}



//Returns a pointer to a space of size len, ready for data
uint8_t* camio_ostream_udp_start_write(camio_ostream_t* this, size_t len ){
    camio_ostream_udp_t* priv = this->priv;

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
int camio_ostream_udp_ready(camio_ostream_t* this){
    //Not implemented
    eprintf_exit(CAMIO_ERR_NOT_IMPL, "\n");
    return 0;
}


//Commit the data to the buffer previously allocated
//Len must be equal to or less than len called with start_write
uint8_t* camio_ostream_udp_end_write(camio_ostream_t* this, size_t len){
    camio_ostream_udp_t* priv = this->priv;
    int result = 0;

    if(priv->assigned_buffer){
        result = sendto(this->fd,priv->assigned_buffer,len,0,(struct sockaddr*)&priv->addr, sizeof(priv->addr));
        if(result < 1){
            eprintf_exit(CAMIO_ERR_SEND, "Could not send on udp socket. Error = %s\n", strerror(errno));
        }

        priv->assigned_buffer    = NULL;
        priv->assigned_buffer_sz = 0;
        return NULL;
    }

    result = sendto(this->fd,priv->buffer,len,0,(struct sockaddr*)&priv->addr, sizeof(priv->addr));
    if(result < 1){
        eprintf_exit(CAMIO_ERR_SEND, "Could not send on udp socket. Error = %s\n", strerror(errno));
    }
    return NULL;
}


void camio_ostream_udp_delete(camio_ostream_t* ostream){
    ostream->close(ostream);
    camio_ostream_udp_t* priv = ostream->priv;
    free(priv);
}

//Is this stream capable of taking over another stream buffer
int camio_ostream_udp_can_assign_write(camio_ostream_t* this){
    return 1;
}

//Assign the write buffer to the stream
int camio_ostream_udp_assign_write(camio_ostream_t* this, uint8_t* buffer, size_t len){
    camio_ostream_udp_t* priv = this->priv;

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

camio_ostream_t* camio_ostream_udp_construct(camio_ostream_udp_t* priv, const camio_descr_t* descr,  camio_ostream_udp_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"udp stream supplied is null\n");
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
    priv->ostream.open              = camio_ostream_udp_open;
    priv->ostream.close             = camio_ostream_udp_close;
    priv->ostream.start_write       = camio_ostream_udp_start_write;
    priv->ostream.end_write         = camio_ostream_udp_end_write;
    priv->ostream.ready             = camio_ostream_udp_ready;
    priv->ostream.delete            = camio_ostream_udp_delete;
    priv->ostream.can_assign_write  = camio_ostream_udp_can_assign_write;
    priv->ostream.assign_write      = camio_ostream_udp_assign_write;
    priv->ostream.fd                = -1;

    //Call open, because its the obvious thing to do now...
    priv->ostream.open(&priv->ostream, descr);

    //Return the generic ostream interface for the outside world
    return &priv->ostream;

}

camio_ostream_t* camio_ostream_udp_new( const camio_descr_t* descr,  camio_ostream_udp_params_t* params){
    camio_ostream_udp_t* priv = malloc(sizeof(camio_ostream_udp_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for ostream udp creation\n");
    }
    return camio_ostream_udp_construct(priv, descr,  params);
}



