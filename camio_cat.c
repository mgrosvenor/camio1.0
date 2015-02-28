/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#include "istreams/camio_istream_log.h"
#include "ostreams/camio_ostream_log.h"
#include "options/camio_options.h"
#include "camio_types.h"

#include "camio_util.h"

camio_list_t(istream) istreams = {};
camio_list_t(ostream) ostreams = {};

void term(int signum){
    int i;
    for(i=0; i < istreams.count; i++){ istreams.items[i]->delete(istreams.items[i]);}
    for(i=0; i < ostreams.count; i++){ ostreams.items[i]->delete(ostreams.items[i]);}
    exit(0);
}


struct camio_cat_options_t{
    camio_list_t(string) inputs;
    camio_list_t(string) outputs;
    char* clock;
    char* selector;
} options ;


int main(int argc, char** argv){

    signal(SIGTERM, term);
    signal(SIGINT, term);


    camio_options_short_description("camio_cat");
    camio_options_add(CAMIO_OPTION_UNLIMTED, 'i', "input",     "One or more input descriptions in camio format. eg log:/file.txt",  CAMIO_STRINGS, &options.inputs, "std-log"   );
    camio_options_add(CAMIO_OPTION_OPTIONAL, 'o', "output",    "One or more output descriptions in camio format. eg log:/file.txt", CAMIO_STRINGS, &options.outputs, "std-log");
    camio_options_add(CAMIO_OPTION_OPTIONAL, 's', "selector",  "Selector description eg selection", CAMIO_STRING, &options.selector, "spin" );
    camio_options_long_description("Concatenates one or more inputs, into one or more outputs. \n - If no inputs are supplied, defaults to standard in.\n - If no outputs are supplied, defaults to standard out.");
    camio_options_parse(argc, argv);

    camio_selector_t* selector = camio_selector_new(options.selector,NULL);

    camio_list_init(istream,&istreams,options.inputs.count);
    camio_list_init(ostream,&ostreams,options.outputs.count);

    int i;
    for(i = 0; i < options.inputs.count; i++){
        camio_istream_t* in = camio_istream_new(options.inputs.items[i],  NULL);
        selector->insert(selector,in,i);
        camio_list_add(istream,&istreams,in);
    }

    for(i = 0; i < options.outputs.count; i++){
        camio_ostream_t* out = camio_ostream_new(options.outputs.items[i],  NULL);
        camio_list_add(ostream,&ostreams,out);
    }

    uint8_t* in_buff = NULL;
    uint8_t* out_buff = NULL;
    size_t len = 0;
    size_t which = ~0;
    uint8_t* free_buff = NULL;

    while(selector->count(selector)){

        //Wait for some input
        which = selector->select(selector);

        //Read the input from the right stream
        camio_istream_t* in = istreams.items[which];
        len = in->start_read(in, &in_buff );
        if(unlikely(!len)){
            selector->remove(selector,which);
            continue;
        }

        //Write it out
        for(i=0; i < ostreams.count; i++){
            camio_ostream_t* out = ostreams.items[i];

            //Assign writes may imply a memory copy
            if(likely(out->can_assign_write(out))){
                out->assign_write(out,in_buff,len);
                in_buff = NULL;
            }
            //Non assigned writes require memory copy
            else{
                 out_buff = out->start_write(out,len);
                 if(unlikely(!out_buff)){
                     printf("Could not get an output buffer for output %i\n", i);
                     continue;
                 }
                 memcpy(out_buff,in_buff,len);
                 out_buff = NULL;
            }

            free_buff = out->end_write(out, len);
            if(unlikely(in->end_read(in, free_buff))){
                printf("Overrun detected for output %i\n", i);
            }
        }
    }

    term(0);

    //Unreachable
    return 0;
}
