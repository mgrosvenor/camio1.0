/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ error handling
 *
 */


#ifndef CAMIO_ERRORS_H_
#define CAMIO_ERRORS_H_

#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>


#define CAMIO_ERR                    0x00
#define CAMIO_WARN                   0x01
#define CAMIO_DBG                    0x02

//REMEMBER to update camio_error_to_str as well.
#define CAMIO_ERR_NONE               0x00
#define CAMIO_ERR_NULL_PTR           0x01
#define CAMIO_ERR_INCOMPLETE_OPT     0x02
#define CAMIO_ERR_UNKNOWN_OPT        0x03
#define CAMIO_ERR_FILE_OPEN          0x04
#define CAMIO_ERR_FILE_READ          0x05
#define CAMIO_ERR_FILE_FLAGS         0x06
#define CAMIO_ERR_UNKNOWN_ISTREAM    0x07
#define CAMIO_ERR_UNKNOWN_OSTREAM    0x08
#define CAMIO_ERR_UNKNOWN_IOSTREAM   0x09
#define CAMIO_ERR_NOT_IMPL           0x0A
#define CAMIO_ERR_IOCTL              0x0B
#define CAMIO_ERR_SOCKET             0x0C
#define CAMIO_ERR_BIND               0x0D
#define CAMIO_ERR_RCV                0x0E
#define CAMIO_ERR_SEND               0x0F
#define CAMIO_ERR_FILE_LSEEK         0x10
#define CAMIO_ERR_MMAP               0x11
#define CAMIO_ERR_FILE_WRITE         0x12
#define CAMIO_ERR_SOCK_OPT           0x13
#define CAMIO_ERR_BUFFER_OVERRUN     0x14
#define CAMIO_ERR_STREAMS_OVERRUN    0x15
#define CAMIO_ERR_UNKNOWN_SELECTOR   0x16
#define CAMIO_ERR_UNKNOWN_CLOCK      0x17
#define CAMIO_ERR_NOT_AN_ERF         0x18
//REMEMBER to update camio_error_to_str as well.

void _eprintf_exit(int err_type, int error_no, int line_no, const char* file, const char *format, ...);
void eprintf_exit_simple(const char *format, ...);
#define eprintf_exit(err_no, format, args...) _eprintf_exit(CAMIO_ERR, err_no, __LINE__, __FILE__, format, ## args)
#define wprintf(err_no, format, args...) _eprintf_exit(CAMIO_WARN, err_no, __LINE__, __FILE__, format, ## args)
void veprintf_exit_simple(const char *format, va_list args);

#endif /* CAMIO_ERRORS_H_ */
