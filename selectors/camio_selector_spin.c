/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ spin selector
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "../camio_errors.h"
#include "../camio_util.h"

#include "camio_selector_spin.h"

/* Wrapper around `rdtsc' to take reliable timestamps flushing the pipeline */ 
#define do_rdtsc(t)                     \
    do {                                \
        u_int __regs[4];                \
        do_cpuid(0, __regs);            \
        (t) = rdtsc();                  \
    } while (0)

static __inline void
do_cpuid(u_int ax, u_int *p)
{
    __asm __volatile("cpuid"
             : "=a" (p[0]), "=b" (p[1]), "=c" (p[2]), "=d" (p[3])
             :  "0" (ax));
}

static __inline uint64_t
rdtsc(void)
{
    uint64_t rv;

    __asm __volatile("rdtsc" : "=A" (rv));
    return (rv);
}


int camio_selector_spin_init(camio_selector_t* this){
    //camio_selector_spin_t* priv = this->priv;
    return 0;
}

//Insert an istream at index specified
int camio_selector_spin_insert(camio_selector_t* this, camio_istream_t* istream, size_t index){
    camio_selector_spin_t* priv = this->priv;
    if(!istream){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No istream supplied\n");
    }

    if(priv->stream_count >= CAMIO_SELECTOR_SPIN_MAX_STREAMS){
        wprintf(CAMIO_ERR_STREAMS_OVERRUN, "Cannot insert more than %u streams in this selector\n", CAMIO_SELECTOR_SPIN_MAX_STREAMS);
        return -1;
    }

    priv->streams[priv->stream_count].index = index;
    priv->streams[priv->stream_count].istream = istream;
    priv->stream_count++;
    priv->stream_avail++;

    return 0;
}



size_t camio_selector_spin_count(camio_selector_t* this){
    camio_selector_spin_t* priv = this->priv;
    return priv->stream_avail;
}


//Remove the istream at index specified
//For performance reasons, this stream only removes from the tail
int camio_selector_spin_remove(camio_selector_t* this, size_t index){
    camio_selector_spin_t* priv = this->priv;

    size_t i = 0;
      for(i = 0; i < CAMIO_SELECTOR_SPIN_MAX_STREAMS; i++ ){
          if(priv->streams[i].index == index){
              priv->streams[i].istream = NULL;
              priv->stream_avail--;
              return 0;
          }
      }

    if(index >= CAMIO_SELECTOR_SPIN_MAX_STREAMS){
        wprintf(CAMIO_ERR_STREAMS_OVERRUN, "Cannot remove this stream (%lu) from this selector. The index could not be found.\n", index);
        return -1;
    }

    //Unreachable
    return 0;
}

//Block waiting for a change on a given istream
//return the stream number that changed
uint64_t str0= 0;
uint64_t str1 = 0;
size_t camio_selector_spin_select(camio_selector_t* this){
    camio_selector_spin_t* priv = this->priv;

    size_t i = (priv->last + 1) % priv->stream_count; //Start from the next stream to avoid starvation
    while(1){
        for(; i < priv->stream_count; i++){
//            uint64_t start = 0;
//            uint64_t end = 0;
//            do_rdtsc(start);
            if(likely(priv->streams[i].istream != NULL)){
                if(likely(priv->streams[i].istream->ready(priv->streams[i].istream))){
                    priv->last = i;
                    return priv->streams[i].index;
                }
            }
//            do_rdtsc(end);
//            printf("SSSS %lu %lu %lu", i, priv->streams[i].index, end-start);
//            if(i == 1){
//                str1 += (end - start);
//                printf("%lu\n",str1);
//            }
//            else if (i == 0){
//                str0 += (end - start);
//               printf("%lu\n",str0);
//            }
//            else{
//                printf("WTF? %lu\n", i);
//            }
 
        }
        i = 0;
    }
    return ~0; //Unreachable
}


void camio_selector_spin_delete(camio_selector_t* this){
    camio_selector_spin_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_selector_t* camio_selector_spin_construct(camio_selector_spin_t* priv, camio_selector_spin_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"spin stream supplied is null\n");
    }
    //Initialize the local variables
    priv->params           = params;
    priv->stream_count     = 0;
    priv->stream_avail     = 0;
    priv->last			   = 0;
    bzero(&priv->streams,sizeof(camio_selector_spin_stream_t) * CAMIO_SELECTOR_SPIN_MAX_STREAMS) ;



    //Populate the function members
    priv->selector.priv          = priv; //Lets us access private members
    priv->selector.init          = camio_selector_spin_init;
    priv->selector.insert        = camio_selector_spin_insert;
    priv->selector.remove        = camio_selector_spin_remove;
    priv->selector.select        = camio_selector_spin_select;
    priv->selector.delete        = camio_selector_spin_delete;
    priv->selector.count         = camio_selector_spin_count;

    //Call init, because its the obvious thing to do now...
    priv->selector.init(&priv->selector);

    //Return the generic selector interface for the outside world to use
    return &priv->selector;

}

camio_selector_t* camio_selector_spin_new( camio_selector_spin_params_t* params){
    camio_selector_spin_t* priv = malloc(sizeof(camio_selector_spin_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for spin selector creation\n");
    }
    return camio_selector_spin_construct(priv,  params);
}





