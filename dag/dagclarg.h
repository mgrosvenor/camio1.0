/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagclarg.h,v 1.7.2.1 2009/08/13 23:57:02 karthik Exp $
 */
 
/* Integrated CommandLine ARGument processing (i.e. getopt_long()) and usage(). */

#ifndef DAGCLARG_H
#define DAGCLARG_H

/* Endace headers. */
#include "dagutil.h"

/** \defgroup CLARG Command Line Arguments API.
 */
/*@{*/

typedef struct ClArg_ ClArg_;
typedef ClArg_* ClArgPtr; /* Opaque type-safe pointer. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Lifecycle routines. */
/** 
 * @brief       	allocates the header structure.and initialises it appropriately.
 * @param[in] argc      the number of command line arguments.
 * @param[in] argv      an array of pointers to the strings which are those arguments
 * @return              Pointer to ClArgHeader
 * 
 */

ClArgPtr dagclarg_init(int argc, const char* const * argv);
/** 
 * @brief       	disposes the header structure once the use of it is over.
 * @param[in] clarg     Pointer to ClArgHeader
 * @return              NULL
 */

ClArgPtr dagclarg_dispose(ClArgPtr clarg);
/** 
 * @brief       	    Adds a command line argument details for null argument (which does not take a value) to the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] description   description of the command line argument
 * @param[in] longopt       long option for the command line argument.
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */

/* Add an option. */
void dagclarg_add(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, uint16_t code);
/** 
 * @brief       	    Adds a command line argument details for char argument (which takes a char value) to the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] description   description of the command line argument
 * @param[in] longopt       long option for the command line argument.
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] argname       name of the argument.
 * @param[in] storage       storage for the char argument.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */

void dagclarg_add_char(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, uint8_t* storage, uint16_t code);
/** 
 * @brief       	    Adds a command line argument details for int argument (which takes an int value) to the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] description   description of the command line argument
 * @param[in] longopt       long option for the command line argument.
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] argname       name of the argument.
 * @param[in] storage       storage for the int32 argument.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */

void dagclarg_add_int(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, int32_t* storage, uint16_t code);
/** 
 * @brief       	    Adds a command line argument details for int 64 argument (which takes an int 64 value) to the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] description   description of the command line argument
 * @param[in] longopt       long option for the command line argument.
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] argname       name of the argument.
 * @param[in] storage       storage for the int64 argument. 
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */

void dagclarg_add_int64(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, int64_t* storage, uint16_t code);
/** 
 * @brief       	    Adds a command line argument details for unsigned uint32 argument (which takes an uint 32 value) to the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] description   description of the command line argument
 * @param[in] longopt       long option for the command line argument.
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] argname       name of the argument.
 * @param[in] storage       storage for the uint32 argument. 
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */
void dagclarg_add_uint(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, uint32_t* storage, uint16_t code);
/** 
 * @brief       	    Adds a command line argument details for unsigned string argument (which takes an string value) to the argumentlist.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] description   description of the command line argument
 * @param[in] longopt       long option for the command line argument.
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] argname       name of the argument.
 * @param[in] buffer        storage for the string argument.
 * @param[in] buflen        length of the buffer.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */
void dagclarg_add_string(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, char* buffer, uint32_t buflen, uint16_t code);

/** 
 * @brief       	    Adds a command line argument properties (short-option) to an existing member in the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] shortopt      short option for the command line argument.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */
void dagclarg_add_short_option(ClArgPtr clarg, uint16_t code, const char shortopt);
/** 
 * @brief       	    Adds a command line argument properties (long-option) to an existing member in the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] longopt       long option for the command line argument.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */
void dagclarg_add_long_option(ClArgPtr clarg, uint16_t code, const char* longopt);
/** 
 * @brief       	    Adds a command line argument properties (long-option) to an existing member in the argument list.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] line          description for the command line argument.
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */
void dagclarg_add_description(ClArgPtr clarg, uint16_t code, const char* line);
/** 
 * @brief       	    1 => display this in usage, 0 => don't display in usage.
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] code          code for the command line argument. 
 * @return                  none.
 */
void dagclarg_suppress_display(ClArgPtr clarg, uint16_t code);


/** 
 * @brief       	    Parse commandline arguments: 1 = valid, 0 = no more commands, -1 = error parsing option argument (ie missing) 
 * 			    If an unrecognised option is found, it is added to the unprocessed args array and the code
 * 			    argument to dagclarg_parse() will contain -1 on output.
 *
 * @param[in] clarg         Pointer to ClArgHeader
 * @param[in] errorfile     the error file where you want the output to be directed.(eg.stdout)
 * @param[in] code          code for the command line argument. 
 * @return                  Parse commandline arguments: 1 = valid, 0 = no more commands, -1 = error parsing option argument (ie missing).
 */
int dagclarg_parse(ClArgPtr clarg, FILE* errorfile, int* argindex, int* code);

/** 
 * @brief       	    	  Returns the the commandline arguments that were unrecognised.
 *
 * @param[in] clarg               Pointer to ClArgHeader
 * @param[in] unprocessed_argc    the error file where you want the output to be directed.(eg.stdout)
 * @return                        Returns an {argc, argv} pair containing the commandline arguments that were unrecognised.
 */
char** dagclarg_get_unprocessed_args(ClArgPtr clarg, int* unprocessed_argc);

/** 
 * @brief       	    	  Display autogenerated usage text.
 * @param[in] clarg               Pointer to ClArgHeader
 * @param[in] outfile             file to display the autogenerated text.
 * @return                        Returns an {argc, argv} pair containing the commandline arguments that were unrecognised.
 */
void dagclarg_display_usage(ClArgPtr clarg, FILE* outfile);

/** 
 * @brief       	    	  Reset the internal argument counters so one can use dagclarg_parse repeatedly from the beginning again 
 * @param[in] clarg               Pointer to ClArgHeader
 * @return                        Returns an {argc, argv} pair containing the commandline arguments that were unrecognised.
 */
void dagclarg_reset(ClArgPtr clarg);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAGCLARG_H */
