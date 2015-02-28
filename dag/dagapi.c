//#LINKFLAGS=-lpthread

/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagapi.c,v 1.158.2.9 2009/11/24 02:11:51 alexey.korolev Exp $
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "dagtoken.h"
#include "dagapi.h"
#include "dagpbm.h"
#include "dagutil.h"
#include "dagopts.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <error.h>

/* C Standard Library headers. */
#include <ctype.h>
#include <errno.h>
#include <regex.h>

/* POSIX headers. */
#include <fcntl.h>
#include <netinet/in.h>    /* for ntohs() */
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>


/* Some notes:
Naming conevtions:
	'_bus_' Some of the variables will have this into the name 
		this represents the address of memory hole as seen by the 
		firmware in some cases it will be equivalent to physical
		address into the memory.
All addresses related to bus(physical addresses will be presented into 
 uint32_t or uint64_t this is because the firmware supports only 32 bit addresses 
 on both 32 bit and 64 bit system the uint64_t will be used for firmware which supports 64bits
 )
*/

/*****************************************************************************/
/* Macros and constants                                                      */
/*****************************************************************************/
#define ToHM2    (*(volatile uint32_t *)(herd[dagfd].iom+0x08))
#define ToHM4    (*(volatile uint32_t *)(herd[dagfd].iom+0x10))
#define ToAM2    (*(volatile uint32_t *)(herd[dagfd].iom+0x48))
#define ToAM3    (*(volatile uint32_t *)(herd[dagfd].iom+0x4c))
#define IOM(OFF) (*(volatile uint32_t *)(herd[dagfd].iom+(OFF)))

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

#define ARMOFFSET(FD)	({ int temp_off = ToHM4 - dag_info(FD)->phy_addr; \
					(temp_off == herd[FD].stream[0].size) ? 0 : temp_off; })

#define CUROFFSET(FD,STR)	({ int temp_off = (int) *(pbm->record_ptr) -  stream->bus_addr; \
					(temp_off == herd[FD].stream[STR].size) ? 0 : temp_off; })

#define SEGOFFSET(FD,STR)	({ int temp_off = ((dagpbm_MkI_t *)pbm)->segaddr - dag_info(FD)->phy_addr; \
					(temp_off == herd[FD].stream[STR].size) ? 0 : temp_off; })

#elif defined(_WIN32)

#define ARMOFFSET(FD)		armoffset(FD)
#define CUROFFSET(FD,STR)	curoffset(FD,STR,pbm)
#define SEGOFFSET(FD,STR)	segoffset(FD,STR,pbm)

#define my_ntohs(val)	((val) >> 8 | (val) << 8)

#else 
#error unsupported platform 
#endif /* Platform-specific code. */

#define PBMOFFSET(FD,STR)	(herd[FD].brokencuraddr ? \
					SEGOFFSET(FD,STR) : CUROFFSET(FD,STR))
#define ABS(X)		(((X)>0)?(X):-(X))

// SAFETY_WINDOW is the number of bytes we maintain of difference between
// the read pointer (curaddr) and the limit pointer (wrsafe) so this last
// one can never catch the first one. At the moment this safety window is
// used only in the receive streams, being not necessary in transmit
// streams.
#define SAFETY_WINDOW 8

/*
 * Long comment on why this is here and necessary:
 * There are a number of ambiguities with the wrsafe and curaddr pointers being
 * the same, in particular, understanding in any given situation whether the
 * buffer is empty/full from the PBM point of view, or meant to be emptied from
 * a users point of view. A number of fixes are possible, this one appears to be
 * the reliable path to address the problem, for the moment.
 */
#define WRSAFE(FD,STR,X)    (((X)<SAFETY_WINDOW) ? ((X)+herd[FD].stream[STR].size - SAFETY_WINDOW) : ((X)-SAFETY_WINDOW))

#define PBM_CONFIGURED(pbm) ( (( *((pbm)->mem_addr) & 0xfffffff0) != 0xfffffff0) && (*((pbm)->mem_addr) != 0x0) )

#define Int_Enables 0x8c
#define IW_Enable   0x10000

/* macro to check if inspection window is on */
#define armcode(X) ((herd[X].has_arm) && (*(int*)(herd[X].iom + Int_Enables) & IW_Enable))

#if defined(_WIN32)
#define MAX_OPEN_FILES	WIN_MAX_OPEN_FILES
#endif /* _WIN32 */

///*****************************************************************************/
///* Extern variables and functions                                            */
///*****************************************************************************/
//extern void dagopt_scan_string(const char * scan_string);
//extern int dagoptlex(void);
//extern char * dagopttext;
//extern int dagoptlval;

/*****************************************************************************/
/* Data structures                                                           */
/*****************************************************************************/
/*
 * Need to explain the file descriptor associative array.
 */

/* Every DAG card can handle several streams (aka memory holes)              */
typedef struct {
	//this might change to 64bits if we support bigger then 4GB streams 
	uint32_t           size;              /* Size of the stream's memory hole  */	
	
	uint32_t           offset;            /* Offset to current record          */
	uint32_t           processed;         /* Amount of data processed so far   */
	dagpbm_stream_t   *pbm;               /* Pointer to stream burst manager   */
	/* Note this type may be uint64_t for 64bit firmware 
	 it is not related to 64bit or 32bit systems (kernel or api)*/
	uint32_t           bus_addr;           /* Pointer to physical(bus) base address as seen by the firmware */
	
	uint32_t           mindata;
	struct timeval     maxwait;
	struct timespec    poll;
	uint32_t           extra_window_size; /* size of the second mmap         */
	int32_t            free_space;        /* free/used space                 */
	
	/* Note this type may be uint64_t for 64bit firmware 
	 it is not related to 64bit or 32bit systems*/
	uint32_t	last_bus_top;          /* last read value of record ptr   */
	uint32_t	 last_bus_bottom;       /* last written value of limit ptr */
	
	uint32_t	is_first_time:1;   /* is the stream's first call?     */
	uint32_t	attached:1;        /* is the stream attached?         */
	uint32_t	started:1;         /* is the stream started?          */
	uint8_t           *buf;               /* large buffer                      */
	
	uint8_t            pad_flush;         /* is partial block flushing needed? */
	uint32_t	reverse_mode;	/* sets the Reverse or normal mode working with Limit and the record pointer */
	uint32_t	paused:1;
	uint32_t	resync_required:1;
	uint32_t	resync_in_progress:1;
} stream_t;

/* The 'sheep' data structure stores the information related to one DAG card */
/* file descriptor. If you open one DAG card twice, you will have two sheeps */
typedef struct sheep {
#if defined(_WIN32)
	HANDLE      handle; /* Windows version doesn't rely on file descriptors
	                     * for sheep handling, so it needs to store the handle 
	                     * to the open device
	                     */
#endif
	char            dagname[32];    /* DAG card device name              */
	int             dagiom;         /* IOM file descriptor, cannot be 0  */
	stream_t        stream[DAG_STREAM_MAX]; /* Stream properties         */
	uint8_t         *iom;           /* IO memory pointer                 */
	daginf_t        daginf;
	uint32_t        brokencuraddr;  /* fix for ECMs and Dag4.1s          */
	uint32_t        byteswap;       /* endianness for 3.4/3.51ecm        */
	dag_reg_t       regs[DAG_REG_MAX_ENTRIES]; /* register enum cache    */
	dagpbm_global_t *pbm;           /* Pointer to global burst manager   */
	uint16_t         num_rx_streams; /* Number of receive streams         */
	uint16_t         num_tx_streams; /* Number of transmit streams        */
	uint32_t        has_arm:1;      /* indicates arm is present          */
	uint32_t        has_tx:1;
	uint32_t        implicit_detach:1;
	uint32_t        opened:1;
	uint32_t       *dsmu;           /* dsmu address for pad record forcing */
} sheep_t;

#if defined(_WIN32)

/* Info needed to allocate and map hole */
typedef struct {
	uint32_t num;
	uint32_t size;
	uint32_t offset;
	uint32_t extra_size;
	ULONG_PTR  memory_address;
} dag_stream_info_t;
#endif

/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/
static sheep_t *herd;          /* I was going to call this flock, initially */
/* Default behaviour is blocking for erf record hdr, no timeout              */
static uint32_t     dagapi_mindata = dag_record_size; /* wait for record */
static struct timeval   dagapi_maxwait = { 0, 0 };  /* timeout disabled      */
static struct timespec  dagapi_poll    = { 0, 1*1000*1000 };        /*  1 ms */

#if defined(__linux__)
static pthread_t softdag_sync_thread;
static uint32_t  softdag_sync_thread_running = 0;
#endif /* __linux__  */

/*****************************************************************************/
/* Private function declarations -> should go to a header file               */
/* Remember: static == private (when talking about functions)                */
/*****************************************************************************/
static int dag_update(int dagfd);

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
static int spawn(char *cmd, ...);
#endif

/*****************************************************************************/
/*** Functions                                                             ***/
/*****************************************************************************/
#if defined(_WIN32)
static int
armoffset(int dagfd)
{
    int temp_off = ToHM4 - dag_info(dagfd)->phy_addr;
    return (temp_off == herd[dagfd].stream[0].size) ? 0 : temp_off;
}

static int
segoffset(
    int dagfd,
    int stream_num,
    volatile dagpbm_stream_t *pbm)
{
    int temp_off =((dagpbm_MkI_t *)pbm)->segaddr - dag_info(dagfd)->phy_addr;
	return (temp_off == herd[dagfd].stream[stream_num].size) ? 0 : temp_off;
}

static int
curoffset(
    int dagfd,
    int stream_num,
    volatile dagpbm_stream_t *pbm)
{
    int temp_off = (int) *(pbm->record_ptr) -  (int)herd[dagfd].stream[stream_num].bus_addr;
	return (temp_off == herd[dagfd].stream[stream_num].size) ? 0 : temp_off;
}
#endif /* _WIN32 */


/*****************************************************************************/
/* Internal */
static char*
dagpath(char *path, char *temp, int tempsize) {
	
	if (!getenv("DAG"))
		return path;
	snprintf(temp, tempsize-1, "%s/%s", getenv("DAG"), path);
	return temp;
}

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
/*****************************************************************************/
/* Non windows dag_open and dag_close Implementation
*/
/*****************************************************************************/
int
dag_open(char *dagname)
{
	int	dagfd, i;
	daginf_t* dagfd_info;

	if((dagfd = open(dagname, O_RDWR)) < 0)
		goto fail;
	
#if 0 
	fprintf(stderr, "[dag_open] name: %s \n", dagname );
#endif 
	
	if(herd == NULL) {
		int herdsize = sysconf(_SC_OPEN_MAX) * sizeof(sheep_t);
		
		herd = (sheep_t *)dagutil_malloc_aligned(herdsize);		

		if(herd == NULL)
		{
			/* malloc() sets errno. */
			goto fail;
		}
		memset(herd, 0, herdsize);
		for( i = 0; i < sysconf(_SC_OPEN_MAX); i++)
			herd[i].dagiom = -1;
	}
	if(dagfd >= sysconf(_SC_OPEN_MAX)) {
		errno = ENFILE; /* XXX */
		fprintf(stderr, "[dag_open] internal error in %s line %u\n", __FILE__, __LINE__);
		goto fail;
	}
	/*
	 * Now fill in the herd structure
	 */
	strcpy(herd[dagfd].dagname, dagname);
	if((herd[dagfd].dagiom = dag_clone(dagfd, DAGMINOR_IOM)) < 0) {
		/* dag_clone() sets errno. */
		fprintf(stderr, "[dag_open] dag_clone dagfd for dagiom: %s\n", strerror(errno));
		goto fail;
	}

	/* Get info, we need to get the iom_size */
	if(ioctl(dagfd, DAGIOCINFO, &herd[dagfd].daginf) < 0)
	{
		fprintf(stderr, "[dag_open] ioctl DAGIOCINFO: fail\n");
		/* ioctl() sets errno. */
		goto fail;
	}
#if 0 	
	fprintf(stderr, "[dag_open] mmap fail\n");
#endif 	

	herd[dagfd].opened = 1;
	dagfd_info = dag_info(dagfd);
	if (NULL == dagfd_info)
	{
		fprintf(stderr, "[dag_open] dag_info fail\n");
		/* dag_info() sets errno. */
		goto fail;
	}
#if 0	
	fprintf(stderr, "[dag_open] size of daginf_t: %d\n", sizeof(daginf_t) ) ;
	fprintf(stderr, "dag_open IOM size: %d, dagiom 0x%x \n", dagfd_info->iom_size,herd[dagfd].dagiom) ;
#endif 
	if((herd[dagfd].iom = mmap(NULL, dagfd_info->iom_size, PROT_READ | PROT_WRITE,
					MAP_SHARED, herd[dagfd].dagiom, 0)) == MAP_FAILED)
	{
		fprintf(stderr, "[dag_open] mmap fail\n");
		/* mmap() sets errno. */
		goto fail;
	}

	/* Initialize (to NULL) pbm global data structure */
	herd[dagfd].pbm = NULL;

	/* This function not only updates daginf and regs, but also sets
	 * some global parameters.
	 */
	
	if(dag_update(dagfd)<0)
	{
		/* dag_update() sets errno. */
		fprintf(stderr, "[dag_open] dag_update failed: %s\n", strerror(errno));
		goto fail;
	}
	/* errno set appropriately */
	return dagfd;

fail:
	if (dagfd > 0)
	{
		/* Zero the entry for the failed dag file descriptor. */
		memset(&herd[dagfd], 0, sizeof(sheep_t));
	}
	return -1;
}

/*****************************************************************************/
int
dag_close(int dagfd)
{
	if(!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if (herd[dagfd].implicit_detach) {
		herd[dagfd].implicit_detach = 0;
		dag_detach_stream(dagfd, 0);
	}
	(void)close(herd[dagfd].dagiom);
	if (herd[dagfd].iom != NULL) {
		munmap(herd[dagfd].iom, dag_info(dagfd)->iom_size);
		herd[dagfd].iom = NULL;
	}
	dagutil_free_aligned(herd[dagfd].pbm);
	memset(&herd[dagfd], 0, sizeof(herd[dagfd]));
	herd[dagfd].dagiom = -1;
	return close(dagfd);
}

#elif defined(_WIN32)

/*
 * Windows open and close implementation
 * Expects dagname of the form dagN (e.g. dag0).
 */

int
dag_open(char *user_dagname)
{
	HANDLE	DagHandle;
	ULONG	BytesTransfered;
	int		dagfd, i;
	char *dagname = user_dagname;
	char dname[128];
	DWORD last_error;
	LPVOID lpMsgBuf;
	int herdsize;

	/* Construct a windows device name */
	if (dagname[0] == 'd') {
	    snprintf(dname, 128, "\\\\.\\%s", dagname);
	    dagname = dname;
	}

	DagHandle = CreateFile(
		dagname,			
		GENERIC_READ,		
		FILE_SHARE_READ,					
		NULL,				
		OPEN_EXISTING,		
		0,					
		0);

	if (DagHandle == INVALID_HANDLE_VALUE)  {
	    last_error = GetLastError();
	    if (!FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL )) {
		fprintf(stderr, "Arg: CreateFile failed, FormatMessage failed\n");
		return -1;
	    }
#ifndef NDEBUG
	    fprintf(stderr, "Failed to open dag %s\n", user_dagname);
	    fprintf(stderr, lpMsgBuf);
	    fprintf(stderr, "\n");
#endif
		errno = EACCES;

	    LocalFree(lpMsgBuf);
		return -1;	
	}
	
	EnterCriticalSection(&herdcs);

	if(herd == NULL)
	{

		herdsize = (MAX_OPEN_FILES * sizeof(sheep_t));
		
		herd = dagutil_malloc_aligned(herdsize);	
		if(herd == NULL)
		{
			fprintf(stderr, "herd is null\n");
			LeaveCriticalSection(&herdcs);
			return -1;	/* errno is ENOMEM */
		}
		memset(herd, 0, herdsize);
		
		for( i = 0 ; i < MAX_OPEN_FILES ; i++)
		{
			herd[i].dagiom = -1;
			herd[i].handle = INVALID_HANDLE_VALUE;
		}
	}

	/* Find a free sheep */
	for( i = 0 ; i < MAX_OPEN_FILES ; i++)
	{
		if(herd[i].handle == INVALID_HANDLE_VALUE)
		{
			herd[i].handle = DagHandle;
			break;
		}
	}

	dagfd = i;
		
	if(dagfd == MAX_OPEN_FILES) /* No free sheeps */
		panic("dagapi: internal error in %s line %u\n", __FILE__, __LINE__);
	
	LeaveCriticalSection(&herdcs);
	
	/*
 	 * Now fill in the herd structure
	 */

	/* Save the device name */
	strcpy(herd[dagfd].dagname, dagname);

	/* Get I/O memory */
	if(DeviceIoControl(DagHandle,
		IOCTL_GET_IO_MEM,
		NULL,
		0,
		&herd[dagfd].dagiom,
		sizeof(int),
		&BytesTransfered,
		NULL) == FALSE)
		panic("dag_open DeviceIoControl IOCTL_GET_IO_MEM\n");

	/* Get device info */
	if(DeviceIoControl(DagHandle,
		IOCTL_GET_DAGINFO,
		NULL,
		0,
		&herd[dagfd].daginf,
		sizeof(daginf_t),
		&BytesTransfered,
		NULL) == FALSE)
		panic("dag_open DeviceIoControl IOCTL_GET_DAGINFO\n");

	/* I/O memory was already mapped in our context by the driver, so simply copy the pointer */
	herd[dagfd].iom = (char*)herd[dagfd].dagiom;

	herd[dagfd].opened = 1;

	if(dag_update(dagfd) < 0)	/* update daginf and regs */
	{
		fprintf(stderr, "dag_update failed: %s\n", user_dagname);
		return -1;
	}

	return dagfd;	/* errno set appropriately */
}

int
dag_close(int dagfd)
{
	/* Close the handle */
	CloseHandle(herd[dagfd].handle);

	EnterCriticalSection(&herdcs);
	
	/* Reset the sheep */
	memset(&herd[dagfd], 0, sizeof(herd[dagfd]));
	herd[dagfd].handle = INVALID_HANDLE_VALUE;
	herd[dagfd].dagiom = -1;
	
	LeaveCriticalSection(&herdcs);

	return 0;
}
#endif /* _WIN32 */

/*****************************************************************************/
/* This is tentative, could be made an inline function.                      */
/* Problem is we don't want to make herd public.                             */
/*****************************************************************************/
daginf_t*
dag_info(int dagfd)
{
	if(!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	return &herd[dagfd].daginf;
}

/*****************************************************************************/
dag_reg_t*
dag_regs(int dagfd)
{
	if(!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	return herd[dagfd].regs;
}

/*****************************************************************************/
uint8_t*
dag_iom(int dagfd)
{
	if(!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	return herd[dagfd].iom;
}

/*****************************************************************************
 * dag_linktype
 *
 * Deprecated
 * Replaced by: dag_get_erf_types()
 *
 * Parameters: an open dag fd.
 * Returns: a single ERF type value, or 255 on error and sets errno.
 *
 * Originally written for libpcap support (now uses dag_get_erf_types).
 * Only checks the first 'gpp' or equivalent module found
 * Only capable of returning one ERF type
 * Will still not work for DAG 3.7T or 7.1S, which can capture in multiple formats
 * simultaneously.
 */
uint8_t
dag_linktype(int dagfd)
{
	volatile daggpp_t	*gpp;
	volatile uint32_t	*module;
	uint8_t		type = -1;
	dag_reg_t	result[DAG_REG_MAX_ENTRIES];
	uint32_t	regn=0;

	if(!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	/* Check if a DSM(/DSO/HMM) module is present */
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_HMM, result, &regn)) {
		errno = EIO;
		return -1;
	}

	if (regn) {
		/* found DSM */
		module = (volatile uint32_t*)(herd[dagfd].iom + DAG_REG_ADDR(*result));
		/* Check if ERF record reporting is present */
		if (*module &(1<<24)) {
			/* Read ERF type and return */
			type = (uint8_t)((*module>>16) & 0xff);
			return type;
		}
	}

	/* Check if a SR-GPP module is present */
	regn=0;
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_SRGPP, result, &regn)) {
		errno = EIO;
		return -1;
	}
	
	if (regn) {
		/* Check only first SR-GPP reg enum entry */
		module = (volatile uint32_t*)(herd[dagfd].iom + DAG_REG_ADDR(*result));
		
		/* Check only first SRGPP 'stream' i.e. physical interface */
		/* Check if ERF record reporting is present */
		if (!(*module &(1<<24))) {
			/* Read ERF type and add to list */
			type = (uint8_t)((*module>>16) & 0xff);
			return type;
		}
	}

	regn=0;
	if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_GPP, result, &regn))
	    || (regn < 1)) {
		errno = EIO;
		return -1;
	}

	/* Only check the first gpp */
	gpp = (volatile daggpp_t *)(herd[dagfd].iom + DAG_REG_ADDR(*result));

	/* gpp global value */
	type = (uint8_t)((gpp->control >> 16) & 0xff);
	if (type == 0xff)
		type = TYPE_ETH;

	return type;
}

/*****************************************************************************
 * dag_get_interface_count
 *
 * Parameters:
 * dagfd: an open dag fd
 *
 * Return: number of interfaces on the card
 */
uint8_t
dag_get_interface_count(int dagfd)
{
	volatile daggpp_t	*gpp;
	volatile uint32_t	*module;
	uint8_t		ifc_cnt = 0;
	dag_reg_t	result[DAG_REG_MAX_ENTRIES];
	uint32_t	regn=0;

	if(!herd[dagfd].opened)
	{
		errno = EBADF;
		return -1;
	}

	/* Check if a SR-GPP module is present */
	regn=0;
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_SRGPP, result, &regn))
	{
		errno = EIO;
		return -1;
	}
	
	if (regn)
	{
		/* Check only first SR-GPP reg enum entry */
		module = (volatile uint32_t*)(herd[dagfd].iom + DAG_REG_ADDR(*result));
		
		ifc_cnt = (uint8_t)((*module>>28) & 0x7);
		return ifc_cnt;

	}

	/* If no SR-GPP, check if a GPP module is present */
	regn=0;
	if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_GPP, result, &regn)) || (regn < 1))
	{
		errno = EIO;
		return -1;
	}

	/* Only check the first gpp */
	gpp = (volatile daggpp_t *)(herd[dagfd].iom + DAG_REG_ADDR(*result));

	/* gpp global value */
	ifc_cnt = (uint8_t)((gpp->control >> 6) & 0x3);
	ifc_cnt += 1;	/* Corresponds to the get_value method of the GPP component */

	return ifc_cnt;
}



/* Helper function for dag_get_erf_types() */
int dag_insert_into_list_uniquely(uint8_t *erfs, int size, int index, uint8_t value) {
	int count;
	
	for (count=0; count < index; count++) {
		if (erfs[count]==value)
			return index;
	}

	if (index < size) {
		erfs[index++] = value;
	}

	return index;
}

/*****************************************************************************
 * dag_get_erf_types
 *
 * Parameters:
 * dagfd: an open dag fd
 * erfs: an array of bytes supplied by the user.
 * size: must be at least TYPE_MAX, as this allows all ERF types to be
 * announced. Recommend users set size=255.
 *
 * Return: number of types detected or -1 on error and sets errno.
 * erfs is updated to contain the list of unique ERF types supported.
 */
int
dag_get_erf_types(int dagfd, uint8_t *erfs, int size)
{
	volatile daggpp_t	*gpp;
	volatile uint32_t	*module;
	uint8_t		type;
	dag_reg_t	result[DAG_REG_MAX_ENTRIES];
	uint32_t	regn=0;
	int		index=0;
	int		count, scount, streams;

	if(!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}	

	if (size < TYPE_MAX) {
		errno = EINVAL;
		return -1;
	}

	/* zero erfs array */
	memset(erfs, 0, size);

	/* Check if a DSM(/DSO/HMM) module is present */
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_HMM, result, &regn)) {
		errno = EIO;
		return -1;
	}

	if (regn) {
		/* found DSM */
		module = (volatile uint32_t*)(herd[dagfd].iom + DAG_REG_ADDR(*result));
		/* Check if ERF record reporting is present */
		if (*module &(1<<24)) {
			/* Read ERF type and return */
			type = (uint8_t)((*module>>16) & 0xff);
			erfs[index++] = type;
			return index;
		}
	}
	
	/* Check if a SR-GPP module is present */
	regn=0;
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_SRGPP, result, &regn)) {
		errno = EIO;
		return -1;
	}
	
	/* Loop over all available SR-GPP reg enum entries */
	for (count = 0; count < regn; count++) {
		module = (volatile uint32_t*)(herd[dagfd].iom + DAG_REG_ADDR(result[count]));

		/* read number of SRGPP 'streams' i.e. physical interfaces */
		streams = ((*module>>28) & 0x07);

		for(scount=0; scount<streams; scount++) {
			/* Check if ERF record reporting is present */
			if (!(module[scount*2] &(1<<24))) {
				/* Read ERF type and add to list */
				type = (uint8_t)((module[scount*2]>>16) & 0xff);
				index = dag_insert_into_list_uniquely(erfs, size, index, type);
			}
		}
	}
	
	/* If we have results return */
	if (index) return index;

	/* Check if a GPP module is present */
	regn=0;
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_GPP, result, &regn)) {
		errno = EIO;
		return -1;
	}
	
	for (count = 0; count < regn; count++) {
		/* Found GPP */
		gpp = (volatile daggpp_t *)(herd[dagfd].iom + DAG_REG_ADDR(result[count]));
		
		/* gpp global value */
		type = (uint8_t)((gpp->control >> 16) & 0xff);
		if (type == 0xff)
			type = TYPE_ETH;
		
		index = dag_insert_into_list_uniquely(erfs, size, index, type);
	}
	
	/* If we have results return */
	if (index) return index;

	/* These cases are for the DAG 3.7T */
	regn=0;
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_E1T1_HDLC_DEMAP, result, &regn)) {
		errno = EIO;
		return -1;
	}
	
	if (regn) {
		/* Only check the first demapper */
		switch(DAG_REG_VER(*result)) {
		case 0:
			erfs[index++] = TYPE_MC_HDLC;
			break;
		case 1:
			erfs[index++] = TYPE_MC_HDLC;
			erfs[index++] = TYPE_MC_RAW;
			break;
		case 2:
			erfs[index++] = TYPE_MC_HDLC;
			erfs[index++] = TYPE_MC_RAW;
			erfs[index++] = TYPE_MC_ATM;
			break;			
		case 3:
			erfs[index++] = TYPE_HDLC_POS;
			break;
		case 4:
			erfs[index++] = TYPE_ATM;
			erfs[index++] = TYPE_HDLC_POS;
			break;
		}
		return index;
	}

	regn=0;
	if (dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_E1T1_ATM_DEMAP, result, &regn)) {
		errno = EIO;
		return -1;
	}
	
	if (regn) {
		/* Only check the first demapper */
		switch(DAG_REG_VER(*result)) {
		case 0:
		case 1:
			erfs[index++] = TYPE_MC_ATM;
			break;
		case 2:
			erfs[index++] = TYPE_ATM;
			break;
		}
		return index;
	}

	return index;
}

/*
 * For now firmware doesn't report ERF types per stream, so just emulate.
 */
int
dag_get_stream_erf_types(int dagfd, int stream_num, uint8_t *erfs, int size)
{
	return dag_get_erf_types(dagfd, erfs, size);
}

/*
 * Setting transmit ERF types on a real card doesn't make sense, return error.
 */
int
dag_set_stream_erf_types(int dagfd, int stream_num, uint8_t *erfs)
{
	errno = EINVAL;
	return -1;
}

/*****************************************************************************/
int
dag_configure(int dagfd, char *params)
{
#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	int dagarm;

#elif defined(_WIN32)

	ULONG BytesTransfered, armoff;

#endif /* _WIN32 */

	int reset_gpp = 0;
	int tok;
	int setatm;
	int val, c;
	char temp1[80];
	char temp2[80];
	volatile daggpp_t * gpp;
	dag_reg_t result[DAG_REG_MAX_ENTRIES];
	uint32_t regn = 0;
	uint32_t phy_base;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	/*
	 * The SR-GPP, as used by the DAG 4.5G4 is not supported by this function.
	 */
	if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_SRGPP, result, &regn))
	    || (regn > 0)) {
#if 0
		fprintf(stderr, "dag_configure(): This card/firmware is not supported, use the Config & Status API instead\n");
#endif
		errno = EIO;
		return 0;
	}
	regn = 0;

	/*
	 * If no GPP, then exit immediately, but succeed.
	 * Assumes there is no important non-GPP configuration done here
	 */
	if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_GPP, result, &regn))
	    || (regn < 1)) {
		errno = EIO;
		return 0;
	}

	/*
	 * XXX Note that there is no locking on configuration in 2.5.x yet.
	 * This function currently only supports old non-2.5.x cards.
	 */

	/* 
	 * Parse input options
	 *
	 * Options are parsed before loading arm code since
	 * arm code loading depends on the options provided.
	 * The ncells options may enable armcode loading, and if so sets
	 * ToAM3 afterwards, since it is volatile on mailbox use.
	 *
	 * It is also possible auto-fpga loading may depend on options
	 * parsed here. If so, needs rearranging since reprogramming the
	 * pp fpga will void the gpp configurations set now.
	 */
	dagopt_scan_string(params);

	setatm = 1; /* default to ncells=1 */
	while((tok = dagoptlex()) != 0)
	{
		switch(tok)
		{
			case T_ERROR:
				fprintf(stderr, "unknown option '%s'\n", dagopttext);
				break;
				
			case T_POS:
				break;
				
			case T_ATM:
				break;
				
			case T_ATM_NCELLS:
				if (dagoptlval)
					*(int*)(herd[dagfd].iom + Int_Enables) |= IW_Enable;
				else
					*(int*)(herd[dagfd].iom + Int_Enables) &= ~IW_Enable;
					
				if (dagoptlval > 15)
				{
					fprintf(stderr, "ncells set too high (%d), max is 15\n",
						  dagoptlval);
					errno = ERANGE;
					return -1;
				}

				setatm &= ~0xffff;
				setatm |= dagoptlval;
				break;
				
			case T_ATM_LCELL:
				if (dagoptlval)
					setatm |= 0x20000;
				break;
				
			case T_GPP_SLEN:
				/* Setting ggp parameters here is deprecated,
				 * please use the config API instead.
				 * Doing our best anyway for backward compatibility
				 */
				for (c=0; c<regn; c++) {
					gpp = (volatile daggpp_t *)(herd[dagfd].iom + DAG_REG_ADDR(result[c]));
					gpp->snaplen = dagoptlval;
					reset_gpp = 1;
				}
				break;

			case T_GPP_VARLEN:
				for (c=0; c<regn; c++) {
					gpp = (volatile daggpp_t *)(herd[dagfd].iom + DAG_REG_ADDR(result[c]));
					if(dagoptlval)
						gpp->control |= 1;
					else
						gpp->control &= ~1;
					reset_gpp = 1;
				}
				break;

			default:
				/* silently ignore unhandled tokens */
				if (tok < T_MAX)
					break;

				/* fail on illegal tokens */
				fprintf(stderr, "unknown token %u in %s line %u\n", tok, __FILE__, __LINE__);
				errno = EINVAL;
				return -1;

		}
	}

	switch(dag_info(dagfd)->device_code)
	{
		case 0x3200:
			regn = 0;
			if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_ARM_PHY, result, &regn)) || (regn < 1))
			{
				errno = EIO;
				return -1;
			}
			
			/* Only check the first gpp */
			phy_base = DAG_REG_ADDR(*result);

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

			if( (dagarm=dag_clone(dagfd, DAGMINOR_ARM)) < 0)
			{
				return -1;
			}
			
			if(lseek(dagarm, (off_t)(phy_base+(0x5c<<2)), SEEK_SET) < 0)
			{
				fprintf(stderr, "dagioread lseek register 0x%x: %s\n", 0x5c, strerror(errno));
				return -1;
			}
			
			if(read(dagarm, &val, sizeof(val)) != sizeof(val))
			{
				fprintf(stderr, "read /dev/dagarmN register 0x%x: %s\n", 0x5c, strerror(errno));
				return -1;
			}
			
			close(dagarm);
			if (val & 0x80)
			{
				if(spawn(dagpath("tools/dagld", temp1, 80), "-d",
					 herd[dagfd].dagname,
					 "-r",
					 dagpath("arm/dag3pos-full.b",
						 temp2, 80), NULL) < 0)
					return -1;				
			}
			else
			{
				if(spawn(dagpath("tools/dagld", temp1, 80), "-d",
					 herd[dagfd].dagname,
					 "-r",
					 dagpath("arm/dag3atm-hash.b",
						 temp2, 80), NULL) < 0)
					return -1;
			}

#elif defined(_WIN32)

			armoff = phy_base+(0x5c<<2);
	
			if(DeviceIoControl(herd[dagfd].handle,
				IOCTL_ARM_READ,
				&armoff,
				sizeof(armoff),
				&val,
				sizeof(val),
				&BytesTransfered,
				NULL) == FALSE)
				panic("DeviceIoControl IOCTL_ARM_READ register 0x%x\n", 0x5c);
			
			if (val & 0x80) 
			{
				if(_spawnl(_P_WAIT, 
					dagpath("tools/dagld", temp1, 80),
					"-d",
					herd[dagfd].dagname,
					"-r",
					dagpath("arm/dag3pos-full.b", temp2, 80), 
					NULL) == 1)
					return -1;
			} 
			else 
			{
				if(_spawnl(_P_WAIT, 
					dagpath("tools/dagld", temp1, 80),
					"-d",
					herd[dagfd].dagname,
					"-r",
					dagpath("arm/dag3atm-hash.b", temp2, 80), 
					NULL) == 1)
					return -1;
			}
#else 
	#error Platform not supported 
#endif /* Platform-specific code. */

			break;
			
		case 0x3500:
			/*
			 * XXX might want to load recent images first
			 * XXX check if DUCK PPS present ?
			 */
			/*
			 * XXX we need to split params and implement an "ignore what
			 * you don't know" option in dagthree
			 */
		 
#if notyet
			if(execlp(dagpath("tools/dagthree", temp1, 80), "-d", herd[dagfd].dagname, params, NULL) < 0)
				return -1; /* errno set appropriately */
#endif

			if(armcode(dagfd))
			{

#if defined(__linux__)
				if(spawn(dagpath("tools/dagld", temp1, 80), "-d", herd[dagfd].dagname, "-r", dagpath("arm/dag3atm-erf.b", temp2, 80), NULL) < 0)
					return -1;
			
#elif defined(__FreeBSD__)

				if(spawn(dagpath("dagld", temp1, 80), "-d", herd[dagfd].dagname, "-r", dagpath("arm/dag3atm-erf.b", temp2, 80), NULL) < 0)
					return -1;

#elif defined(_WIN32)

				if(_spawnl(_P_WAIT, dagpath("tools/dagld", temp1, 80),
					"-d", herd[dagfd].dagname, "-r",
					dagpath("arm/dag3atm-erf.b", temp2, 80), NULL) == 1)
				{
					return -1;
				}
#else 
	//#warning Platform not supported  
#endif /* Platform-specific code. */
	
			}
			break;
		
		case 0x351c:
			/* byteswapped */
			herd[dagfd].byteswap = DAGPBM_BYTESWAP;
			break;
		
		case 0x3400:
		case 0x340e:
		case 0x341e:
			/* byteswapped and ipp */
			herd[dagfd].byteswap = DAGPBM_BYTESWAP;
			/* fallthrough */
			
		case 0x4100:
		case 0x4110:
			/* ipp */
			herd[dagfd].brokencuraddr++;
			break;
	}

	/* Now we are finished with loading, it's safe to set ARM parameters */
	if(armcode(dagfd))
	{
		ToAM3 = setatm;
	}
	
	if (1 == reset_gpp)
	{
		int val;
		
		/* L2 reset is necessary if GPP settings have been changed.
		 * This is to fix a bug reported by Jeremy 2006-03-22.
		 *
		 * XXX This reset should be inside dag_lockallstreams()
		 * side effects could break other capturing sessions */
		val = *(volatile uint32_t*) (herd[dagfd].iom + 0x88);
		val |= 0x80000000;
		*(volatile uint32_t*)(herd[dagfd].iom + 0x88) = val;
	}

	return 0;
}

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
/*****************************************************************************/
/* Internal */
static int
spawn(char *cmd, ...)
{
	va_list ap;
	int i, pid, status;
	char* args[16]; /* arbitrarily small */

	switch(pid = fork()) {
	  case -1:	/* error */
		return -1;	/* errno set appropriately */
	  case 0:	/* child */
		va_start(ap, cmd);
		args[0] = cmd;
		for( i = 1 ; i < 15 ; i++)
			if((args[i] = va_arg(ap, char *)) == NULL)
				break;
		args[i] = NULL;
		for( i = 0 ; i < 15 ; i++ ) {
			if(args[i] == NULL) {
				break;
			}
		}

		va_end(ap);
		execvp(cmd, args);
		fprintf(stderr, "execvp %s failed: %s\n", cmd, strerror(errno));
		return -1;
	  default:	/* parent */
		if(wait(&status) != pid)
			return -1;
		if(!WIFEXITED(status))
			return -1;
		if(WEXITSTATUS(status) != 0)
			return -1;
		break;
	}
	return 0;
}

#endif /* Platform-specific code. */

#if defined (__linux__)
/*****************************************************************************
 * softdag_sync_thread_fn
 *
 * Description: used to clear DAGPBM_SYNCL2R bit for the reverse streams
 *              also initialize limit pointer and other per-stream variables. 
 * Parameters:
 * context not used
 *
 * Returns:
 */
static void * 
softdag_sync_thread_fn(void *context)
{
	stream_t * stream;
	dagpbm_stream_t * pbm;
	uint32_t dagfd, stream_num;

	do {
		for(dagfd = 0; dagfd < sysconf(_SC_OPEN_MAX); dagfd++)
			for(stream_num = 0; stream_num < DAG_STREAM_MAX; stream_num++)
			{
				stream = &(herd[dagfd].stream[stream_num]);
				
				if((stream->reverse_mode != DAG_REVERSE_MODE) || (!stream->attached) || (!stream->resync_required))
					continue;

				pbm = stream->pbm;
				if(*(pbm->status) & DAGPBM_PAUSED)
				{
					stream->paused = 1;
				}
				if(*(pbm->status) & DAGPBM_SYNCL2R)
				{
					stream->resync_in_progress = 1;
					if(stream_num & 1)
					{
						if((stream->paused) && (*(pbm->limit_ptr))) 
						{
							/* get current record pointer, it may be record aligned or not */
							/* 
							 * But we need to calculate appropriate limit_ptr for dag_advance_stream to work correctly 
							 * with *bottom = NULL passed as function argument when dag_advance_stream caleed for the 
							 * the first time
							 */
							stream->last_bus_bottom = *(pbm->record_ptr) - SAFETY_WINDOW;
							if(stream->last_bus_bottom < *(pbm->mem_addr))
							{
								stream->last_bus_bottom += *(pbm->mem_size);
							}
							*(pbm->limit_ptr) = stream->last_bus_bottom;
							/* We need to initialize offset differently because valid data will start from the current record_ptr, not from 0 */
							stream->offset = (*(pbm->record_ptr) - *(pbm->mem_addr));
						}
						else
						{
							stream->last_bus_bottom = *(pbm->mem_addr) + *(pbm->mem_size) - SAFETY_WINDOW;
							*(pbm->limit_ptr) = stream->last_bus_bottom;
							stream->offset = (*(pbm->limit_ptr) - *(pbm->mem_addr));
						}
					}
					else
					{
						/* Receive stream */
						stream->last_bus_bottom = *(pbm->mem_addr); /* Initialise limit pointer to hole base */
						*(pbm->limit_ptr) = stream->last_bus_bottom;
						stream->offset = (*(pbm->limit_ptr) - *(pbm->mem_addr));
					}
					stream->paused = 0;
					stream->started = 1;

					*(pbm->status) = DAGPBM_UNPAUSED; /* if we will set PAUSED then it will set paused = 1 in the next loop */
					stream->resync_in_progress = 0;
				}
			}
		usleep(20 * 1000); /* 50 Hz */
	} while(1);

	return context;
}


/*****************************************************************************
 *
 * Description: 
 * Parameters:
 *
 * Returns:
 */
static int 
dag_set_stream_resync_flag(int dagfd, int stream_num, int paused)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	int result;

	stream->resync_required = 1;
	stream->paused = paused;
	
	if(!softdag_sync_thread_running)
	{
		softdag_sync_thread_running = 1;
		result = pthread_create(&softdag_sync_thread, NULL, softdag_sync_thread_fn, NULL);

		if (0 != result)
		{
			softdag_sync_thread_running = 0;
			return -1;
		}
	}

	return 0;
}


/*****************************************************************************
 *
 * Description: 
 * Parameters:
 *
 * Returns:
 */
static int 
dag_clear_stream_resync_flag(int dagfd, int stream_num)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	uint32_t counter;

	counter = 1000;
	stream->resync_required = 0;
	while(stream->resync_in_progress && counter--)
	{
		usleep(1000);
	}

	stream->paused = 0;

	return 0;
}


/*
 * note this functio is used in reverse 
 * and must be called instead of dag_start_stream only if TX to RX stream and RX from TX stream
 * it is used in rverese 
 * it is not required function unless the customer has to create DAEMON which talks to soft dag in reverse orderd 
 */
int 
dag_start_stream_softdag(int dagfd, int stream_num, int timeout)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	dagpbm_stream_t * pbm = stream->pbm;
	
	int is_timeouted = 0;

	if(!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	if(stream->started) {
		errno = EBUSY;
		return -1;
	}

	if(stream->paused) {
		errno = EBUSY;
		return -1;
	}
	stream->paused = 1;
	stream->resync_required = 0;

	if (stream_num & 1) {
		/* Transmit stream */

		/* Check if stream number is within available streams */
		if (herd[dagfd].num_tx_streams <= ((stream_num - 1) / 2)) {
			errno = EINVAL;
			return -1;
		}

	} else {
		/* Receive stream */

		/* Check if stream number is within available streams */
		if (herd[dagfd].num_rx_streams <= ((stream_num) / 2)) {
			errno = EINVAL;
			return -1;
		}
		/* not needed done by the use application */ 
		/* memset(herd[dagfd].stream[stream_num].buf, 0, herd[dagfd].stream[stream_num].size); */

	}

	/* in case daginf has changed */
	if (dag_update(dagfd) < 0)
		return -1;
	
    stream->offset = 0;	
	if(pbm) 
	{
		/* wait for the User application to set BM reset - this is normal 
		 * wait for a L2 (DP reset to arrive ) 
		 * There is no need for time out may be just for some extended functionality 
		 */
		for( ; !(*(pbm->status) & DAGPBM_SYNCL2R); timeout-- )
		{

			if( timeout <= 0 )
			{
				/* timed out when polling for stream reset */
				is_timeouted = 1;
				if(stream->started)
				{
					errno = EBUSY;
					return -1;
				}
				break;
			}
#ifndef NDEBUG
			fprintf(stderr,"wait for customer(consumer)\n");
#endif
			usleep(10000); /* 10ms, smallest reliable sleep */
		}
#ifndef NDEBUG
		fprintf(stderr,"initbase %x, record %x, limit %x \n",*(pbm->mem_addr),*(pbm->limit_ptr), *(pbm->record_ptr) );
#endif

		if (stream_num & 1)
		{
			/* transmit stream */
			if (is_timeouted) 
			{
				if(*(pbm->limit_ptr))
				{
					/* restarting from the current position, stream may not aligned to record. */
					/* get current record pointer, it may be record aligned or not */
					/* 
					 * But we need to calculate appropriate limit_ptr for dag_advance_stream to work correctly 
					 * with *bottom = NULL passed as function argument when dag_advance_stream called for the 
					 * the first time
					 */
					stream->last_bus_bottom = *(pbm->record_ptr) - SAFETY_WINDOW;
					if(stream->last_bus_bottom < *(pbm->mem_addr))
					{
						stream->last_bus_bottom += *(pbm->mem_size);
					}
					*(pbm->limit_ptr) = stream->last_bus_bottom;
					/* We need to initialize offset differently because valid data will start from the current record_ptr, not from 0 */
					stream->offset = (*(pbm->record_ptr) - *(pbm->mem_addr));
					is_timeouted = 0;
				}
				else
				{
				/* fprintf(stderr,"dag_start_stream_softdag: clean run : let sync thread do the job, is_timeouted = %d\n", is_timeouted); */
				}
			}
			else
			{
				 if(!(*(pbm->record_ptr)))
				{
					/* 
					 *  there is no client connected to the other end of stream 
					 *  so leave it as is and the sync thread will do the job. 
					 */
					is_timeouted = 1;
				}
				else
				{
					/* started from the start of the buffer */
					stream->last_bus_bottom = *(pbm->mem_addr) + *(pbm->mem_size) - SAFETY_WINDOW;
					*(pbm->limit_ptr) = stream->last_bus_bottom;
				}
			}
		}
		else
		{
			/* Receive stream */
			stream->last_bus_bottom = *(pbm->mem_addr); /* Initialise limit pointer to hole base */
			*(pbm->limit_ptr) = stream->last_bus_bottom;
		}


		*(pbm->status) = DAGPBM_UNPAUSED;	/* set the pause flag */ 

		if(dag_set_stream_resync_flag(dagfd, stream_num, is_timeouted)) 
		{
			errno = ENOMEM;
			return -2;
		}
		if(!stream->offset) 
		{
			stream->offset = (*(pbm->limit_ptr) - *(pbm->mem_addr));
		}
	}

	if(is_timeouted)
		stream->started = 0;
	else
		stream->started = 1;

	return 0;
}
#endif /* __linux__ */

/*****************************************************************************/
int
dag_start_stream(int dagfd, int stream_num)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	dagpbm_stream_t * pbm = stream->pbm;
	dagpbm_global_t * pbm_global = herd[dagfd].pbm;
	dag_reg_t	result[DAG_REG_MAX_ENTRIES];
	uint32_t	regn=0;
	int timeout;

	if (herd[dagfd].stream[stream_num].reverse_mode) {
#if defined(__linux__)
		/* timeout in 10ms units. 10000*10ms = 100s */
		return dag_start_stream_softdag(dagfd, stream_num, 100);
#else
		errno = EBADF;
		return -1;
#endif /* __linux__ */
	}
	
	if(!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	if(stream->started) {
		errno = EBUSY;
		return -1;
	}
	
	if (stream_num & 1) {
		// Transmit stream

		// Check if stream number is within available streams
		if (herd[dagfd].num_tx_streams <= ((stream_num - 1) / 2)) {
			errno = EINVAL;
			return -1;
		}

		if(pbm) {
			//FIXME:
			*(pbm->status) = DAGPBM_PAUSED ;
			usleep(10);
			//*(pbm->status) = (DAGPBM_PAUSED | DAGPBM_SAFETY);
			//while (*(pbm->status) & DAGPBM_REQPEND)
			//	usleep(1);
			
			
			stream->last_bus_bottom = *(pbm->mem_addr); // Initialise limit pointer to hole base
			*(pbm->limit_ptr) = stream->last_bus_bottom;
			*(pbm->status) = (DAGPBM_SYNCL2R | DAGPBM_PAUSED);
			//wait for the card to get out of BM reset what ever version basicly MK_2 and MK_3
			for( timeout = 0; *(pbm->status) & DAGPBM_SYNCL2R; timeout++ ) {
				if( timeout > 4000 ) {
					/* timed out when polling for stream reset */
					errno = ETIMEDOUT;
					return -1;
				}
				
				usleep(1000);
			}
			
			*(pbm->status) = DAGPBM_UNPAUSED;	// clear paused flag
		}
	} else {
		// Receive stream

		// Check if stream number is within available streams
		if (herd[dagfd].num_rx_streams <= ((stream_num) / 2)) {
			errno = EINVAL;
			return -1;
		}

		memset(herd[dagfd].stream[stream_num].buf, 0, herd[dagfd].stream[stream_num].size);

		/* in case daginf has changed */
		if (dag_update(dagfd) < 0)
			return -1;

		/* Can we start this card? */
		if ((!pbm) && (!armcode(dagfd))) {
			errno = ENODEV; /* need to say something */
			return -1;
		}

		/*
		* XXX make sure the DUCK is in sync, if needed
		*/
		if(pbm) {
			*(pbm->status) = (DAGPBM_PAUSED | DAGPBM_SAFETY);
			while (*(pbm->status) & DAGPBM_REQPEND)
				usleep(1);

			*(pbm_global->burst_timeout) = 0xffff;			
			/* Init the limit pointer and software local copy for the stream*/
			stream->last_bus_bottom = *(pbm->mem_addr) + *(pbm->mem_size) - SAFETY_WINDOW;
			*(pbm->limit_ptr) = stream->last_bus_bottom;
			*(pbm->status) = DAGPBM_SYNCL2R;

			for( timeout = 0; *(pbm->status) & DAGPBM_SYNCL2R; timeout++ ) {
				if( timeout > 1000 ) {
					/* timed out when polling for stream reset */
					errno = ETIMEDOUT;
					return -1;
				}
				
				usleep(1000);
			}

			*(pbm->status) = (DAGPBM_AUTOWRAP|herd[dagfd].byteswap);
		}

		if(armcode(dagfd)) {
			ToAM2 = 0;			/* clear command register */

			while(ToHM2 != 1) {
				usleep(1);
				if(ToHM2 == 2)
					break;		/* protocol bug */
			}

			ToAM2 = 1;			/* command: run */

			while(ToHM2 != 2)
				usleep(1);
		}

		/* set up the packet flush mechanism if relevant */
		if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_DSMU, result, &regn) == 0)
		    && (regn > 0)) {
			    //CHECKME: is DAG_REGADDR will return address as pointer as int
			herd[dagfd].dsmu = (uint32_t * ) (herd[dagfd].iom + DAG_REG_ADDR(*result));
			herd[dagfd].stream[stream_num].pad_flush = 1;
		}
		else
		{
			herd[dagfd].dsmu = NULL;
		}

	}

	stream->started = 1;

	return 0;
}

/*****************************************************************************/
int
dag_stop_stream(int dagfd, int stream_num)
{
	stream_t *stream = &(herd[dagfd].stream[stream_num]);
	dagpbm_stream_t *pbm = stream->pbm;
	int loop = 100;
	int retval = 0;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	if(!stream->started) {
		errno = EINVAL;
		return -1;
	}

	if (stream_num & 1) {
		// Transmit stream

		// Check if stream number is within available streams
		if (herd[dagfd].num_tx_streams <= ((stream_num - 1) / 2)) {
			errno = EINVAL;
			return -1;
		}

		if(pbm) {
			*(pbm->status) = DAGPBM_PAUSED;
		}
	} else {
		// Receive stream

		// Check if stream number is within available streams
		if (herd[dagfd].num_rx_streams <= ((stream_num) / 2)) {
			errno = EINVAL;
			return -1;
		}

		/* Can we stop this card? */
		if ((!pbm) && (!armcode(dagfd))) {
			errno = ENODEV;		/* need to say something */
			return -1;
		}

		if(armcode(dagfd)) {
			ToAM2 = 2;			/* stop command */

			while(--loop > 0) {
				usleep(10*1000); /* give ARM a chance to settle */
				if(ToHM2 == 3)
					break;
			}
			retval = (ToHM2 == 3) ? 0 : -1;	/* XXX need to set errno */
		}
		if(pbm) {
			*(pbm->status) = (DAGPBM_PAUSED);

			while(--loop > 0) {
				if(!(*(pbm->status) & DAGPBM_REQPEND))
					break;
				usleep(10*1000);
			}
			retval += (*(pbm->status) & DAGPBM_REQPEND) ? -1 : 0;
		}
	}

#if defined(__linux__)
	if(stream->reverse_mode == DAG_REVERSE_MODE)
		dag_clear_stream_resync_flag(dagfd, stream_num);
#endif /* __linux__ */

	stream->started = 0;

	return retval;
}

/*****************************************************************************/
int
dag_get_stream_buffer_size(int dagfd, int stream_num)
{
	stream_t	*stream = &(herd[dagfd].stream[stream_num]);
	dagpbm_stream_t	*pbm = stream->pbm;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(pbm) {
		return *(pbm->mem_size);
	}

	return dag_info(dagfd)->buf_size;
}
/******************************************************************************/
void *
dag_get_stream_buffer_virt_base_address(int dagfd, int stream_num)
{
	stream_t        *stream = &(herd[dagfd].stream[stream_num]);
	//dagpbm_stream_t *pbm = stream->pbm;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}
	/*Verify to check if teh stream is attached.*/
	if(!stream->attached) {
		  errno = EINVAL;
		  return NULL;
		 }
						          
	
	return stream->buf;
}
/*******************************************************************************/
int
dag_get_stream_phy_buffer_address(int dagfd, int stream_num)
{
	
	stream_t        *stream = &(herd[dagfd].stream[stream_num]);
	dagpbm_stream_t *pbm = stream->pbm;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}
	/* to verification if the stream is attached */
	if(!stream->attached){
		errno = EINVAL;
		return -1;
	}
	if(pbm) {
		return *(pbm->mem_addr);
	}
				
	return dag_info(dagfd)->phy_addr;
}
/*****************************************************************************/
int
dag_rx_get_stream_count(int dagfd)
{
	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	return herd[dagfd].num_rx_streams;
}

/*****************************************************************************/
int
dag_tx_get_stream_count(int dagfd)
{
	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	return herd[dagfd].num_tx_streams;
}

/*****************************************************************************/
int
dag_get_stream_poll(int dagfd, int stream_num, uint32_t *mindata,
     struct timeval *maxwait, struct timeval *poll)
{
	stream_t	*stream = &(herd[dagfd].stream[stream_num]);

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	*mindata = stream->mindata;
	*maxwait = stream->maxwait;
	TIMESPEC_TO_TIMEVAL(poll, &stream->poll);

	return 0;
}

/*****************************************************************************/
int
dag_set_stream_poll(int dagfd, int stream_num, uint32_t mindata,
     struct timeval *maxwait, struct timeval *poll)
{
	stream_t	*stream = &(herd[dagfd].stream[stream_num]);

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	stream->mindata = mindata;
	stream->maxwait = *maxwait;
	TIMEVAL_TO_TIMESPEC(poll, &stream->poll);

	return 0;
}

/*****************************************************************************/
/* Deprecated */
void
dag_getpollparams(int *mindatap, struct timeval *maxwait, struct timeval *poll)
{
	*mindatap = dagapi_mindata;
	*maxwait  = dagapi_maxwait;
	TIMESPEC_TO_TIMEVAL(poll, &dagapi_poll);
}

/*****************************************************************************/
/* Deprecated */
void
dag_setpollparams(int mindata, struct timeval *maxwait, struct timeval *poll)
{
	int dagfd;
	int count = 0;

#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	count = sysconf(_SC_OPEN_MAX);

#elif defined(_WIN32)

	count = MAX_OPEN_FILES;

#endif /* Platform-specific code. */

	dagapi_mindata = mindata;
	dagapi_maxwait = *maxwait;
	TIMEVAL_TO_TIMESPEC(poll, &dagapi_poll);

	/* change all active read streams to new global settings */
	for (dagfd = 0; dagfd < count; dagfd++) {
		if (herd[dagfd].iom != NULL) {
			herd[dagfd].stream[0].mindata = dagapi_mindata;
			herd[dagfd].stream[0].maxwait = dagapi_maxwait;
			TIMEVAL_TO_TIMESPEC(poll, &herd[dagfd].stream[0].poll);
		}
	}
}

/*****************************************************************************/
/* Deprecated */
int
dag_start (int dagfd) {
	return dag_start_stream (dagfd, 0);
}

/*****************************************************************************/
/* Deprecated */
int
dag_stop (int dagfd) {
	return dag_stop_stream (dagfd, 0);
}

/*****************************************************************************/
int
dag_detach_stream(int dagfd, int stream_num)
{
	int error = 0, lock, curaddr;
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	volatile dagpbm_stream_t * pbm = stream->pbm;

#if defined(_WIN32)
	ULONG BytesTransfered;
#endif /* _WIN32 */


	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	// Check if stream number is within DAG capabilities
	if (stream_num & 1) {
		// This is a transmit stream
		if (herd[dagfd].num_tx_streams <= ((stream_num - 1) / 2)) {
			errno = EINVAL;
			return (intptr_t) MAP_FAILED;
		}
	} else {
		// This is a receive stream
		if (herd[dagfd].num_rx_streams <= (stream_num / 2)) {
			errno = EINVAL;
			return (intptr_t) MAP_FAILED;
		}
	}

	if (herd[dagfd].stream[stream_num].buf != NULL) {
		if(pbm) {
			/* wait for active tx stream to complete */
			if ((stream_num&1) && !(*(pbm->status)&DAGPBM_PAUSED)) {
				curaddr = *(pbm->record_ptr);
				while (!(*(pbm->status)&DAGPBM_SAFETY)) {
					usleep( 1000 );
					/* break loop if tx stalls */
					if (*(pbm->record_ptr) == curaddr) {
						break;
					} else {
						curaddr = *(pbm->record_ptr);
					}
				}
				*(pbm->status) = DAGPBM_PAUSED;
			}

			// Free space of pbm structure
			dagutil_free_aligned(stream->pbm);
		}

#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

		munmap(stream->buf, stream->size + stream->extra_window_size);

#elif defined(_WIN32)

		/* XXX what is appropriate here? Do we need to "unmap"? */
#else 
#error Platform not supported 
#endif /* _WIN32 */
	}

	// Unlock stream
	lock = (stream_num << 16) | 0; /* unlock */
	if(herd[dagfd].stream[stream_num].reverse_mode == DAG_REVERSE_MODE)
		lock |= (1 << 8);
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	if (ioctl(dagfd, DAGIOCLOCK, &lock) < 0) {
	    error = -1;		/* errno set accordingly */
		
	}

#elif defined(_WIN32)

	if (DeviceIoControl(herd[dagfd].handle,
		IOCTL_LOCK,
		&lock,
		sizeof(lock),
		NULL,
		0,
		&BytesTransfered,
		NULL) == FALSE) {
	    error = -1;		/* errno set accordingly */
	}
#else 
	#define Platform not supported 
#endif /* Platform-specific code. */

	stream->started = 0;
	stream->attached = 0;

	return error;
}
/*****************************************************************************/
/*added a new interface for read only mapping of the memory streams.The old interf
ace dag_attach_stream will map the streams with read and write access.the new in
terface dag_attach_stream_protection()takes a parameter protection which you hav
e to specify as PROT_READ to map the stream as read-only.*/


int
dag_attach_stream(int dagfd, int stream_num, uint32_t flags, uint32_t extra_window_size)
{
#ifndef _WIN32
 	return dag_attach_stream_protection(dagfd, stream_num, flags, extra_window_size, PROT_READ | PROT_WRITE);
#else
	return dag_attach_stream_protection(dagfd, stream_num, flags, extra_window_size, NULL);
#endif
}

/*****************************************************************************/
int
dag_attach_stream_protection(int dagfd, int stream_num, uint32_t flags, uint32_t extra_window_size,int protection)
{
#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	void *p;
	void *sp, *ep;
	int aux;

#elif defined(_WIN32)

	ULONG BytesTransfered;
	dag_stream_info_t stream_info;  /* Info needed to allocate and map hole */
	int map_attempts = 0;    /* count how many attempts have been made at mapping the hole */
	BOOL map_successful = FALSE;  /* flag if mapping attempt was successful */
	uint32_t * mem_pointer;
	SYSTEM_INFO sys_info;

#else 
	#error Platform not supported 
#endif /* Platform-specific code. */

	int  buf_size=0;
	dagpbm_stream_t *pbm=0;
	dag_reg_t result[DAG_REG_MAX_ENTRIES];
	uint32_t regn=0;
	int  lock;
	uint64_t physaddr;
	uint32_t offset;    // offset in phy addr to present mem hole
	dagpbm_MkI_t * pbm_MkI = 0;
	dagpbm_stream_MkII_t * pbm_MkII = 0;
	dagpbm_stream_MkIII_t * pbm_MkIII = 0;
	int return_val = -1;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	/* flags currently ignored */
	//flags = flags;

	// Check if stream number is within DAG capabilities
	if (stream_num & 1) {
		// This is a transmit stream
		if (herd[dagfd].num_tx_streams <= ((stream_num - 1) / 2)) {
			errno = EINVAL;
			return (intptr_t) MAP_FAILED;
		}
	} else {
		// This is a receive stream
		if (herd[dagfd].num_rx_streams <= (stream_num / 2)) {
			errno = EINVAL;
			return (intptr_t) MAP_FAILED;
		}
	}

	/* Get stream lock */
	lock = (stream_num << 16) | 1;
	if(herd[dagfd].stream[stream_num].reverse_mode == DAG_REVERSE_MODE)
		lock |= (1 << 8);

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	aux = ioctl(dagfd, DAGIOCLOCK, &lock);
	if (aux != 0) {
		return (intptr_t) MAP_FAILED;	/* errno set */
	}

#elif defined(_WIN32)

	if (DeviceIoControl(herd[dagfd].handle,
		IOCTL_LOCK,
		&lock,
		sizeof(lock),
		NULL,
		0,
		&BytesTransfered,
		NULL) == FALSE) {
	    return (intptr_t) MAP_FAILED;	/* errno set */
	}
#else
#error Platform not supported 
#endif /* Platform-specific code. */

	/* in case daginf has changed */
	if (dag_update(dagfd) < 0)
		goto fail;

	// Look for DAG card PBM (Pci Burst Manager)
	if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_PBM, result, &regn)) {
		errno = EIO;
		goto fail;
	}

	// Do we have PBM?
	if(regn) {

		// Which version of the burst manager are we using?
		switch (result->version) {
		case 0: // Version 0 (MkI)
			pbm_MkI = (dagpbm_MkI_t *) (herd[dagfd].iom + result->addr + (stream_num * 0x60));

			if ( (pbm = (dagpbm_stream_t *) dagutil_malloc_aligned( sizeof(dagpbm_stream_t) )) == NULL)
				goto fail;

			// Initialize interface structure
			pbm->status        = &(pbm_MkI->status);
			pbm->mem_addr      = &(pbm_MkI->mem_addr);
			pbm->mem_size      = &(pbm_MkI->mem_size);
			pbm->record_ptr    = &(pbm_MkI->record_ptr);
			pbm->limit_ptr     = &(pbm_MkI->limit_ptr);
			pbm->safetynet_cnt = &(pbm_MkI->safetynet_cnt);
			pbm->drop_cnt      = &(pbm_MkI->drop_cnt);

			// Bind the structure to the stream
			herd[dagfd].stream[stream_num].pbm = pbm;

			break;

		case 1:	// Version 1 (MkII)
			pbm_MkII = (dagpbm_stream_MkII_t *) (herd[dagfd].iom + result->addr + 0x40 + (stream_num * 0x40));
			
			if ( (pbm = (dagpbm_stream_t *) dagutil_malloc_aligned( sizeof(dagpbm_stream_t) )) == NULL)
				goto fail;

			// Initialize interface structure
			pbm->status        = &(pbm_MkII->status);
			pbm->mem_addr      = &(pbm_MkII->mem_addr);
			pbm->mem_size      = &(pbm_MkII->mem_size);
			if (herd[dagfd].stream[stream_num].reverse_mode) {
#ifndef NDEBUG
				fprintf(stderr,"REVERSE OPERATION\n");
#endif
				pbm->record_ptr    = &(pbm_MkII->limit_ptr);
				pbm->limit_ptr     = &(pbm_MkII->record_ptr);
			} else 	{
#ifndef NDEBUG
				fprintf(stderr,"NORMAL  OPERATION\n");
#endif
				pbm->record_ptr    = &(pbm_MkII->record_ptr);
				pbm->limit_ptr     = &(pbm_MkII->limit_ptr);
			}
			pbm->safetynet_cnt = &(pbm_MkII->safetynet_cnt);
			pbm->drop_cnt      = &(pbm_MkII->drop_cnt);

			// Bind the structure to the stream
			herd[dagfd].stream[stream_num].pbm = pbm;

			break;

		case 2: // Version 2 (MkIII) CSBM
       		case 3:// Version 3 HSBM ( use the similar pbm stream structure 
			pbm_MkIII = (dagpbm_stream_MkIII_t *) (herd[dagfd].iom + result->addr + 0x40 + (stream_num * 0x40));
			
			if ( (pbm = (dagpbm_stream_t *) dagutil_malloc_aligned( sizeof(dagpbm_stream_t) )) == NULL)
				goto fail; 

			// Initialize interface structure
			pbm->status        = &(pbm_MkIII->status);
			pbm->mem_addr      = &(pbm_MkIII->mem_addr);
			pbm->mem_size      = &(pbm_MkIII->mem_size);
			pbm->record_ptr    = &(pbm_MkIII->record_ptr);
			pbm->limit_ptr     = &(pbm_MkIII->limit_ptr);
			pbm->safetynet_cnt = &(pbm_MkIII->safetynet_cnt);
			pbm->drop_cnt      = NULL; // not present

			/* Make sense for Linux only. Win32 uses non standard IOCTL 
			interface - so avoid other OS's while not required */
#if defined(__linux__) 
			/* An IOCTL to get full 64bit physical address */
			/* Generic daginf_t can return only 32bit value */
			if (ioctl(dagfd, DAGIOCPHYADDR, &physaddr) == 0) {
				pbm_MkIII->mem_addr_h = physaddr >> 32;
				pbm_MkIII->limit_ptr_h = physaddr >> 32;
			}
#endif
			// Bind the structure to the stream
			herd[dagfd].stream[stream_num].pbm = pbm;

			break;

		default: // Not implemented at this time
			errno = EINVAL;
			return_val = (intptr_t) MAP_FAILED;
			goto fail;
		}

		// Is PBM for this stream unconfigured? -> initialize
		if(!PBM_CONFIGURED(pbm)) {
			if (stream_num) {
				errno = ENOMEM;
				goto fail;
			} else {
				*(pbm->mem_addr) = dag_info(dagfd)->phy_addr; /* XXX curaddr bugfix */
				*(pbm->mem_size) = dag_info(dagfd)->buf_size;
			}
		}
	}

	// Get the buffer size from the card if possible, since
	// if rx/tx is configured only part of the memory space
	// is used for rx.
	if(pbm)
	{
		buf_size = *(pbm->mem_size);
#if defined(_WIN32)
//CHECKME:
		/* an error can occur on Windows platform when driver is unloaded and reloaded and 
		   no reboot is done that causes the memory available to drop, therefore limit the 
		   memory requested */
		if(buf_size > dag_info(dagfd)->buf_size)
		{
			
			buf_size = dag_info(dagfd)->buf_size;
		}		
#else 
//CHECKME:
/*		if(buf_size > dag_info(dagfd)->buf_size)
		{
#if 0 
		printf("happening \n");
#endif 		
		buf_size = dag_info(dagfd)->buf_size;
		}		
*/			
	
#endif /* _WIN32 */
	}
	else
	{
		buf_size = dag_info(dagfd)->buf_size;
	}
	// Store buf_size as an stream parameter
	herd[dagfd].stream[stream_num].size = buf_size;

	// Set stream polling parameters
	herd[dagfd].stream[stream_num].mindata = dagapi_mindata;
	herd[dagfd].stream[stream_num].maxwait = dagapi_maxwait;
	herd[dagfd].stream[stream_num].poll    = dagapi_poll;

	// Store free_space as an stream parameter
	herd[dagfd].stream[stream_num].free_space = 0;

	// The first call to get/put records in the stream may be
	// different, so we have to know about that
	herd[dagfd].stream[stream_num].is_first_time = 1;

	/*
	 * Start off with a fake mapping to allocate contiguous virtual
	 * address space in one lot for the size of the memory buffer
	 * plus an extra window which will let us wrap around the buffer
	 * The window's size is specified as an argument to this function.
	 * If it takes the value 0, we are in fact mmaping twice the memory
	 * hole (that was the first way to work in the API)
	 */
	if (extra_window_size == 0) {
		extra_window_size = buf_size;
	}

	if ((int)extra_window_size > buf_size) {
		errno=EINVAL;
		return_val = (intptr_t) MAP_FAILED;
		goto fail;
	}

#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	// Do the mmap
	if((sp = mmap(NULL, buf_size + extra_window_size, protection,
			MAP_ANON|MAP_SHARED, -1, 0)) == MAP_FAILED)
	{
		return_val = (intptr_t) MAP_FAILED;
		goto fail;
	}

	/*
	 * 'dagfd' provides the starting physical address of the whole
	 * memory holes. We should apply an offset to point exactly
	 * at the memory hole used by the present stream.
	 */
	if (pbm)
		offset = *(pbm->mem_addr) - dag_info(dagfd)->phy_addr;
	else
		offset = 0;

	/*
	 * Now map the real buffer, 1st round.
	 */

	if((p = mmap(sp, buf_size, protection,
			MAP_FIXED|MAP_SHARED, dagfd, offset)) == MAP_FAILED)
	{
		return_val = (intptr_t) MAP_FAILED;
		goto fail;
	}
	/*
	 * Map the buffer for a second time, this will turn out to be a neat
	 * feature for handling data records crossing the wrap around the
	 * top of the memory buffer.
	 *
	 * We are not mmaping the whole memory buffer for a second time, but
	 * only the smallest part as possible. Although we are not wasting
	 * physical memory addresses, we are wasting virtual memory space.
	 * The size of this second mmap is specified in the arguments of
	 * this function (extra_window_size). If this size is 0, that means
	 * we are mmaping the whole buffer (as it worked in previous
	 * releases)
	 */
	if((ep = mmap(sp+buf_size, extra_window_size, protection,
			MAP_FIXED|MAP_SHARED, dagfd, offset)) == MAP_FAILED)
	{
		return_val = (intptr_t) MAP_FAILED;
		goto fail;
	}

	herd[dagfd].stream[stream_num].buf = p;

#elif defined(_WIN32)

	if (pbm)
		offset = *(pbm->mem_addr) - dag_info(dagfd)->phy_addr;
	else
		offset = 0;

	/*
	 * Really want to send the following info:
	 * - stream_num
	 * - hole size
	 * - extra size
	 *
	 */
	stream_info.num = stream_num;
	stream_info.size = buf_size;
	stream_info.offset = offset;
	stream_info.extra_size = extra_window_size;

	GetSystemInfo(&sys_info);

	while(map_successful == FALSE )
	{
		map_attempts++;
		mem_pointer = malloc(stream_info.size+stream_info.extra_size+sys_info.dwPageSize);

		if(mem_pointer == NULL)
		{
			//continue;
			// The proper fix is to make Windows not using malloc here 
			//Note this may be not the best fix due after some time and if the system has enough Virtual memory (Swap space) 
			// It will find mmemory
			if(map_attempts < 100) {
			    continue;
			} else {
			   /* 100 unsuccessful attempts at mapping have been made - fail loudly */
			    fprintf(stderr, "Memory could not be mapped\n");
			    goto fail;
			}
		}

		free(mem_pointer);
		stream_info.memory_address = mem_pointer;

			/* XXX this is going to need some work to match the above */
		if (DeviceIoControl(herd[dagfd].handle,
				    IOCTL_GET_HOLE,
				    &stream_info,
				    sizeof(stream_info),
				    &herd[dagfd].stream[stream_num].buf,
				    sizeof(herd[dagfd].stream[stream_num].buf),
				    &BytesTransfered,
				    NULL) == FALSE)
		{
			LPVOID lpMsgBuf;
			int error_code = GetLastError();

			/* determine if the failure has occurred due to unsuccessful mapping in the driver 
			   or some other error that should be considered fatal */
			if(error_code == ERROR_GEN_FAILURE) /* map was unsuccessful, retry */
			{	
				if(map_attempts < 100)
					continue;
				else
				{
					/* 100 unsuccessful attempts at mapping have been made - fail loudly */
				    fprintf(stderr, "Memory could not be mapped\n");
				    goto fail;
				}
			}
			
			/*error is fatal - fail loudly*/
			if (!FormatMessage( 
			    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			    FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				error_code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL ))
			{
			    fprintf(stderr, "Arg: IOCTL_GET_HOLE failed, FormatMessage failed\n");
			    goto fail;
			}

			fprintf(stderr, lpMsgBuf);
			fprintf(stderr, "\n");
			LocalFree(lpMsgBuf);
			return_val = (int) MAP_FAILED;
			goto fail;
		}
		/*we have memory mapped, break from the loop*/
		map_successful = TRUE;
	}
#else 
	#error Platform not supported 
#endif /* Platform-specific code. */

	// In order to unmap this second mmap, we need to store the its size
	herd[dagfd].stream[stream_num].extra_window_size = extra_window_size;

	// Store the pointer to the memory hole space (stream)
#if 0 	
	printf("[dag_attach_stream] stream: %d, buf: 0x%p , size: %d\n", stream_num, herd[dagfd].stream[stream_num].buf,herd[dagfd].stream[stream_num].size);
#endif 
	// Set initial record offset
	herd[dagfd].stream[stream_num].offset = 0;

	// Store stream starting physical address
	if (pbm)
		herd[dagfd].stream[stream_num].bus_addr = *(pbm->mem_addr);
	else
		herd[dagfd].stream[stream_num].bus_addr = dag_info(dagfd)->phy_addr;

	// If we arrive this point, all has been successful
	herd[dagfd].stream[stream_num].attached = 1;
	herd[dagfd].stream[stream_num].started  = 0;

	return 0;

fail:
	/* Release the stream lock. */
	lock = (stream_num << 16) | 0; /* unlock */
	if(herd[dagfd].stream[stream_num].reverse_mode == DAG_REVERSE_MODE)
		lock |= (1 << 8);

	/* We're returning an error anyway, so there's little point checking the ioctl() return value. */
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	(void) ioctl(dagfd, DAGIOCLOCK, &lock);

#elif defined(_WIN32)

	(void) DeviceIoControl(herd[dagfd].handle,
							IOCTL_LOCK,
							&lock,
							sizeof(lock),
							NULL,
							0,
							&BytesTransfered,
							NULL);
#else
#error Platform not supported
#endif /* Platform-specific code. */

	return return_val; /* Usually -1, sometimes (int) MAP_FAILED. */
}

/*****************************************************************************/
/* This function is private. It is intended to keep compatibility backwards  */
/* in function dag_mmap, which needs a pointer. Pointers has been hidden in  */
/* dag_attach to provide a more abstract interface                           */
/*****************************************************************************/
static void*
dag_get_stream_ptr (int dagfd, int stream_num) {

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	return herd[dagfd].stream[stream_num].buf;
}

/*****************************************************************************/
/* This function maps the stream 0 (or the receive memory hole) and returns  */
/* a pointer to it. Its use is deprecated and should be replaced as soon as  */
/* possible for dag_attach                                                   */
/*****************************************************************************/
/* Deprecated */
void*
dag_mmap(int dagfd)
{
	void *p;
	int status;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return MAP_FAILED;
	}

	status =  dag_attach_stream(dagfd, 0, 0, 0);
	if (status != (intptr_t) MAP_FAILED) {
		herd[dagfd].implicit_detach = 1;
		p = dag_get_stream_ptr(dagfd, 0);
	} else {
		p = MAP_FAILED;
	}

	return p;
}

/*****************************************************************************/
uint8_t*
dag_advance_stream(int dagfd, int stream_num, uint8_t **bottom)
{
	struct timeval now;
	struct timeval expire;
/* 	int tx = stream_num&1; /\* odd stream_nums are transmit *\/ */
	stream_t *stream = &(herd[dagfd].stream[stream_num]);
	int mindata = stream->mindata;
	volatile dagpbm_stream_t *pbm = stream->pbm;
	const int size = stream->size;
	uint32_t bus_addr = stream->bus_addr;
	uint32_t dsmu_bus_addr;
	uint32_t dsmu_val = 0;
	uint8_t *offset;
	uint8_t *buf = stream->buf;

#define BOTTOM_POINTER (*bottom) 
	
	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return NULL;
	}

	memset(&expire, 0, sizeof(expire));
	memset(&now, 0, sizeof(now));

	if (pbm && !PBM_CONFIGURED(pbm)) {
		errno = ENOMEM; /* XXX not strictly true? */
		return NULL;    /* hole not configured */
	}
	if (0) {
#if 0		
		printf("this must not happen because we are not using dag_advance_stream on transmit streams \n");
#endif 		
		/* wrap around the virtual address*/
		if (BOTTOM_POINTER >= (buf+size)) {
			BOTTOM_POINTER -= size;
		}

		/* recalculate limit pointer from virtual address */
		/*Note: probably is is a good idia to have unified convertion */		
		stream->last_bus_bottom = bus_addr + (BOTTOM_POINTER - buf);
		*(pbm->limit_ptr) = stream->last_bus_bottom;

		stream->last_bus_top = *(pbm->record_ptr);
		
		
		//offset = (void *)stream->last_bus_top ;

		/* convert a offset pointer from BUS ADDRESS to Virtual address*/		
		offset = buf +  (uint32_t) (stream->last_bus_top - bus_addr);
		/* ensures that the new top pointer is bigger then the botom pointer in virtual address space
		due the double mapping that is ok
		 */
		if(offset <= BOTTOM_POINTER) {
			offset += size;
		}
		

	} else { /* RX */

		/*
		 * On the first call the user has no prior buffer, so may pass in NULL.
		 * We assume that the capture starts with the hole empty so we can set
		 * bottom to the userspace hole base address
		 */

        if((stream->reverse_mode == DAG_REVERSE_MODE) && (stream->paused)) 
        {
            BOTTOM_POINTER = NULL;
            return NULL;
        }

        if(BOTTOM_POINTER == NULL)
        {
            if(stream->reverse_mode == DAG_NORMAL_MODE)
            {
		/*
		 * The implementation implies that offsets within
		 * the buffer are now starting zero to top of the
		 * hole, exclusive the top address, which is considered
		 * zero.
		 */
                BOTTOM_POINTER = buf;
            } 
            else 
            {
                uint32_t restored_offset;
                restored_offset = (uint32_t)(*(pbm->limit_ptr) - bus_addr + SAFETY_WINDOW);
                if(restored_offset >= size) 
                {
                    restored_offset -= size;
                }
                BOTTOM_POINTER = buf + restored_offset;
            }
        }


#if 0		
		printf("dag_advance_stream:  buf: %p Bottom_pointer: %p size %d \n",buf,BOTTOM_POINTER ,size);
#endif 		
		if(BOTTOM_POINTER >= size+buf)
			BOTTOM_POINTER -= size;

		if(pbm) {
			offset = (void *)(PBMOFFSET(dagfd, stream_num) + buf);
#if 0 			
		        printf("dag_advance_stream:[2] offset %p  PMBOFFSET %d  \n",offset, PBMOFFSET(dagfd, stream_num) );
#endif 
			/*
			 * Advance acknowledgement pointer, this should be done in
			 * all cases, blocking or non-blocking.
			 * Reinit the burst manager, in case safety net was reached
			 * XXX we might consider reporting safety net status ?
			 */
			stream->last_bus_bottom = bus_addr + WRSAFE(dagfd, stream_num, (BOTTOM_POINTER-buf));
#if 0 			
		        printf("dag_advance_stream:[3] stream->last_bus_bottom %08x  bus_addr %08x  WRSAFE %d \n",stream->last_bus_bottom, bus_addr, WRSAFE(dagfd, stream_num, (BOTTOM_POINTER-buf)) );
#endif  
	
			*(pbm->limit_ptr) = stream->last_bus_bottom;
            /* TODO . this is done temporarily to support reverse operation synchronisation*/
            /* TODO More complete and precise solution is needed */
             if(stream->reverse_mode == DAG_NORMAL_MODE)
			     *(pbm->status) = (DAGPBM_AUTOWRAP|herd[dagfd].byteswap);
             else 
                 *(pbm->status) |= (DAGPBM_AUTOWRAP|herd[dagfd].byteswap);
			/*
			 * With the WRSAFE() macro in place, if offset equals oldoffset,
			 * the buffer is guaranteed to be empty.
			 */
			
			while( (unsigned long)(offset>=BOTTOM_POINTER?offset-(unsigned long)BOTTOM_POINTER:size-(unsigned long)BOTTOM_POINTER+offset) < mindata) {

 				/* If a timeout is configured: */
				if (timerisset(&herd[dagfd].stream[stream_num].maxwait)) {
					/* If we have been through before, update current time */
					if (timerisset(&now)) {
						(void)gettimeofday(&now, NULL);
					} else {
						/* Otherwise record start time, start timer */
						(void)gettimeofday(&now, NULL);
						expire = now;
						timeradd(&expire, &herd[dagfd].stream[stream_num].maxwait, &expire);
					}
				}

				if(herd[dagfd].dsmu != NULL)
				{
					/* check for blocks to be flushed */
					dsmu_bus_addr = bus_addr  + (offset - buf) ;
					
					if( (dsmu_bus_addr == stream->last_bus_top) && stream->pad_flush )
					{
						/*read value from the firmware 32bit */
						dsmu_val = *herd[dagfd].dsmu; 

						/* check for data available */
						 if(( dsmu_val & (0x1 << (stream_num / 2))) > 0) 
						 {
							/* prod data into memory hole (along with pad records)  */
						    *herd[dagfd].dsmu = (0x1 << (stream_num / 2));
						 }
					  }
				}
	
				/* Windows does not support >= or <= for timercmp. */
				if (timerisset(&herd[dagfd].stream[stream_num].maxwait) && timercmp(&now, &expire, >))
				{ 
					break;
				}

				/* If poll sleep configured, sleep */
				if (herd[dagfd].stream[stream_num].poll.tv_sec != 0 ||
				    herd[dagfd].stream[stream_num].poll.tv_nsec != 0) {
					nanosleep(&herd[dagfd].stream[stream_num].poll, NULL);
				}

				/* Update offset from card */
				offset = (void*)(PBMOFFSET(dagfd, stream_num) + buf);
			}

		} else {

			offset = (void *)(ARMOFFSET(dagfd) + buf);

			while( (unsigned long)(offset>=BOTTOM_POINTER?offset-(unsigned long)BOTTOM_POINTER:size-(unsigned long)BOTTOM_POINTER+offset) < mindata) {

				/* If a timeout is configured: */
				if (timerisset(&herd[dagfd].stream[stream_num].maxwait)) {
					/* If we have been through before, update current time */
					if (timerisset(&now)) {
						(void)gettimeofday(&now, NULL);
					} else {
						/* Otherwise record start time, start timer */
						(void)gettimeofday(&now, NULL);
						expire = now;
						timeradd(&expire, &herd[dagfd].stream[stream_num].maxwait, &expire);
					}
				}

				/* Windows does not support >= or <= for timercmp. */
				if (timerisset(&herd[dagfd].stream[stream_num].maxwait) && timercmp(&now, &expire, >))
					break;

				if (herd[dagfd].stream[stream_num].poll.tv_sec != 0 ||
				    herd[dagfd].stream[stream_num].poll.tv_nsec != 0) {
					nanosleep(&herd[dagfd].stream[stream_num].poll, NULL);
				}
				offset = (void*)(ARMOFFSET(dagfd) + buf);
			}
		}

		/* Sanity check */
		if((offset - buf) > size) {
			fprintf(stderr, "dagapi: dag_offset internal error offset=0x%lx\n", (unsigned long)(offset-(unsigned long)buf));
			errno = EIO;
			return NULL;
		}
		/**/
		 //error bad assumption or error it will make a problem only with dsmu
		 //not synchronized with the frimware record 
		 //FIXME:
		stream->last_bus_top = bus_addr +offset - buf;

		/* If top pointer has wrapped over, use extra memmapping to make it look contiguous */
		if(offset < BOTTOM_POINTER)
			offset += size;

		/*
		 * If top pointer is now higher than the extra_window_size available,
		 * trim it to the avilable extra_window_size. The user will see the rest of
		 * the available data the next time they call dag_advance, since then
		 * the bottom pointer will be >buf+size, and will be wrapped back down.
		 */
		if(offset > buf+size+stream->extra_window_size)
			offset = buf+size+stream->extra_window_size;
	}

	return offset;
}

/*****************************************************************************/
int
dag_offset(int dagfd, int *oldoffset, int flags)
{
	uint32_t mindata;
	struct timeval maxwait;
	struct timeval poll;
	int offset;
	uint8_t *oldoffset_p;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(flags & DAGF_NONBLOCK) {
		dag_get_stream_poll(dagfd, 0, &mindata, &maxwait, &poll);
		if(mindata)
			dag_set_stream_poll(dagfd, 0, 0, &maxwait, &poll);
	}
	oldoffset_p = herd[dagfd].stream[0].buf + *oldoffset;
	offset = (int) ((unsigned long)dag_advance_stream(dagfd, 0, &oldoffset_p) - (unsigned long)herd[dagfd].stream[0].buf);
	*oldoffset = (unsigned long) (oldoffset_p - (unsigned long)herd[dagfd].stream[0].buf);
	
	return offset;
}

/*****************************************************************************/
/* New                                                                       */
/* This function returns a pointer to the next record in the memory hole, so */
/* it is a zero-copy function. To advance to the next record it should take  */
/* a look into the ERF header for RLEN (record length). It also deals with   */
/* the hole boundaries so the user don't have to worry about.                */
/* The offset in the memory hole is kept in the 'stream' data                */
/* structure (inside a sheep, inside a herd)                                 */
/*****************************************************************************/
uint8_t*
dag_rx_stream_next_record (int dagfd, int stream_num)
{
	stream_t * stream	= &(herd[dagfd].stream[stream_num]);
	uint8_t record_type;
	int32_t rlen;
	uint8_t * next_record;		/* pointer to the next record        */
	const uint8_t * buf	= stream->buf;
	uint8_t * top;
	
	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return NULL;
	}

	// Is the stream a read stream or a write stream?
	// We need a read stream
	// Odd numbers are write (transmit) streams
	if (stream_num & 1) {
		//errno = ENOTTY;
		//return NULL;
	}

	// Get next record pointer
	if (stream->is_first_time) {
		stream->free_space = 0;
		stream->offset = 0;
		stream->processed = 0;
		stream->is_first_time = 0;
	}
	
	next_record = (uint8_t *) buf + stream->offset;

	/* If we know of no more data to process, or we have processed at least extra_window_size,
	 * then call dag_advance to free some buffer space and check for new data.
	 * If extra_window_size is equal to the stream size, then this has no effect and the whole
	 * buffer size is always processed before freeing space in the buffer.
	 */
	if ((stream->free_space < dag_record_size) ||
	    ((stream->processed + dag_record_size) > stream->extra_window_size) ) {
		
		top = dag_advance_stream(dagfd, stream_num, &next_record);

		if(top == NULL) {
			/* errno set by dag_advance */
			return NULL;
		}
		stream->processed = 0;
		stream->free_space = top - next_record;
		if(stream->free_space < dag_record_size) {
			errno = EAGAIN;
			return NULL;
		}
	}
	/* by now we have an ERF header, hopefully */
	record_type = ((dag_record_t*)next_record)->type & 0x7F;
	if( (record_type < TYPE_MIN) || (record_type > TYPE_MAX) ) {
		errno = EIO; /* HELP */
		return NULL;
	}

	/* ERF type within valid range */

	/* Get record length */
	rlen = ntohs(((dag_record_t*)next_record)->rlen);

	if ( (stream->free_space < rlen) ||
	     ((stream->processed + rlen) > stream->extra_window_size) ){
		top = dag_advance_stream(dagfd, stream_num, &next_record);
		if(top == NULL) {
			/* errno set by dag_advance */
			return NULL;
		}
		stream->processed = 0;
		stream->free_space = top - next_record;
		if(stream->free_space < rlen) {
			errno = EAGAIN;
			return NULL;
		}
	}

	/* have a valid erf header and complete record */

	/* setup for next call */
	stream->offset = rlen + (uint32_t) (next_record - buf);
	stream->free_space -= rlen;
	stream->processed += rlen;

	return next_record;
}

/*****************************************************************************/
/* This function is intended for inline forwarding of packets                */
/* It does the same as dag_rx_stream_next_record but also keeps track of the */
/* pointers of the transmit stream                                           */
/*****************************************************************************/
uint8_t*
dag_rx_stream_next_inline (int dagfd, int rx_stream_num, int tx_stream_num)
{
	stream_t * rx_stream = &(herd[dagfd].stream[rx_stream_num]);
	stream_t * tx_stream = &(herd[dagfd].stream[tx_stream_num]);
	volatile dagpbm_stream_t *rx_pbm = rx_stream->pbm;
	volatile dagpbm_stream_t *tx_pbm = tx_stream->pbm;
	uint8_t record_type;
	int32_t rlen;
	int32_t offset;
	uint8_t * next_record; /* pointer to the next record        */
	struct timeval now;
	struct timeval expire;
	
	/* is the card opened? */
	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	/* receive stream numbers should be even */
	/* transmit stream numbers should be odd */
	//Note tyhe meaning of the streams in Reverse mode is the oposite 
	if (rx_stream_num & 1 || !tx_stream_num & 1) 
	{
		if( (herd[dagfd].stream[rx_stream_num].reverse_mode == DAG_NORMAL_MODE) && (herd[dagfd].stream[tx_stream_num].reverse_mode == DAG_NORMAL_MODE) )
		{
			errno = ENOTTY;
			return NULL;
		};
	}
	
	/* are the streams properly attached? */
	if(!rx_stream->attached || !tx_stream->attached) {
		errno = EINVAL;
		return NULL;
	}

	/* initialize some parameters the first time */
	if (rx_stream->is_first_time) {
		rx_stream->offset = 0;
		rx_stream->processed = 0;
		rx_stream->free_space = 0;
		rx_stream->is_first_time = 0;
	}
	

	memset(&expire, 0, sizeof(expire));

	/* this is the pointer to the next valid record                  */
	/* the next valid record is the one we are returning to the user */
	next_record = (uint8_t*) (rx_stream->buf + rx_stream->offset);
	
	/* are there enough bytes of the record to read? */
	/* the following code are changed by jeff 2006-04-19,
	 * orginal, a flag "first" is used, so that the thread would sleep the first time in the loop,
	 * this may give better latency for some of the packets, but the CPU expense is too high.*/
	if ((rx_stream->free_space < rx_stream->mindata ) ||
	       ((rx_stream->processed + dag_record_size) > rx_stream->extra_window_size) ) 
	{
		if (timerisset(&rx_stream->maxwait) )
	        {
			(void)gettimeofday(&now, NULL);
			expire = now;
			timeradd(&expire, &rx_stream->maxwait, &expire);
		}
		
		while(1)
		{
			/* before updating the free space, lets update the limit pointer of  */
			/* the receive stream. Remember that the limit pointer cannot        */
			/* overpass the read pointer of the transmit stream                  */
			tx_stream->last_bus_top = *(tx_pbm->record_ptr);
			offset = tx_stream->last_bus_top - (uint32_t)tx_stream->bus_addr - SAFETY_WINDOW;
			if (offset < 0) offset += rx_stream->size;
			rx_stream->last_bus_bottom =  (uint32_t) (rx_stream->bus_addr + offset);
			*(rx_pbm->limit_ptr) = rx_stream->last_bus_bottom;
		
			/* Windows does not support >= or <= for timercmp. */
			if (timerisset(&herd[dagfd].stream[rx_stream_num].maxwait) && timercmp(&now, &expire, >)) {
				errno = EAGAIN;
				return NULL;
			}
	
			if (  (rx_stream->poll.tv_sec != 0 || rx_stream->poll.tv_nsec != 0) ) {
				nanosleep(&rx_stream->poll, NULL);
				(void)gettimeofday(&now, NULL);
			}
		
			/* now lets find how many bytes can we read */
			rx_stream->last_bus_top = *(rx_pbm->record_ptr);
			rx_stream->free_space = rx_stream->last_bus_top - (rx_stream->offset + rx_stream->bus_addr);
			if (rx_stream->free_space < 0) rx_stream->free_space += rx_stream->size;

			rx_stream->processed = 0;
			if(!((rx_stream->free_space < rx_stream->mindata ) ||
	       			((rx_stream->processed + dag_record_size) > rx_stream->extra_window_size) ))
				break;
		}
	}
	/* by now we have an ERF header, hopefully */
	record_type = ((dag_record_t*)next_record)->type & 0x7F;
	
	/* check if the record type is right */
	if( (record_type < TYPE_MIN) || (record_type > TYPE_MAX) ) {
		errno = EIO;
		return NULL;
	}

	/* Get record length */
	rlen = ntohs(((dag_record_t*)next_record)->rlen);

	/* are there enough byte to completely read the record? */
	if ((rx_stream->free_space < rx_stream->mindata ) ||
	       ((rx_stream->processed + dag_record_size) > rx_stream->extra_window_size) ) 
	{
		/* you should have a deja vu, this loop is the same as the previous 
           This is not really needed.
           The idea is basically to avoid corrupt data from hardware, 
           If we believe hardware will always change their pointers on packets boundary, 
           then this could should never execute */
		if (timerisset(&rx_stream->maxwait) )
	        {
			(void)gettimeofday(&now, NULL);
			expire = now;
			timeradd(&expire, &rx_stream->maxwait, &expire);
		}
		
		while(1)
		{
			/* before updating the free space, lets update the limit pointer of  */
			/* the receive stream. Remember that the limit pointer cannot        */
			/* overpass the read pointer of the transmit stream                  */
			tx_stream->last_bus_top = *(tx_pbm->record_ptr);
			offset = tx_stream->last_bus_top - (uint32_t)tx_stream->bus_addr - SAFETY_WINDOW;
			if (offset < 0) offset += rx_stream->size;
			rx_stream->last_bus_bottom =  (uint32_t) (rx_stream->bus_addr + offset);
			*(rx_pbm->limit_ptr) = rx_stream->last_bus_bottom;
		
			/* Windows does not support >= or <= for timercmp. */
			if (timerisset(&herd[dagfd].stream[rx_stream_num].maxwait) && timercmp(&now, &expire, >)) {
				errno = EAGAIN;
				return NULL;
			}
	
			if (  (rx_stream->poll.tv_sec != 0 || rx_stream->poll.tv_nsec != 0) ) {
				nanosleep(&rx_stream->poll, NULL);
				(void)gettimeofday(&now, NULL);
			}
		
			/* now lets find how many bytes can we read */
			rx_stream->last_bus_top = *(rx_pbm->record_ptr);
			rx_stream->free_space = rx_stream->last_bus_top - (rx_stream->offset + rx_stream->bus_addr);
			if (rx_stream->free_space < 0) rx_stream->free_space += rx_stream->size;

			rx_stream->processed = 0;
			if(!((rx_stream->free_space < rx_stream->mindata ) ||
	       			((rx_stream->processed + dag_record_size) > rx_stream->extra_window_size) ))
				break;
		}
	}

	/* setup for next call */
	rx_stream->free_space -= rlen;
	rx_stream->processed += rlen;
	
	rx_stream->offset += rlen;
	if (rx_stream->offset >= rx_stream->size)
		rx_stream->offset -= rx_stream->size;

	return next_record;
}


/*****************************************************************************/
/* This function 'commits' a certain amount of bytes into the stream buffer. */
/* The normal use of this function is after writing a record in the stream,  */
/* so we need to tell the card how many bytes we have written.               */
/* The function returns a pointer to the next available position to write    */
/* in the stream buffer                                                      */
/* Attention: the number of bytes you commit cannot overpass the extra       */
/* window you used in 'dag_attach'                                           */
/*****************************************************************************/
uint8_t*
dag_tx_stream_commit_bytes (int dagfd, int stream_num, uint32_t size)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	volatile dagpbm_stream_t * pbm = stream->pbm;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return NULL;
	}

	if (size > herd[dagfd].stream[stream_num].size) {
		errno = EINVAL;
		return NULL;
	}

	// If we use a burst manager (not ARM), is it initialized?
	if (pbm && !PBM_CONFIGURED(pbm)) {
		errno = ENOMEM;
		return NULL;
	}

	// ARM processor
	if (pbm == NULL) {
#if 0
		printf("[%s(%u):dag_commit_bytes] ARM writing not supported\n", __FILE__, __LINE__);
#endif
		errno = ENOTTY;
		return NULL;
	}

	// Is the stream a read stream or a write stream?
	// We need a write stream
	// Even numbers are read streams
	// Odd  numbers are write streams
	if ( (stream_num & 1) == 0) {
		if( ! herd[dagfd].stream[stream_num].reverse_mode )
		{
			errno = ENOTTY;
			return NULL;
		}
	}

    if((stream->paused) && (stream->reverse_mode == DAG_REVERSE_MODE))  {
		errno = ENOMEM;
		return NULL;
	}

	// Advance offset in the number of bytes to commit
	stream->offset += size;

	// Keep offset inside buffer boundaries
	if (stream->offset >= stream->size)
	    stream->offset -= stream->size;

	// Advance pointer in Burst Manager (PBM)
	// We keep offset inside buffer boundaries, so it is not
	// necessary to make another check
	// Move the limit pointer at the end of the record (always)
	stream->last_bus_bottom = stream->bus_addr + stream->offset;
	*(pbm->limit_ptr) =  stream->last_bus_bottom;

	// All OK
	return stream->buf + stream->offset;
}

/*****************************************************************************/
/* COPY 'size' bytes from 'orig' to 'stream_num' in 'dagfd' for tx.          */
/* This is not a zero-copy function, so it should be avoided when possible.  */
/* If the copy overpasses the buffer limit, split in two copies (transparent)*/
/* Function returns the number of bytes written.                             */
/*****************************************************************************/
int
dag_tx_stream_copy_bytes(int dagfd, int stream_num, uint8_t * orig, uint32_t size)
{
	int remaining_block;		/* bytes remaining to the end of buf */
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	volatile dagpbm_stream_t * pbm = stream->pbm;
	int written;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	if (size > herd[dagfd].stream[stream_num].size) {
		errno = EINVAL;
		return -1;
	}

	// If we use a burst manager (not ARM), is it initialized?
	if (pbm && !PBM_CONFIGURED(pbm)) {
		errno = ENOMEM;
		return -1;
	}

	// Is the stream a read stream or a write stream?
	// We need a write stream
	// Even numbers are read streams
	// Odd  numbers are write streams
	//Note in Reverse mode the meaning of the streams is the oposite 
	if ((stream_num & 1)== 0) {
		if( ! herd[dagfd].stream[stream_num].reverse_mode )
		{
			errno = ENOTTY;
			return -1;
		};
	}

    if((stream->paused) && (stream->reverse_mode == DAG_REVERSE_MODE))  {
		errno = ENOMEM;
		return size;
	}

	// Is there enougth space to commit that amount of bytes?
	// Wait until space is available.
	while (stream->free_space < (int32_t)size) {
		stream->last_bus_top = *(pbm->record_ptr);
		//stream->free_space =  stream->last_bus_top - ( *(pbm->limit_ptr) + SAFETY_WINDOW );
		stream->free_space = stream->last_bus_top - stream->bus_addr - (stream->offset + SAFETY_WINDOW);
		if (stream->free_space < 0 ) {
			stream->free_space += stream->size;
		}
		nanosleep(&herd[dagfd].stream[stream_num].poll,NULL);

		if((stream->paused) && (stream->reverse_mode == DAG_REVERSE_MODE))  {
			errno = ENOMEM;
			return size;
		}
	}
	written = size;

	// Decrease free space
	stream->free_space -= written;

	// Can we do only one memcopy or have to split in two?
	remaining_block = stream->size - stream->offset;
	if (remaining_block >= written) {
		// Just one memcpy
		memcpy (stream->buf + stream->offset, orig, written);
	} else {
		// Two memcpy's
		memcpy (stream->buf + stream->offset, orig, remaining_block);
		memcpy (stream->buf, (uint8_t *)orig + remaining_block, written - remaining_block);
	}

	// Advance offset in the number of bytes to commit
	stream->offset += written;

	// Keep offset inside buffer boundaries
	if (stream->offset >= stream->size)
	    stream->offset -= stream->size;

	// Advance pointer in Burst Manager (PBM)
	// We keep offset inside buffer boundaries, so it is not
	// necessary to make another check
	stream->last_bus_bottom = stream->bus_addr + stream->offset;
	*(pbm->limit_ptr) = stream->last_bus_bottom;

	// All OK
	return written;
}

/*****************************************************************************/
/* Waits for the transmit stream to have at least 'size' bytes free to write */
/* in. It returns a pointer where the user can write his ERF records. After  */
/* writing the records, they must still be committed before they will be sent*/
/*****************************************************************************/
uint8_t*
dag_tx_get_stream_space (int dagfd, int stream_num, uint32_t size)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	volatile dagpbm_stream_t * pbm = stream->pbm;	/* pointer to bus manager            */
	struct timeval now;
	struct timeval expire;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return NULL;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return NULL;
	}

	// Is the stream a read stream or a write stream?
	// We need a write stream
	// Even numbers are read streams
	// Odd  numbers are write streams
	// Note that in Reverse mode the Read is done from a write stream 
	// Note that in Reverse mode the Write is done to a read stream 

	if ((stream_num & 1)== 0) {
		if( ! stream->reverse_mode )
		{
			errno = ENOTTY;
			return NULL;
		}
	}

	// If we use a burst manager (not ARM), is it initialized?
	if (pbm && !PBM_CONFIGURED(pbm)) {
		errno = ENOMEM;
		return NULL;
	}

	if((stream->paused) && (stream->reverse_mode == DAG_REVERSE_MODE))  {
		errno = ENOMEM;
		return NULL;
	}

	memset(&expire, 0, sizeof(expire));

	// Wait for free space to write
	while (stream->free_space < (int32_t)size) {
		stream->last_bus_top = *(pbm->record_ptr);
		stream->last_bus_bottom = *(pbm->limit_ptr);
		//stream->free_space = stream->last_bus_top - (stream->last_bus_bottom + SAFETY_WINDOW);
		stream->free_space = stream->last_bus_top - stream->bus_addr - (stream->offset + SAFETY_WINDOW);

		if (stream->free_space < 0 ) {
			stream->free_space += stream->size;
		}

		if (stream->free_space >= (int32_t)size)
			break;

		if(0 == stream->mindata) {
			errno = EAGAIN;
			return NULL;
		}
		
		if (timerisset(&stream->maxwait)) {
			if (timerisset(&expire)) {
				(void)gettimeofday(&now, NULL);
				if (timercmp(&now, &expire, >))
				{ 
					errno = EAGAIN;
					return NULL;
				}
			} else {
				(void)gettimeofday(&now, NULL);
				expire = now;
				timeradd(&expire, &stream->maxwait, &expire);
			}
		}

		if (stream->poll.tv_sec != 0 ||
		    stream->poll.tv_nsec != 0) {
			nanosleep(&stream->poll, NULL);
		}

		if((stream->paused) && (stream->reverse_mode == DAG_REVERSE_MODE))
		{
			errno = ENOMEM;
			return NULL;
		}

	}

	// Decrease free space available
	stream->free_space -= size;

	// Return pointer where the program can start writing
	return (stream->buf + stream->offset);
}


/*****************************************************************************/
int
dag_get_stream_buffer_level (int dagfd, int stream_num)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	volatile dagpbm_stream_t *pbm = stream->pbm;
	int level = 0;
	volatile uint32_t * l_record_ptr = NULL;		// Record pointer
	volatile uint32_t * l_limit_ptr = NULL;		// Limit pointer
	volatile uint32_t * l_mem_addr;		// physical address to check if the stream is configured
	volatile uint32_t * l_stream_size;		// pointer to the size of the memory hole (stream size)
	dagpbm_MkI_t * pbm_MkI = 0;
	dagpbm_stream_MkII_t * pbm_MkII = 0;
	dagpbm_stream_MkIII_t * pbm_MkIII = 0;
	uint32_t regn=0;
	dag_reg_t result[DAG_REG_MAX_ENTRIES];
	int l_stream_num = stream_num;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	// If the card has an ARM processor, the buffer level can be
	// calculated keeping track of offsets (see dag_advance). We
	// don't support ARM processors in this function.
	if (herd[dagfd].has_arm) {
		errno = EINVAL;
		return -1;
	}

	if(stream->attached) {
		if (pbm && !PBM_CONFIGURED(pbm)) {
			errno = ENOMEM;
			return -1;
		}
		
		l_record_ptr    = pbm->record_ptr;
		l_limit_ptr     = pbm->limit_ptr;
		l_mem_addr      = pbm->mem_addr;
		l_stream_size   = pbm->mem_size;
		
	} else {
		// If stream is not attached, then pbm may not be initialised
		
		// Look for DAG card PBM (Pci Burst Manager)
		if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_PBM, result, &regn)) {
			errno = EIO;
			goto fail;
		}
		
		// Do we have PBM if not the level will not work
		if(regn) {
			
			// Which version of the burst manager are we using?
			switch (result->version) {
			case 0: // Version 0 (MkI)
				pbm_MkI = (dagpbm_MkI_t *) (herd[dagfd].iom + result->addr + (stream_num * 0x60));
				
				// Initialize the pointers to acces the limit and record pointers
				l_record_ptr    = &(pbm_MkI->record_ptr);
				l_limit_ptr     = &(pbm_MkI->limit_ptr);
				l_mem_addr      = &(pbm_MkI->mem_addr);
				l_stream_size   = &(pbm_MkI->mem_size);
				
				
				break;
				
			case 1:	// Version 1 (MkII)
				pbm_MkII = (dagpbm_stream_MkII_t *) (herd[dagfd].iom + result->addr + 0x40 + (stream_num * 0x40));
				
				// Initialize interface structure
				if (herd[dagfd].stream[stream_num].reverse_mode) {
					l_record_ptr    = &(pbm_MkII->limit_ptr);
					l_limit_ptr     = &(pbm_MkII->record_ptr);
				} else 	{
					l_record_ptr    = &(pbm_MkII->record_ptr);
					l_limit_ptr     = &(pbm_MkII->limit_ptr);
				}
				l_mem_addr      = &(pbm_MkII->mem_addr);
				l_stream_size   = &(pbm_MkII->mem_size);
				break;
				
			case 2: // Version 2 (MkIII) CSBM
			case 3: // Version 3 HSBM ( use the similar pbm stream structure 
				pbm_MkIII = (dagpbm_stream_MkIII_t *) (herd[dagfd].iom + result->addr + 0x40 + (stream_num * 0x40));
				
				// Initialize interface structure
				l_record_ptr      = &(pbm_MkIII->record_ptr);
				l_limit_ptr      = &(pbm_MkIII->limit_ptr);
				l_mem_addr      = &(pbm_MkIII->mem_addr);
				l_stream_size   = &(pbm_MkIII->mem_size);
				
				break;
				
			default: // Not implemented at this time
//			errno = EINVAL;
//			return_val = (int) MAP_FAILED;
				goto fail;
			}
			
			// Is PBM for this stream unconfigured? -> fail if not 
			if(!( (( *(l_mem_addr) & 0xfffffff0) != 0xfffffff0) && (*(l_mem_addr) != 0x0) )) {
				errno = ENOMEM;
				goto fail;
			}
		} else
		{
			return -1;
		}	
	}
	
	if (herd[dagfd].stream[stream_num].reverse_mode) {
		l_stream_num ^= 0x01;
	}

	// Receive and Transmit use different calculations
	if (l_stream_num & 1) {

		// This is a transmit stream
		level = *(l_limit_ptr) - *(l_record_ptr);

	} else {

		// This is a receive stream
		level = *(l_record_ptr) - *(l_limit_ptr);
	}
	if (level < 0) {
		 level += *(l_stream_size);
	}

	return level;
fail:
		return -1;
}

/*****************************************************************************/
int
dag_get_stream_last_buffer_level (int dagfd, int stream_num)
{
	stream_t * stream = &(herd[dagfd].stream[stream_num]);
	int level = 0;

	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}

	if(!stream->attached) {
		errno = EINVAL;
		return -1;
	}

	// If the card has an ARM processor, the buffer level can be
	// calculated keeping track of offsets (see dag_advance). We
	// don't support ARM processors in this function.
	if (herd[dagfd].has_arm) {
		errno = EINVAL;
		return -1;
	}

	// Receive and Transmit use different calculations
	if (stream_num & 1) {

		// This is a transmit stream
		level = stream->last_bus_bottom - stream->last_bus_top;

	} else {

		// This is a receive stream
		level = stream->last_bus_top - stream->last_bus_bottom;
	}
	if (level < 0) level += stream->size;

	return level;
}

/**********************************************************************************/
/* Function to control the modes it sets a flag for the stream and later is used  */
/**********************************************************************************/

int dag_set_mode(int dagfd, int stream_num, uint32_t mode) {
	
	if (!herd[dagfd].opened) {
		errno = EBADF;
		return -1;
	}


	// Check if stream number is within DAG capabilities
	if (stream_num & 1) {
		// This is a transmit stream
		if (herd[dagfd].num_tx_streams <= ((stream_num - 1) / 2)) {
			errno = EINVAL;
			return (intptr_t) MAP_FAILED;
		}
	} else {
		// This is a receive stream
		if (herd[dagfd].num_rx_streams <= (stream_num / 2)) {
			errno = EINVAL;
			return (intptr_t) MAP_FAILED;
		}
	}
	
	//sets the mode 0 is normal mode 
	//1 or anything else is Reverse mode
	herd[dagfd].stream[stream_num].reverse_mode = mode;
	return 0;

};


/*****************************************************************************/
/* main function is to init the Burst manager structures and read out how many stream are 
presented on the card */
/*****************************************************************************/
static int
dag_update(int dagfd)
{
	uint32_t regn = 0;
	dag_reg_t result[DAG_REG_MAX_ENTRIES];
	dagpbm_global_t * pbm;
	dagpbm_MkI_t * pbm_MkI;
	dagpbm_global_MkII_t * pbm_MkII;
    dagpbm_global_MkIV_t *pbm_MkIV;

#if defined(_WIN32)

	ULONG BytesTransfered;

	if(DeviceIoControl(herd[dagfd].handle,
		IOCTL_GET_DAGINFO,
		NULL,
		0,
		&herd[dagfd].daginf,
		sizeof(daginf_t),
		&BytesTransfered,
		NULL) == FALSE)
		return -1;

#elif defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	if(ioctl(dagfd, DAGIOCINFO, &herd[dagfd].daginf) < 0)
		return -1;

#endif /* Platform-specific code. */

	if(dag_reg_find((char*) herd[dagfd].iom, 0, herd[dagfd].regs)<0) {
		errno = EIO;
		return -1;
	}

	// If the sheep has already a dagpbm structure, use it
	// otherwise create a new one
	if (herd[dagfd].pbm)
		pbm = herd[dagfd].pbm;
	else
		pbm = (dagpbm_global_t *) dagutil_malloc_aligned( sizeof(dagpbm_global_t));


	// Look for DAG card PBM (Pci Burst Manager)
	if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_PBM, result, &regn)<0) {
		errno = EIO;
		return -1;
	}

	// Do we have PBM?
	if(regn) {

		// Which version of the burst manager are we using?
		switch (result->version) {
		case 0: // Version 0 (MkI)

			pbm_MkI = (dagpbm_MkI_t *) (herd[dagfd].iom + result->addr);

			// Initialize interface structure
			pbm->status          = &(pbm_MkI->status);
			pbm->burst_threshold = &(pbm_MkI->burst_threshold);
			pbm->burst_timeout   = &(pbm_MkI->burst_timeout);

			// Bind the structure to the stream
			herd[dagfd].pbm = pbm;

			// We know for sure we have one receive stream
			herd[dagfd].num_rx_streams = 1;

			// But do we have transmit capabilities?
			regn = 0;
			if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_TERF64, result, &regn)<0) {
				errno = EIO;
				return -1;
			}

			if(!regn) {
				if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_RAW_TX, result, &regn)<0) {
					errno = EIO;
					return -1;
				}
			}

        	if(!regn) {
				if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_E1T1_HDLC_MAP, result, &regn)<0) {
					errno = EIO;
					return -1;
				}
			}

            if(!regn) {
				if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_E1T1_ATM_MAP, result, &regn)<0) {
					errno = EIO;
					return -1;
				}
			}

			if (regn) {
				// Yes, we have transmit firmware
				// But only one transmit stream
				herd[dagfd].num_tx_streams = 1;
				herd[dagfd].has_tx = 1;
			} else {
				// No, we don't have transmit firmware
				herd[dagfd].num_tx_streams = 0;
				herd[dagfd].has_tx = 0;
			}

			break;

		case 1:	// Version 1 (MkII)
		case 2: // Version 2 (MkIII) CSBM
			pbm_MkII = (dagpbm_global_MkII_t *) (herd[dagfd].iom + result->addr);

			// Initialize interface structure
			pbm->status          = &(pbm_MkII->status);
			pbm->burst_threshold = &(pbm_MkII->burst_threshold);
			pbm->burst_timeout   = &(pbm_MkII->burst_timeout);

			// Bind the structure to the stream
			herd[dagfd].pbm = pbm;

			// Get the number of transmit and receive streams
			herd[dagfd].num_tx_streams = (uint8_t)(( *(pbm->status) >> 24 ) & 0xf);
			herd[dagfd].num_rx_streams = (uint8_t)(( *(pbm->status) >> 20 ) & 0xf);

			// So, do we have transmit capabilities?
			if (herd[dagfd].num_tx_streams > 0)	
				herd[dagfd].has_tx = 1;
			else
				herd[dagfd].has_tx = 0;

			break;
        case 3: // Version 3 (MkIV) HSBM
			pbm_MkIV = (dagpbm_global_MkIV_t *) (herd[dagfd].iom + result->addr);

			// Initialize interface structure
			pbm->status          = &(pbm_MkIV->status);
			//pbm->burst_threshold = &(pbm_MkII->burst_threshold);
			pbm->burst_timeout   = &(pbm_MkIV->burst_timeout);

			// Bind the structure to the stream
			herd[dagfd].pbm = pbm;

			// Get the number of transmit and receive streams
			herd[dagfd].num_rx_streams = (uint16_t)((pbm_MkIV->stream_counts) & 0xfff);
			herd[dagfd].num_tx_streams = (uint16_t)((pbm_MkIV->stream_counts >> 16 ) & 0xfff);

			// So, do we have transmit capabilities?
			if (herd[dagfd].num_tx_streams > 0)	
				herd[dagfd].has_tx = 1;
			else
				herd[dagfd].has_tx = 0;
            break;
		default: // Not implemented at this time
		errno = EIO;
			return -1;
		}
	}

	/* Check for ARM processor */
	regn = 0;
	if(dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_ARM, result, &regn)<0) {
		errno = EIO;
		return -1;
	}

	/* Only check the first pbm */
	if (regn) {
		// The ARM processor based cards offer only a receive stream
		herd[dagfd].has_arm = 1;
		herd[dagfd].num_rx_streams = 1;
		herd[dagfd].num_tx_streams = 0;
		herd[dagfd].has_tx = 0;
	} else {
		herd[dagfd].has_arm = 0;
	}

	return 0;
}

#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
/*****************************************************************************/
/*
 * I wish there was a better way, by means of a clone ioctl() in the
 * kernel, but it appears to be more difficult and also OS specific,
 * so here is the second best and also portable version.
 */
int
dag_clone(int dagfd, int minor)
{
	regex_t		reg;
	regmatch_t	match;
	char		buf[16];
	char		*fmt[DAGMINOR_MAX] = {
				"/dev/dag%u",		/* DAGMINOR_DAG */
				"/dev/dagmem%u", 	/* DAGMINOR_MEM */
				"/dev/dagiom%u",	/* DAGMINOR_IOM */
				"/dev/dagarm%u",	/* DAGMINOR_ARM */
				/*"/dev/dagram%u",	 DAGMINOR_RAM */
			};
	int		r;

	if(minor >= DAGMINOR_MAX) {
		errno = ENODEV;
		return -1;
	}
	if(regcomp(&reg, "/dev/dag(iom|mem|arm|ram)*[0-9]", REG_EXTENDED) != 0) {
		errno = EIO; /* static regex compilation failed? */
		return -1;
	}
	if((r = regexec(&reg, herd[dagfd].dagname, 1, &match, 0)) !=0) {
		errno = ENODEV;
		return -1;
	}
	/* the dag index starts at  (match.rm_eo-1). So use atoi to get the index as an integer*/
	(void)snprintf(buf, 16,fmt[minor], atoi(((char*)(herd[dagfd].dagname)+(match.rm_eo-1))));
	buf[15] = '\0';
	regfree(&reg);

	return open(buf, O_RDWR);
}

#endif

int
dag_parse_name(const char* name, char* buffer, int buflen, int* stream_number)
{
	uint32_t device_number = 0;
	int result;
	const char* device;
	const char* stream;
	char tokbuf[16];
	assert(NULL != buffer);
	assert(NULL != name);
	assert(NULL != stream_number);
	assert(0 != buflen);

	/* Check parameters. */
	if ((NULL == name) || (NULL == buffer) || (NULL == stream_number) || (buflen < 16))
	{
		errno = EINVAL;
		return -1;
	}

	/* Initialise results. */
	buffer[0] = '\0';
	*stream_number = 0;
	
	/* Support specifications in the forms /dev/dag0:3, dag0:3, 0:3 */
	device = NULL;
	if ((0 != strcmp("", name)) && (0 != strlen(name)))
	{
		strncpy(tokbuf, name, 16);
		device = strtok(tokbuf, ":\n");
	}

	if ((NULL == device) || (0 == strlen(device)))
	{
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

		strncpy(buffer, "/dev/dag0", buflen);

#elif defined(_WIN32)

		strncpy(buffer, "\\\\.\\dag0", buflen);

#endif /* Platform-specific code. */

		return 0;
	}
	
	/* 'device' contains everything up to any ':' */
	result = 0;
	if (device[0] == '/')
	{
		/* Compare against /dev/dagN */
		if (NULL == strstr(device, "/dev/dag"))
		{
			result = -1;
		}
	}
	else if (device[0] == 'd')
	{
		/* Compare against dagN */
		if (NULL == strstr(device, "dag"))
		{
			result = -1;
		}
	}
	
	if (0 == result)
	{
		/* Look for device number. Using tmp_str to parse through the string*/
        const char *tmp_str = device ;
        /* first just skip till we find a digit ( The valid string here could be /dev/dagN or dagN or only N) */
        while( *tmp_str  && !isdigit(*tmp_str) )
            tmp_str++;
        if (*tmp_str)
        {
            /* here we have a digit in tmp_str - convert integer */
            while( isdigit(*tmp_str))
            {
                device_number = ( device_number * 10 ) + (*tmp_str - '0');
                tmp_str++;
            }
            
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

            snprintf(buffer, buflen, "/dev/dag%u", device_number);

#elif defined(_WIN32)

            snprintf(buffer, buflen, "\\\\.\\dag%u", device_number);

#endif /* Platform-specific code. */
		}
		else
		{
            /* we have hit '\0' without seeing an integer- Report error */
			result = -1;
		}
	}
	
	if (-1 == result)
	{
		/* Localized error handling without resorting to goto statements. */
		errno = EINVAL;
		return -1;
	}
	
	/* See if there was a stream number. */
	stream = strtok(NULL, " \t:\n");
	if (NULL != stream)
	{
		/* There was an explicit stream number. */
		if (isdigit(stream[0]))
		{
			*stream_number = (int) atoi(stream);
		}
	}

	return 0;
}

int
dag_set_param(int dagfd, int stream_num, uint32_t param, void *value)
{
	int i = 0;
	int retval = -1;
	dag_reg_t	result[DAG_REG_MAX_ENTRIES];
	uint32_t	regn=0;


	switch(param)
	{
	case DAG_FLUSH_RECORDS:
		/* Ensure the register that holds the DSMU interface is available */
		if(!herd[dagfd].opened) {
			errno = EBADF;
			return -1;
		}

		if ((dag_reg_table_find(herd[dagfd].regs, 0, DAG_REG_DSMU, result, &regn))
	    || (regn < 1)) {
			errno = EIO;
			return -1;
		}
		herd[dagfd].dsmu = (uint32_t * ) (herd[dagfd].iom + DAG_REG_ADDR(*result));


		/* Check for entire card or single stream set up */
		if(stream_num != -1) {
			herd[dagfd].stream[stream_num].pad_flush = *(uint32_t*)value;
		}
		else {
			for( i = 0; i < herd[dagfd].num_rx_streams; i++ ) {
				herd[dagfd].stream[i*2].pad_flush = *(uint32_t*)value;
			}
		}
		retval = 0;

		break;

	default:
		errno = EINVAL; 
		break;

	}

	return retval;
}


int
dag_get_last_error()
{
	return errno;
}

char *
dag_getname(int dagfd)
{
	return herd[dagfd].dagname;
}

#if defined(_WIN32)

/* Provide a panic() function for everything else */
void
panic(char *fmt, ...)
{
	va_list ap;

	(void)fprintf(stderr, "panic: ");
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}
/* 
 * Return the handle associated with a sheep. Under Windows it's not possible to have a direct
 * association between a dagfd and its file handle, so in some cases the user needs to explicitly 
 * request the handle
 */
HANDLE
dag_gethandle(int dagfd)
{
	return herd[dagfd].handle;
}


#endif /* _WIN32 */




