/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Tools to parse and print command line options
 */

#ifndef CAMIO_OPTIONS_H_
#define CAMIO_OPTIONS_H_

#include "../camio_types.h"

typedef enum {
    CAMIO_OPTION_REQUIRED,       //This option is required for the program to run
    CAMIO_OPTION_OPTIONAL,       //This option is not required but has a default value
    CAMIO_OPTION_UNLIMTED,       //This option produces a list of outputs
    CAMIO_OPTION_FLAG,           //This option is optional, and has no arguments
} camio_options_mode_e;



typedef struct camio_options_opt{
    camio_options_mode_e mode;
    camio_types_e type;
    void* var;
    char short_str;
    const char* long_str;
    const char* descr;
    int found;
    struct camio_options_opt* next;
} camio_options_opt_t;


typedef struct {
    char* short_description;
    char* long_description;
    char* head;
    char* tail;
    camio_options_opt_t* opt_def;
    uint64_t count;
    int help;
    int unlimted_set;
} camio_options_t;


camio_options_t* fep2_options_new();
int camio_options_name(char* description);
int camio_options_tail(char* description);
int camio_options_short_description(char* description);
int camio_options_long_description(char* description);
int camio_options_add(camio_options_mode_e mode, const char short_str, const char* long_str, const char* descr, camio_types_e type, ...);
int camio_options_parse(int argc, char** argv);


/*
 * man, version 1.6c

usage: man [-adfhktwW] [section] [-M path] [-P pager] [-S list]
    [-m system] [-p string] name ...

  a : find all matching entries
  c : do not use cat file
  d : print gobs of debugging information
  D : as for -d, but also display the pages
  f : same as whatis(1)
  h : print this help message
  k : same as apropos(1)
  K : search for a string in all pages
  t : use troff to format pages for printing
  w : print location of man page(s) that would be displayed
      (if no name given: print directories that would be searched)
  W : as for -w, but display filenames only

  C file   : use `file' as configuration file
  M path   : set search path for manual pages to `path'
  P pager  : use program `pager' to display pages
  S list   : colon separated section list
  m system : search for alternate system's man pages
  p string : string tells which preprocessors to run
               e - [n]eqn(1)   p - pic(1)    t - tbl(1)
               g - grap(1)     r - refer(1)  v - vgrind(1)
 */


#endif /* CAMIO_OPTIONS_H_ */
