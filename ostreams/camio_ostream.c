/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Input stream factory definition
 *
 */

#include <string.h>

#include "camio_ostream.h"
#include "../camio_errors.h"

#include "camio_ostream_log.h"
#include "camio_ostream_raw.h"
#include "camio_ostream_udp.h"
#include "camio_ostream_ring.h"
#include "camio_ostream_blob.h"
#include "camio_ostream_netmap.h"


camio_ostream_t* camio_ostream_new( char* description,  void* parameters){
    camio_ostream_t* result = NULL;
    camio_descr_t descr;
    camio_descr_construct(&descr);
    camio_descr_parse(description,&descr);

    if(strcmp(descr.protocol,"log") == 0 ){
        result = camio_ostream_log_new(&descr, parameters);
    }
    else if(strcmp(descr.protocol,"std-log") == 0){
        camio_ostream_log_params_t params = { .fd = STDOUT_FILENO };
        result = camio_ostream_log_new(&descr, &params );
    }
    else if(strcmp(descr.protocol,"raw") == 0 ){
            result = camio_ostream_raw_new(&descr, parameters);
    }
    else if(strcmp(descr.protocol,"ring") == 0 ){
            result = camio_ostream_ring_new(&descr, parameters);
    }
    else if(strcmp(descr.protocol,"udp") == 0 ){
            result = camio_ostream_udp_new(&descr, parameters);
    }
    else if(strcmp(descr.protocol,"blob") == 0 ){
            result = camio_ostream_blob_new(&descr, parameters);
    }
    else if(strcmp(descr.protocol,"nmap") == 0 ){
            result = camio_ostream_netmap_new(&descr, parameters);
    }
    else{
        eprintf_exit(CAMIO_ERR_UNKNOWN_OSTREAM,"Could not create ostream from description \"%s\" \n", description);
    }

    camio_descr_destroy(&descr);
    return result;

}
