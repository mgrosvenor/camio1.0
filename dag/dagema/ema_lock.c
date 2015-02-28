/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: ema_lock.c,v 1.9 2009/06/23 06:55:23 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         ema_lock.c
* DESCRIPTION:  Implements the locking mechanism used for the EMA library.
*               Locking is used to ensure that only one instance of the
*               of the EMA has an open connection to a card at a time.
*
*               Because interprocess communication is quite different
*               on windows and linux most of these functions have two
*               versions, one for linux and one for windows.
*
*               The method used on Linux is to atomically create a lock
*               file for each card. The existance of the file indicates
*               that the ema is using the file. When the ema no longers
*               needs the connection to the card the file is closed.
*
*               The method used for Windows is to use a named mutex, one
*               for each card. If the mutex is locked then the library
*               is in use.
*
* HISTORY:
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"


/* Endace headers. */
#include "dagapi.h"
#include "dagutil.h"
#include "dagema.h"
#include "ema_types.h"





/**********************************************************************
 *
 * FUNCTION:     lock_ema
 *
 * DESCRIPTION:  Globally locks a copy of the embedded messaging API to the
 *               selected card. This means only one instance of the 
 *               library can open a connection to the card. There are two
 *               versions one for Windows and one for Linux.
 *
 * PARAMETERS:   ctx          IN/OUT     The EMA context associated with
 *                                       the card.
 *
 * RETURNS:      0 if the lock was successiful otherwise -1
 *
 **********************************************************************/

#if defined (__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

int
lock_ema (ema_context_t* ctx)
{
	int    fd,tmp;
	char   filename[128];
	char   str[12];
	pid_t  lock_pid;
	
	
	
	/* sanity check */
	if ( ctx == NULL )
		return -1;
		
	/* file name and path */
	snprintf (filename, 128, "/var/lock/LCK..dagema%d", ctx->dag_id);
	
	
	/* check if the file already exists ... if so read the PID information and
	 * determine if the app is still running.
	 */
	fd = open (filename, O_RDWR);
	if ( fd >= 0 )
	{
		if ( read(fd, str, 11) != 11 )
			unlink (filename);
		
		else
		{
			str[11] = '\0';
			lock_pid = atoi(str);
		
			if ( (kill(lock_pid, 0) == -1) && (errno == ESRCH) )
			{
				/* process has terminated, delete the lock file */
				unlink (filename);
			}
		}
		
		close (fd);
	}
	
	
	
	/* attempt to atomically create the locking file */
	fd = open (filename, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
	if ( fd < 0 )
		return -1;
	
	
	/* write the pid into the file */
	snprintf (str, 12, "%10d\n", (int) getpid());
	tmp=write (fd, str, 11);

		
	/* */
	ctx->lock_file = fd;
	return 0;
}

#else

int
lock_ema (ema_context_t* ctx)
{
	HANDLE	hMultex;
	CHAR    mut_name[128];

	/* sanity checking */
	if ( ctx == NULL )
		return -1;
	
	
	/* */
	snprintf (mut_name, 128, "dagema_%d", ctx->dag_id);
	
		
	/* create (or attach too) a mutex to control access to the shared memory */
	hMultex = CreateMutex (NULL, FALSE, mut_name);
	if ( hMultex == NULL )
	{
		EMA_PVERBOSE (1, "Error: CreateMutex failed\n");
		return -1;
	}
	
	/* determine if the mutex is in use or not, if not in use this function grabs it */
	if ( WaitForSingleObject(hMultex, 0) != WAIT_OBJECT_0 )
	{
		/* in use */
		return -1;
	}
	
	
	/* save the mutex handle for unlocking */
	ctx->hLockMutex = hMultex;
	
	return 0;
}

#endif






/**********************************************************************
 *
 * FUNCTION:     unlock_ema
 *
 * DESCRIPTION:  Unlocks the dag card EMA lock. There are two versions
 *               one for Linux and one for Windows, see the comment at
 *               the top of the file for the differences.
 *
 * PARAMETERS:   ctx          IN/OUT     The EMA context associated with
 *                                       the dag card.
 *
 * RETURNS:      0 if success otherwise -1
 *
 **********************************************************************/
#if defined (__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

int
unlock_ema (ema_context_t* ctx)
{
	char filename[128];
	
	/* sanity check */
	if ( ctx == NULL || ctx->lock_file < 0 )
		return -1;
	
	
	/* unlink the file so that when it is closed or the process terminates the file is deleted */
	snprintf (filename, 128, "/var/lock/LCK..dagema%d", ctx->dag_id);
	unlink (filename);


	/* close the file (which releases the lock) */
	close (ctx->lock_file);
	
	/* indicate the lock file is no longer in use */
	ctx->lock_file = -1;
	
	
	
	return 0;
}

#else

int
unlock_ema (ema_context_t* ctx)
{
	/* sanity check */
	if ( ctx == NULL || ctx->hLockMutex == NULL )
		return -1;

	
	/* check we have the mutex to release */
	if ( WaitForSingleObject(ctx->hLockMutex, 0) != WAIT_OBJECT_0 )
		return -1;

	/* release the mutex and close the handle */
	ReleaseMutex (ctx->hLockMutex);
	CloseHandle (ctx->hLockMutex);

	return 0;
}

#endif
