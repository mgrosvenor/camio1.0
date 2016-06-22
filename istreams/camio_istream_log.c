/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ log (newline separated) input stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "camio_istream_log.h"
#include "../camio_errors.h"

#define CAMIO_ISTREAM_ISTREAM_LOG_BUFF_INIT 4096

int64_t camio_istream_log_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_log_t* priv = this->priv;

    if(descr->opt_head){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    priv->line_buffer = malloc(CAMIO_ISTREAM_ISTREAM_LOG_BUFF_INIT);
    if(!priv->line_buffer){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not allocate line buffer\n");
    }
    priv->line_buffer_size  = CAMIO_ISTREAM_ISTREAM_LOG_BUFF_INIT;
    priv->data_head_ptr     = priv->line_buffer;

    //If we have a file descriptor from the outside world, then use it!
    if(priv->params){
        if(priv->params->fd > -1){
            this->fd = priv->params->fd;
            priv->is_closed = 0;
            return CAMIO_ERR_NONE;
        }
    }

    //Grab a file descriptor and rock on
    if(!descr->query){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No filename supplied\n");
    }

    this->fd = open(descr->query, O_RDONLY);
    if(this->fd == -1){
        printf("\"%s\"",descr->query);
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\"\n", descr->query);
    }
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}


void camio_istream_log_close(camio_istream_t* this){
    camio_istream_log_t* priv = this->priv;
    close(this->fd);
    priv->is_closed = 1;
}


static inline void set_fd_blocking(int fd, int blocking){
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

static int64_t read_to_buff(camio_istream_log_t* priv, uint8_t* new_data_ptr, int blocking){

    //Set the file blocking mode as requested
    set_fd_blocking(priv->istream.fd,blocking);

    //Read the data
    size_t amount = (priv->line_buffer + priv->line_buffer_size -1) - new_data_ptr;
    int bytes = read(priv->istream.fd,new_data_ptr,amount);

    //Was there some error
    if(bytes < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return 0; //Reading would have blocked, we don't want this
        }

        //Uh ohh, some other error! Eek! Die!
        eprintf_exit(CAMIO_ERR_FILE_READ, "Could not read tlog input error no=%i (%s)\n", errno, strerror(errno));
    }

    //We've hit the end of the file. Close and leave.
    if(bytes == 0){
        priv->istream.close(&priv->istream);
        return 0;
    }

    //Woot
    priv->line_buffer_count += bytes;
    return bytes;
}


int ascii_hex_to_int(uint8_t a){
    switch(a){
        case '0': return 0x0;
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0x4;
        case '5': return 0x5;
        case '6': return 0x6;
        case '7': return 0x7;
        case '8': return 0x8;
        case '9': return 0x9;
        case 'a': return 0xa;
        case 'b': return 0xb;
        case 'c': return 0xc;
        case 'd': return 0xd;
        case 'e': return 0xe;
        case 'f': return 0xf;
        case 'A': return 0xA;
        case 'B': return 0xB;
        case 'C': return 0xC;
        case 'D': return 0xD;
        case 'E': return 0xE;
        case 'F': return 0xF;
        default:
            return -1;
    }
}

size_t unescape(uint8_t* buffer, size_t size){
    size_t result = size;
    size_t read = 0;
    size_t write = 0;
    for(;read < size -3; read++, write++){
        //Look for escaped characters
        //check for the pattern \x1A

        int lower = ascii_hex_to_int(buffer[read+2]);
        int upper = ascii_hex_to_int(buffer[read+3]);

        if((buffer[read+0] == '\\') &&
           (buffer[read+1] == 'x' || buffer[read+1] == 'X') &&
           (lower > -1)  &&
           (upper > -1) ){
            buffer[write] = upper << 4 & lower;
            read += 3; //Jump read to the next valid char
            result -= 3; //Shrink the size of the buffer
         }
    }

    return result;
}





static int64_t prepare_next(camio_istream_log_t* priv, int blocking){
    if(!priv->line_buffer_count){
        priv->data_head_ptr = priv->line_buffer;
        int bytes = read_to_buff(priv,priv->data_head_ptr, blocking);

        if(!bytes){
            return 0;
        }
    }

    while(1){
        //Unescape the whole buffer first
        if(priv->escape){
            priv->line_buffer_count = unescape(priv->data_head_ptr, priv->line_buffer_count);
        }

        size_t i = 0;
        for(;i < priv->line_buffer_count; i++){

            //Search for a newline in the file
            if(priv->data_head_ptr[i] == '\n' || priv->data_head_ptr[i] == '\r'){
                if(priv->data_head_ptr[i+1] == '\n'){ //Handle windows style line feeds
                    i++;
                }
                i++; //Add 1 to include the line feed
                priv->read_size = i;
                return i; //Found a new line, return the line size
            }
        }

        //We didn't find a newline, read some more data and try again
        //--------------------------------------------------------------
        //Is there more space in the buffer?
        if(priv->data_head_ptr + priv->line_buffer_count < priv->line_buffer + priv->line_buffer_size -1){
            int bytes = read_to_buff(priv,priv->data_head_ptr + priv->line_buffer_count, blocking);
            if(!bytes){
                return 0; //End of the file
            }

            continue;

        }
        //No. OK, see if we can move things around in this buffer to get some more data into it
        else if(priv->data_head_ptr != priv->line_buffer){
            priv->line_buffer = memmove(priv->line_buffer,priv->data_head_ptr, priv->line_buffer_count );
            priv->data_head_ptr = priv->line_buffer; //Reset the read pointer
        }
        //If that's not possible, try expanding the buffer
        else{
            //Grow the buffer by a power of 2
            priv->line_buffer = realloc(priv->line_buffer, priv->line_buffer_size * 2);
            priv->data_head_ptr = priv->line_buffer;
            if(!priv->line_buffer){
                eprintf_exit(CAMIO_ERR_NULL_PTR, "Could not grow line buffer\n");
            }
            priv->line_buffer_size *= 2;
        }

        //No we can read in some more data
        int bytes = read_to_buff(priv,priv->data_head_ptr + priv->line_buffer_count, blocking);
        if(!bytes){
            return 0; //End of the file
        }
    }

    //Unreachable
    return 0;

}

int64_t camio_istream_log_ready(camio_istream_t* this){
    camio_istream_log_t* priv = this->priv;
    if(priv->read_size || priv->is_closed){
        return 1;
    }

    prepare_next(priv,CAMIO_ISTREAM_LOG_NONBLOCKING);

    return priv->read_size || priv->is_closed;
}

int64_t camio_istream_log_start_read(camio_istream_t* this, uint8_t** out){
    *out = NULL;

    camio_istream_log_t* priv = this->priv;
    if(priv->is_closed){
        return 0;
    }

    //Called read without calling ready, they must want to block
    if(!priv->read_size){
        if(!prepare_next(priv,CAMIO_ISTREAM_LOG_BLOCKING)){
            return 0;
        }
    }

    *out = priv->data_head_ptr;
    size_t result = priv->read_size -1; //Strip off the newline

    priv->data_head_ptr     += priv->read_size; //Advance the pointer to the next byte at end of the value just read
    priv->line_buffer_count -= priv->read_size; //Forget about the bytes just read
    priv->read_size          = 0; //Reset the read size for next time

    return result;
}


int64_t camio_istream_log_end_read(camio_istream_t* this, uint8_t* free_buff){
    return 0; //Always true for file I/O
}


void camio_istream_log_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_log_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_log_construct(camio_istream_log_t* priv, const camio_descr_t* descr,  camio_istream_log_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"log stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed         = 1;
    priv->line_buffer_count = 0;
    priv->read_size         = 0;
    priv->line_buffer_size  = 0;
    priv->data_head_ptr     = NULL;
    priv->escape            = 0; //Off for the moment since this is borked
    priv->params            = params;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_log_open;
    priv->istream.close         = camio_istream_log_close;
    priv->istream.start_read    = camio_istream_log_start_read;
    priv->istream.end_read      = camio_istream_log_end_read;
    priv->istream.ready         = camio_istream_log_ready;
    priv->istream.delete        = camio_istream_log_delete;
    priv->istream.fd            = -1;
    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_log_new( const camio_descr_t* descr,  camio_istream_log_params_t* params){
    camio_istream_log_t* priv = malloc(sizeof(camio_istream_log_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for log istream creation\n");
    }
    return camio_istream_log_construct(priv, descr,  params);
}





