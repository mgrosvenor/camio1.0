/**********************************************************************
*
* Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: d37t_host_i2c.c,v 1.1 2007/01/15 03:48:55 cassandra Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         d37t_host_i2c.c
* DESCRIPTION:  This module implements the API for access to the I2C 
*               devices on the DAG3.7T.
*
* HISTORY:
*       09-03-05 DNH 1.0  Initial version.
*       02-12-05 BRG 1.1  Rewrote to use the dagema library
*
**********************************************************************/

#include <stdio.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/in.h>
#elif defined(_WIN32)

#include <signal.h>

#include <getopt.h>
#include <timeofday.h>
#include <regex.h>

#endif /* _WIN32 */

#include "dagapi.h"
#include "dagutil.h"
#include "dagema.h"

#include "d37t_i2c.h"



/**********************************************************************
*
* Constants
*
*/
#define I2C_MSG_TIMEOUT              10000

/*
 * Control Messages
 */
#define D37T_MSG_ERROR                         (EMA_CLASS_CONTROL | 0x0001)
#define D37T_MSG_SET_DEBUG                     (EMA_CLASS_CONTROL | 0x0003)
#define D37T_MSG_SET_DEBUG_CMPLT               (EMA_CLASS_CONTROL | 0x0004)
#define D37T_MSG_READ_VERSION                  (EMA_CLASS_CONTROL | 0x0005)
#define D37T_MSG_READ_VERSION_CMPLT            (EMA_CLASS_CONTROL | 0x0006)

#define D37T_MSG_ERROR_SIZE                     sizeof(t_d37t_msg_error)
#define D37T_MSG_SET_DEBUG_SIZE                 sizeof(t_d37t_msg_set_debug)
#define D37T_MSG_SET_DEBUG_CMPLT_SIZE           sizeof(t_d37t_msg_set_debug_cmplt)
#define D37T_MSG_READ_VERSION_SIZE              sizeof(t_d37t_msg_read_version)
#define D37T_MSG_READ_VERSION_CMPLT_SIZE        sizeof(t_d37t_msg_read_version_cmplt)

/*
 * I2C Messages
 */
#define D37T_MSG_READ_SOFTWARE_ID               (EMA_CLASS_I2C | 0x0001)
#define D37T_MSG_READ_SOFTWARE_ID_CMPLT         (EMA_CLASS_I2C | 0x0002)
#define D37T_MSG_WRITE_SOFTWARE_ID              (EMA_CLASS_I2C | 0x0003)
#define D37T_MSG_WRITE_SOFTWARE_ID_CMPLT        (EMA_CLASS_I2C | 0x0004)
#define D37T_MSG_READ_TEMPERATURE               (EMA_CLASS_I2C | 0x0005)
#define D37T_MSG_READ_TEMPERATURE_CMPLT         (EMA_CLASS_I2C | 0x0006)

#define D37T_MSG_READ_SOFTWARE_ID_SIZE          sizeof(t_d37t_msg_read_software_id)
#define D37T_MSG_READ_SOFTWARE_ID_CMPLT_SIZE    sizeof(t_d37t_msg_read_software_id_cmplt)
#define D37T_MSG_WRITE_SOFTWARE_ID_SIZE         sizeof(t_d37t_msg_write_software_id)
#define D37T_MSG_WRITE_SOFTWARE_ID_CMPLT_SIZE   sizeof(t_d37t_msg_write_software_id_cmplt)
#define D37T_MSG_READ_TEMPERATURE_SIZE          sizeof(t_d37t_msg_read_temperature)
#define D37T_MSG_READ_TEMPERATURE_CMPLT_SIZE    sizeof(t_d37t_msg_read_temperature_cmplt)

typedef struct {
    uint32_t length;		/* Number of bytes to read */
} t_d37t_msg_read_software_id;

typedef struct {
    uint32_t result;		/* Result of the operation */
    uint32_t length;		/* length of software ID (bytes) */
    uint8_t software_id[0];	/* Variable length software ID */
} t_d37t_msg_read_software_id_cmplt;

typedef struct {
    uint32_t debug;
} t_d37t_msg_set_debug;

typedef struct {
    uint32_t result;
} t_d37t_msg_set_debug_cmplt;


typedef struct {
    uint32_t length;		/* Number of bytes in the software ID */
    uint32_t key;		/* Security key to enable write access */
    uint8_t software_id[128];	/* Variable length software ID */
} t_d37t_msg_write_software_id;

typedef struct {
    int32_t result;
    uint32_t length;		/* Number of bytes in the software ID */
    uint32_t key;		/* Security key to enable write access */
    uint8_t software_id[128];	/* Variable length software ID */
} t_d37t_ioctl_write_software_id;

typedef struct {
    uint32_t result;		/* Result of the write */
} t_d37t_msg_write_software_id_cmplt;

typedef struct {
    uint32_t sensor_id;		/* Which sensor to read */
} t_d37t_msg_read_temperature;

typedef struct {
    uint32_t result;		/* Result of the operation */
    int32_t temperature;	/* The temperature read */
} t_d37t_msg_read_temperature_cmplt;


typedef struct {
    uint32_t request;
} t_d37t_msg_read_version;

typedef struct {
	uint32_t result;
    uint32_t version;
	uint32_t type;
} t_d37t_msg_read_version_cmplt;






/**********************************************************************
*
* Public interface
*
*/

/**********************************************************************
* FUNCTION:	d37t_set_debug(debug_flags)
* DESCRIPTION:	Enable/disable debugging flags on the board.
* INPUTS:	debug_flags	- 32-bit flag set
* RETURNS:	0	- success
*		-1	- operation failed.
**********************************************************************/
int d37t_set_debug(int dagfd, uint32_t debug_flags)
{
	t_d37t_msg_set_debug cmd;
	t_d37t_msg_set_debug_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      libema_opened;
	int      res;
    
	
	/* set the arguments */
    cmd.debug = dagema_htole32(debug_flags);
	
	
	/* open a connection to the ema library */
	if ( dagema_open_conn(dagfd) == 0 )
	{
		libema_opened = 1;
	}
	else
	{
		libema_opened = 0;
		
		/* check if we failed because the library is already open */
		res = dagema_get_last_error();
		if ( res != EEXIST)
		{
			/* reset the xscale and try communicating again */
			dagema_reset_processor(dagfd, 0);
			
			if ( dagema_open_conn(dagfd) == 0 )
				libema_opened = 1;
			else
				return -3;
		}
	}

	
	/* send the message */
    if (dagema_send_msg(dagfd, D37T_MSG_SET_DEBUG, D37T_MSG_SET_DEBUG_SIZE, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, I2C_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}
	if (msg_id != D37T_MSG_SET_DEBUG_CMPLT || msg_len != D37T_MSG_SET_DEBUG_CMPLT_SIZE)
	{
		res = -7;
		goto Exit;
	}
	

	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}





/**********************************************************************
* FUNCTION:     d37t_write_software_id(num_bytes, datap, key) 
* DESCRIPTION:  Store a new software ID.  If the key is incorrect the 
*               board will halt and a restart is required.
* INPUTS:       num_bytes   - number of bytes to read.  1-128.
*               datap       - byte array large enough to hold the ID.
*               key         - Security key to enable write mode.
* OUTPUTS:      none
* RETURNS:      0   - success
*               -1  - invalid num_bytes value
*               -2  - operational failure reading ID
*               -3  - Timeout communicating with board
**********************************************************************/
int d37t_write_software_id(int dagfd, int32_t num_bytes, uint8_t *datap, uint32_t key)
{
    t_d37t_msg_write_software_id cmd;
    t_d37t_msg_write_software_id_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res;
	int      libema_opened;

	
	/* santiy check */
	if ( num_bytes > 128 )
		return -1;
	
	
	/* open a connection to the ema library */
	if ( dagema_open_conn(dagfd) == 0 )
	{
		libema_opened = 1;
	}
	else
	{
		libema_opened = 0;
		
		/* check if we failed because the library is already open */
		res = dagema_get_last_error();
		if ( res != EEXIST)
		{
			/* reset the xscale and try communicating again */
			dagema_reset_processor(dagfd, 0);
			
			if ( dagema_open_conn(dagfd) == 0 )
				libema_opened = 1;
			else
				return -3;
		}
	}
	
	

	/* populate the message */
	msg_id  = D37T_MSG_WRITE_SOFTWARE_ID;
	msg_len = D37T_MSG_WRITE_SOFTWARE_ID_SIZE + num_bytes;

	cmd.key    = dagema_htole32(key);
	cmd.length = dagema_htole32(num_bytes);
	memcpy(&cmd.software_id[0], datap, num_bytes);

	
	/* send the request */	
    if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}

	
	/* read the response */
	msg_len = sizeof(reply);
    if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, I2C_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	if (msg_id != D37T_MSG_WRITE_SOFTWARE_ID_CMPLT)
	{
		res = -7;
		goto Exit;
	}

	
	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
	
}




/**********************************************************************
* FUNCTION:     d37t_read_software_id(num_bytes, datap) 
* DESCRIPTION:  Read the contents of the software ID.
* INPUTS:       num_bytes   - number of bytes to read.  1-128.
*               datap       - byte array large enough to hold the ID.
* OUTPUTS:      *datap      - populated with the Software ID.
* RETURNS:      0   - success
*               -1  - invalid num_bytes value
*               -2  - operational failure reading ID
*               -3  - timeout
**********************************************************************/
int d37t_read_software_id(int dagfd, int32_t num_bytes, uint8_t *datap) 
{
    t_d37t_msg_read_software_id cmd;
    t_d37t_msg_read_software_id_cmplt* reply;
	uint8_t  buffer[2048];
	uint32_t msg_len;
	uint32_t msg_id;
	int      res;
	int      libema_opened;
	
	
	/* sanity checking */
	if (num_bytes > 128)
		return -1;

	
	/* open a connection to the ema library */
	if ( dagema_open_conn(dagfd) == 0 )
	{
		libema_opened = 1;
	}
	else
	{
		libema_opened = 0;

		/* check if we failed because the library is already open */
		res = dagema_get_last_error();
		if ( res != EEXIST)
		{
			/* reset the xscale and try communicating again */
			dagema_reset_processor(dagfd, 0);
			
			if ( dagema_open_conn(dagfd) == 0 )
				libema_opened = 1;
			else
				return -3;
		}
	}
	
   
	/* set the message parameters */
    msg_id     = D37T_MSG_READ_SOFTWARE_ID;
    msg_len    = D37T_MSG_READ_SOFTWARE_ID_SIZE;
    cmd.length = dagema_htole32(num_bytes);

 	
	/* send the request */	
    if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}

	
	/* read the response */
	msg_len = 2048;
    if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, buffer, NULL, I2C_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	if (msg_id != D37T_MSG_READ_SOFTWARE_ID_CMPLT)
	{
		res = -7;
		goto Exit;
	}

	
	/* positive response */	
	reply = (t_d37t_msg_read_software_id_cmplt*) &(buffer[0]);
	res = dagema_le32toh(reply->result);
    if (res == EMA_ERROR_NONE)
	{
    	memcpy(datap, &reply->software_id[0], num_bytes);
    }
	
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}


/**********************************************************************
* FUNCTION:     d37t_read_temperature(sensor_id, int *temperature)
* DESCRIPTION:  Read the temperature from the specified sensor.
* INPUTS:       sensor_id      - ID of the sensor to read.  0 = default.
* OUTPUTS:      *temperature   - The current temperature reading in degrees Celcius.
* RETURNS:      0   - success
*               -1  - invalid sensor id
*               -2  - operational failure reading sensor
**********************************************************************/
int d37t_read_temperature(int dagfd, uint32_t sensor_id, int *temperature)
{
	t_d37t_msg_read_temperature cmd;
	t_d37t_msg_read_temperature_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res;
	int      libema_opened;

	
	
	/* sanity checking */
	if ( temperature == NULL )
		return -1;
	
		
 	/* open a connection to the ema library */
	if ( dagema_open_conn(dagfd) == 0 )
	{
		libema_opened = 1;
	}
	else
	{
		libema_opened = 0;
		
		/* check if we failed because the library is already open */
		res = dagema_get_last_error();
		if ( res != EEXIST)
		{
			/* reset the xscale and try communicating again */
			dagema_reset_processor(dagfd, 0);
			
			if ( dagema_open_conn(dagfd) == 0 )
				libema_opened = 1;
			else
				return -3;
		}
	}
	

	/* populate the arguments */    
	msg_id  = D37T_MSG_READ_TEMPERATURE;
    msg_len = D37T_MSG_READ_TEMPERATURE_SIZE;
    cmd.sensor_id = dagema_htole32(sensor_id);

	
	/* send the request */	
    if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}

	
	/* read the response */
	msg_len = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, I2C_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	if (msg_id != D37T_MSG_READ_TEMPERATURE_CMPLT || msg_len != D37T_MSG_READ_TEMPERATURE_CMPLT_SIZE)
	{
		res = -7;
		goto Exit;
	}

	
	/* process the response */
	res = dagema_le32toh(reply.result);
	if (res == EMA_ERROR_NONE)
	{
		*temperature = dagema_le32toh(reply.temperature);
	}


Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}


/**********************************************************************
* FUNCTION:	   d37t_read_version(*dagc, *version, *type)
* DESCRIPTION: Read the software type and version from the Xscale.
* INPUTS:      dagc - communication system information
* OUTPUTS:     *version - Version number of software currently written.
* OUTPUTS:     *type - Type of embedded software running 
* RETURNS:     0 - success
*              !0 - failure				
**********************************************************************/
int d37t_read_version(int dagfd, uint32_t *version, uint32_t *type)
{
	t_d37t_msg_read_version cmd;
	t_d37t_msg_read_version_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res;
	int      libema_opened;



 	/* open a connection to the ema library */
	if ( dagema_open_conn(dagfd) == 0 )
	{
		libema_opened = 1;
	}
	else
	{
		libema_opened = 0;
		
		/* check if we failed because the library is already open */
		res = dagema_get_last_error();
		if ( res != EEXIST)
		{
			/* reset the xscale and try communicating again */
			dagema_reset_processor(dagfd, 0);
			
			if ( dagema_open_conn(dagfd) == 0 )
				libema_opened = 1;
			else
				return -3;
		}
	}
	
	/* populate the arguments */    
    msg_id  = D37T_MSG_READ_VERSION;
    msg_len = D37T_MSG_READ_VERSION_SIZE;
	cmd.request = 0;

	
	/* send the request */	
    if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)(&cmd), NULL) != 0)
	{
		res = -6;
		goto Exit;
	}

	
	/* read the response */
	msg_len = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&reply), NULL, I2C_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	if (msg_id != D37T_MSG_READ_VERSION_CMPLT || msg_len !=D37T_MSG_READ_VERSION_CMPLT_SIZE)
	{
		res = -7;
		goto Exit;
	}
	
	
	/* process the response */
	res = dagema_le32toh(reply.result);
	if (res == EMA_ERROR_NONE)
	{
		if ( *version )
			*version = dagema_le32toh(reply.version);
		
		if ( *type )
			*type = dagema_le32toh(reply.type);
	}


Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}
