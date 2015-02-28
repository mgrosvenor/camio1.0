/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ error handling
 *
 */

#include "camio_errors.h"


const char* camio_error_to_str(uint64_t camio_err){
    switch(camio_err){
        case CAMIO_ERR_NONE:             return "no error";
        case CAMIO_ERR_NULL_PTR:         return "null pointer";
        case CAMIO_ERR_INCOMPLETE_OPT:   return "incomplete option";
        case CAMIO_ERR_UNKNOWN_OPT:      return "unknown option";
        case CAMIO_ERR_FILE_OPEN:        return "open file";
        case CAMIO_ERR_FILE_READ:        return "read file";
        case CAMIO_ERR_FILE_FLAGS:       return "file flags";
        case CAMIO_ERR_UNKNOWN_ISTREAM:  return "unknown istream";
        case CAMIO_ERR_UNKNOWN_OSTREAM:  return "unknown ostream";
        case CAMIO_ERR_UNKNOWN_IOSTREAM: return "unknown iostream";
        case CAMIO_ERR_NOT_IMPL:         return "not implemented";
        case CAMIO_ERR_IOCTL:            return "ioctl fail";
        case CAMIO_ERR_SOCKET:           return "socket fail";
        case CAMIO_ERR_BIND:             return "bind fail";
        case CAMIO_ERR_RCV:              return "receive fail";
        case CAMIO_ERR_SEND:             return "send fail";
        case CAMIO_ERR_FILE_LSEEK:       return "lseek file";
        case CAMIO_ERR_MMAP:             return "mmap fail";
        case CAMIO_ERR_FILE_WRITE:       return "write file";
        case CAMIO_ERR_SOCK_OPT:         return "set socket option";
        case CAMIO_ERR_BUFFER_OVERRUN:   return "buffer overrun";
        case CAMIO_ERR_STREAMS_OVERRUN:  return "streams overrun";
        case CAMIO_ERR_UNKNOWN_SELECTOR: return "unknown selector";
        case CAMIO_ERR_UNKNOWN_CLOCK:    return "unknown clock";
        case CAMIO_ERR_NOT_AN_ERF:       return "not an ERF";
        default:                        return "UNKNOWN ERROR CODE";
    }
}

void _eprintf_exit(int err_type, int error_no, int line_no, const char* file, const char *format, ...)
{
   va_list arg;

   //Figure out how long the error header string will be when we make it
   const char* err_txt = err_type == CAMIO_ERR ? "Error" : "Warning";
               err_txt = err_type == CAMIO_DBG ? "Debug" : "Warning";
   const char* err_str = "Fe2+ %s (0x%02X) [%s:%d] <%s>: ";
   int err_str_full_len = snprintf(NULL,0,err_str,err_txt,error_no, file, line_no, camio_error_to_str(error_no));
   //Make space for the error header string and the format string
   char* full_format = malloc(err_str_full_len + strlen(format));
   if(!full_format){
       //Catch the double fault
       fprintf(stderr, "Fe2+ Error (0x%02X) [%s:%d] <%s>: Could not allocate memory for error text! Panic!\n", CAMIO_ERR_NULL_PTR, __FILE__, __LINE__, camio_error_to_str(CAMIO_ERR_NULL_PTR));
       exit(-1);
   }
   sprintf(full_format,err_str,err_txt,error_no, file, line_no, camio_error_to_str(error_no));
   strcpy(full_format + err_str_full_len, format);

   va_start(arg, format);
   vfprintf (stderr, full_format,arg);
   va_end (arg);

   free(full_format);

   if(err_type == CAMIO_ERR)
       exit(-1);
}



void eprintf_exit_simple(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vfprintf (stderr, format,args);
   va_end (args);
   exit(-1);
}

void veprintf_exit_simple(const char *format, va_list args)
{
   vfprintf (stderr, format,args);
   exit(-1);
}


