/*
 * Copyright (c) 2002-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagutil.c,v 1.66.2.3 2009/09/24 04:47:13 darren.lim Exp $
 */

/* Routines that are generally useful throughout the programs in the tools directory. 
 * Subject to change at a moment's notice, not supported for customer applications, etc.
 */

/* File header. */
#include "dagutil.h"

/* Endace headers. */
#include "dagapi.h"

#if defined(__sun)
#include <asm/clock.h>	
#endif 

//NOTE MAYBE TODO: 
//get_cycles should be as good a ltiile bit problematic on CPU migration
//but that should not be a problem due the loops are busy  
//ADD the function to map and adjust the __MM_CLOCK_DEV 
#ifdef __ia64__
//#include <asm/timex.h>
static unsigned     long rdtsc_ia64()
{

    volatile unsigned long temp = 0;
    __asm__     __volatile__ ("mov %0=ar.itc" : "=r"(temp) :: "memory");
    return    temp;

}


/* 
#include <sys/ioctl.h>
#include "mmtimer.h"
int mmtimer_fd;

unsigned long __mm_timer_clock_res;
unsigned long *__mm_clock_dev;
unsigned long __mm_clock_offset;
*/
#endif


/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: dagutil.c,v 1.66.2.3 2009/09/24 04:47:13 darren.lim Exp $";
#if (__FreeBSD__ == 4)
#define sem_timedwait(a,b) sem_wait(a)
#define pthread_mutex_timedlock(a,b) pthread_mutex_lock(a)
#endif



static pbm_offsets_t uPbmOffset[4] =
{
	/* pbm v0 */
	{
		/*  globalstatus:*/     0x1c,
		/*  streambase:*/       0x0,
		/*  streamsize:*/       0x60,
		/*  streamstatus:*/     0x1c,
		/*  mem_addr:*/         0x0,
		/*  mem_size:*/         0x04,
		/*  record_ptr:*/       0x18,
		/*  limit_ptr:*/        0x10,
		/*  safetynet_cnt:*/    0x14,
		/*  drop_cnt:*/         0x0c
	},
	
	/* pbm v1 */
	{
		/*  globalstatus:*/     0x0,
		/*  streambase:*/       0x40,
		/*  streamsize:*/       0x40,
		/*  streamstatus:*/     0x0,
		/*  mem_addr:*/         0x04,
		/*  mem_size:*/         0x08,
		/*  record_ptr:*/       0x0c,
		/*  limit_ptr:*/        0x10,
		/*  safetynet_cnt:*/    0x14,
		/*  drop_cnt:*/         0x18
	},

	/* pbm v2 */
	{
		/*  globalstatus:*/     0x0,
		/*  streambase:*/       0x40,
		/*  streamsize:*/       0x40,

		/*  streamstatus:*/     0x30,
		/*  mem_addr:*/         0x00,
		/*  mem_size:*/         0x08,
		/*  record_ptr:*/       0x18,
		/*  limit_ptr:*/        0x20,
		/*  safetynet_cnt:*/    0x28,
		/*  drop_cnt:*/         0x38
	},
    /* pbm v3 - same as v2*/
	{
		/*  globalstatus:*/     0x0,
		/*  streambase:*/       0x40,
		/*  streamsize:*/       0x40,

		/*  streamstatus:*/     0x30,
		/*  mem_addr:*/         0x00,
		/*  mem_size:*/         0x08,
		/*  record_ptr:*/       0x18,
		/*  limit_ptr:*/        0x20,
		/*  safetynet_cnt:*/    0x28,
		/*  drop_cnt:*/         0x38
	}
};



#if  defined(__SVR4) && defined(__sun)
/* This will work only on intel based systems */
  #define CUSTOM_SLEEP_CODE 1
#elif  (defined(__APPLE__) && defined(__ppc__)) 
# define CUSTOM_SLEEP_CODE 0
#else
# define CUSTOM_SLEEP_CODE 1
#endif /* CUSTOM_SLEEP_CODE */

#if CUSTOM_SLEEP_CODE
/* FIXME: Relies on the Pentium Time Stamp Counter.  This code will break on:
 *     - x86 CPUs less than 586-class,
 *     - non-x86 CPUs (PowerPC, SPARC)
 *     - 586-class (or higher) CPUs with the Pentium Time Stamp Counter disabled.
 *
 * To do this properly, the CPU architecture should be detected in the configure
 * script and then appropriate definitions provided for each supported CPU.
 *
 * Or we give up on dagutil_nanosleep() as being a bad idea and use POSIX nanosleep() instead.
 */
#if defined(_WIN32)
__int64 rdtscll()
{
	__int64 cycles;
	_asm rdtsc; // won't work on 486 or below - only pentium or above
	_asm lea ebx,cycles;
	_asm mov [ebx],eax;
	_asm mov [ebx+4],edx;
	return cycles;
}

#elif (defined(__SVR4) && defined(__sun)) || (__linux__) || defined (__NetBSD__)
# if __WORDSIZE == 64
#  ifdef __ia64__
/* Itanium implementation for 64bits  */
#define rdtscll(val) do { \
       (val) = rdtsc_ia64(); \
      } while(0)   
/*
#   define rdtscll(val)					\
    do {							\
        val = *__mm_clock_dev;				\
       } while (0)
*/       
#  else
/* x86 Implementation */
#   define rdtscll(val) do { \
      unsigned int __a,__d; \
      asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
      (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
      } while(0)
#  endif 

# else
#  ifdef __ia64__
#   error not implemented for Itanium 32 bit contact support@endace.com
#  else
#   define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))
#  endif 
# endif

#elif defined(__FreeBSD__)

# include <machine/cpufunc.h>
# define rdtscll(X) (X=rdtsc())

#endif

#endif /* CUSTOM_SLEEP_CODE */

static const char* progname = "noname";
static int verbosity = 0;
static int use_syslog = 0;
#define SYSLOG_BUFSIZE 1024

#if CUSTOM_SLEEP_CODE
static uint32_t thiscpu = 0;

/* Internal routines. */
static void getcpuspeed(void);


/* Implementation of internal routines. */
static void
getcpuspeed(void)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))
	struct timeval before;
	struct timeval after;
	struct timeval diff;
	uint64_t tsc1 = 0;
	uint64_t tsc2 = 0;
	uint64_t tdiff;
	uint64_t temp;

	gettimeofday(&before, NULL);
	rdtscll(tsc1);

	usleep(10*1000);

	gettimeofday(&after, NULL);
	rdtscll(tsc2);

	timersub(&after, &before, &diff);
	temp = (uint64_t) diff.tv_sec * 1000 * 1000 + diff.tv_usec;

	tdiff = tsc2 - tsc1;

	thiscpu = tdiff / temp;

#elif defined(_WIN32)

	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);

	thiscpu = freq.LowPart/1000000;

#endif /* Platform-specific code. */
}
#endif /* CUSTOM_SLEEP_CODE */


void
dagutil_set_progname(const char* program_name)
{
	progname = strrchr((char*) program_name, '/');
	if (progname == NULL)
	{
		progname = program_name;
	}
	else
	{
		progname++;
	}
}


const char*
dagutil_get_progname(void)
{
	return progname;
}


void
dagutil_set_signal_handler(void (*handler)(int))
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	struct sigaction sigact;

	/*
	 * Catching most common signals to provide for a graceful
	 * shutdown of the card, worst case the driver will have
	 * to clean up after a crash.
	 */
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	if (sigaction(SIGHUP, &sigact, NULL) < 0)
	{
		dagutil_panic("sigaction SIGHUP: %s\n", strerror(errno));
	}

	if (sigaction(SIGINT, &sigact, NULL) < 0)
	{
		dagutil_panic("sigaction SIGINT: %s\n", strerror(errno));
	}

	if (sigaction(SIGTERM, &sigact, NULL) < 0)
	{
		dagutil_panic("sigaction SIGTERM: %s\n", strerror(errno));
	}

	if (sigaction(SIGPIPE, &sigact, NULL) < 0)
	{
		dagutil_panic("sigaction SIGPIPE: %s\n", strerror(errno));
	}

	if (sigaction(SIGQUIT, &sigact, NULL) < 0)
	{
		dagutil_panic("sigaction SIGQUIT: %s\n", strerror(errno));
	}

#else

	/* Note: in Windows SIGHUP, SIGPIPE, and SIGQUIT do not exist. */
	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	
#endif /* Platform-specific code. */
}


void
dagutil_set_timer_handler(void (*handler)(int), uint32_t seconds)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	struct sigaction sigact;
	struct itimerval itv;

	/*
	 * Set up a timer to expire every 'seconds', for reporting.
	 */
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = handler;
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sigact, NULL) < 0)
	{
		dagutil_panic("sigaction SIGALRM: %s\n", strerror(errno));
	}

	memset(&itv, 0, sizeof(itv));
	itv.it_interval.tv_sec = seconds;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &itv, NULL) < 0)
	{
		dagutil_panic("setitimer ITIMER_REAL %u second: %s\n", seconds, strerror(errno));
	}
#endif
}


void
dagutil_set_verbosity(int v)
{
	verbosity = v;
}


int
dagutil_get_verbosity(void)
{
	return verbosity;
}


void
dagutil_inc_verbosity(void)
{
	verbosity++;
}


void
dagutil_dec_verbosity(void)
{
	verbosity--;
}


void
dagutil_panic(const char *fmt, ...)
{
	va_list ap;
	char buf[SYSLOG_BUFSIZE];

	if (use_syslog) {
		va_start(ap, fmt);
		vsnprintf(buf, SYSLOG_BUFSIZE, fmt, ap);
		va_end(ap);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		syslog(LOG_ERR, "panic: %s", buf);
#endif
	} else {
		(void) fprintf(stderr, "%s: panic: ", progname);
		
		va_start(ap, fmt);
		(void) vfprintf(stderr, fmt, ap);
		va_end(ap);
	}	
	exit(EXIT_FAILURE);
}


void
dagutil_error(const char *fmt, ...)
{
	va_list ap;
	char buf[SYSLOG_BUFSIZE];

	if (verbosity < 0)
		return;
	
	if (use_syslog) {
		va_start(ap, fmt);
		vsnprintf(buf, SYSLOG_BUFSIZE, fmt, ap);
		va_end(ap);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		syslog(LOG_ERR, "error: %s", buf);
#endif
	} else {
		(void) fprintf(stderr, "%s: error: ", progname);
		
		va_start(ap, fmt);
		(void) vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}


void
dagutil_warning(const char *fmt, ...)
{
	va_list ap;
	char buf[SYSLOG_BUFSIZE];


	if (use_syslog) {
		va_start(ap, fmt);
		vsnprintf(buf, SYSLOG_BUFSIZE, fmt, ap);
		va_end(ap);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		syslog(LOG_WARNING, "warning: %s", buf);
#endif
	} else {
		(void) fprintf(stderr, "%s: warning: ", progname);
		
		va_start(ap, fmt);
		(void) vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}


void
dagutil_verbose(const char *fmt, ...)
{
	va_list ap;

	if (verbosity < 1)
		return;
	
	if (use_syslog) {
		va_start(ap, fmt);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		syslog(LOG_INFO, fmt, ap);
#endif
		va_end(ap);
	} else {
		(void) fprintf(stderr, "%s: verbose: ", progname);
		
		va_start(ap, fmt);
		(void) vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}


void
dagutil_verbose_level(uint32_t level, const char *fmt, ...)
{
	va_list ap;
	
	if (verbosity < level)
		return;
	
	if (use_syslog) {
		va_start(ap, fmt);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		syslog(LOG_INFO, fmt, ap);
#endif
		va_end(ap);
	} else {
		(void) fprintf(stderr, "%s: verbose: ", progname);
		
		va_start(ap, fmt);
		(void) vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}


void
dagutil_msg_level(uint32_t level, const char *fmt, ...)
{
	va_list ap;
	
	if (verbosity < level)
		return;
	
	if (use_syslog) {
		va_start(ap, fmt);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		syslog(LOG_INFO, fmt, ap);
#endif
		va_end(ap);
	} else {
		va_start(ap, fmt);
		(void) vfprintf(stdout, fmt, ap);
		va_end(ap);
	}
}


void
dagutil_daemon(int option, int facility)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	openlog(progname, option, facility);
	use_syslog = 1;
#endif
}


void dagutil_sleep_get_wake_time(unsigned long long * wake_time,int microseconds)
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;
#endif
#if CUSTOM_SLEEP_CODE
	/*wake_time is a tsc value */

	int64_t tsc;
	if (0 == thiscpu)
		getcpuspeed();
#if defined(_WIN32)

	QueryPerformanceCounter(&counter);
	tsc = counter.HighPart;
	tsc <<= 32;
	tsc |= counter.LowPart;
#else
	rdtscll(tsc);
#endif

	*wake_time = tsc + (int64_t)(thiscpu) * (int64_t)microseconds; 
#else /* use system time calls only */

	/* wake_time is a system microsecond time */
	struct timeval now;
	
	gettimeofday(&now, NULL);
	
	*wake_time = (now.tv_sec*1000000 + now.tv_usec + microseconds);

#endif /* CUSTOM_SLEEP_CODE */
}


void dagutil_sleep_until(unsigned long long wake_time)
{
#if CUSTOM_SLEEP_CODE
	/*wake_time is a tsc value */

	int64_t tsc = 0;
#if defined(_WIN32)
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;

	QueryPerformanceCounter(&counter);
	tsc = counter.HighPart;
	tsc <<= 32;
	tsc |= counter.LowPart;
#else
		rdtscll(tsc);
#endif

	while (tsc < wake_time)
	{

#if defined(_WIN32)
		QueryPerformanceCounter(&counter);
		tsc = counter.HighPart;
		tsc <<= 32;
		tsc |= counter.LowPart;
#else
		rdtscll(tsc);
#endif

	}

#else /* use system time calls only */
	struct timeval now;
	unsigned long long now_usec;
	
	gettimeofday(&now, NULL);
	now_usec = (now.tv_sec*1000000 + now.tv_usec);

	while (now_usec < wake_time) {
		gettimeofday(&now, NULL);
		now_usec = (now.tv_sec*1000000 + now.tv_usec);
	}

#endif /* CUSTOM_SLEEP_CODE */

}


/* Same as dagutil_sleep_until() but does not busywait.
   CPU usage will be considerably lower, but accuracy MUCH worse!
   usleep(1) should cause a sleep of at least 1usec,
   generally equal to system scheduler time unit (typ 10ms).
*/
void dagutil_sleep_until_nobusy(unsigned long long wake_time)
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;
#endif

#if CUSTOM_SLEEP_CODE
	/*wake_time is a tsc value */

	int64_t tsc = 0;
#if defined(_WIN32)
	QueryPerformanceCounter(&counter);
	tsc = counter.HighPart;
	tsc <<= 32;
	tsc |= counter.LowPart;
#else
		rdtscll(tsc);
#endif

	while (tsc < wake_time)
	{
		usleep(1);

#if defined(_WIN32)
		QueryPerformanceCounter(&counter);
		tsc = counter.HighPart;
		tsc <<= 32;
		tsc |= counter.LowPart;
#else
		rdtscll(tsc);
#endif
	}

#else /* use system time calls only */
	struct timeval now;
	unsigned long long now_usec;
	
	gettimeofday(&now, NULL);
	now_usec = (now.tv_sec*1000000 + now.tv_usec);

	while (now_usec < wake_time) {
		usleep(wake_time - now_usec);

		gettimeofday(&now, NULL);
		now_usec = (now.tv_sec*1000000 + now.tv_usec);
	}

#endif /* CUSTOM_SLEEP_CODE */

}


void
dagutil_microsleep(uint32_t microseconds)
{
#if CUSTOM_SLEEP_CODE
	int64_t tsc = 0;
	int64_t stop;
	int64_t hold;
#if defined(_WIN32)
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;
#endif
	if (0 == thiscpu)
		getcpuspeed();

#if defined(_WIN32)
	//tsc = rdtscll();    
    QueryPerformanceCounter(&counter);
    tsc = counter.HighPart;
    tsc <<= 32;
    tsc |= counter.LowPart;
#else
	rdtscll(tsc);
#endif
	hold = (int64_t)thiscpu * (int64_t)microseconds; 
	stop = tsc + hold;
	while (tsc < stop)
	{

#if defined(_WIN32)
        QueryPerformanceCounter(&counter);
        tsc = counter.HighPart;
        tsc <<= 32;
        tsc |= counter.LowPart;
		//tsc = rdtscll();
#else
		rdtscll(tsc);
#endif

	}
#else
	/*usleep should not be used here as it does not busywait and can be inaccurate.
	  The commented code below performs the correct accurate busywait.
	  User code that wants to busywait calls this function, if it doesn't want to
	  busywait it should call usleep directly */
	usleep(microseconds);
#if 0
 /* use system time calls only */
	struct timeval now;
	unsigned long long wake_usec, now_usec;
	
	gettimeofday(&now, NULL);
	wake_usec = (now.tv_sec*1000000 + now.tv_usec + microseconds);
	now_usec = (now.tv_sec*1000000 + now.tv_usec);

	while (now_usec < wake_usec) {
		gettimeofday(&now, NULL);
		now_usec = (now.tv_sec*1000000 + now.tv_usec);
	}
#endif /* 0 */

#endif /* CUSTOM_SLEEP_CODE */
}


void
dagutil_nanosleep(uint32_t nanoseconds)
{
#if CUSTOM_SLEEP_CODE
	int64_t tsc = 0;
	int64_t stop;
#if defined(_WIN32)
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;
#endif
	if (0 == thiscpu)
		getcpuspeed();

#if defined(_WIN32)
	//tsc = rdtscll();
    QueryPerformanceCounter(&counter);
    tsc = counter.HighPart;
    tsc <<= 32;
    tsc |= counter.LowPart;
#else
	rdtscll(tsc);
#endif

	stop = tsc + ((int64_t)thiscpu * (int64_t)nanoseconds + 999) / 1000;
	while (tsc < stop)
	{
#if defined(_WIN32)
		//tsc = rdtscll();
        QueryPerformanceCounter(&counter);
        tsc = counter.HighPart;
        tsc <<= 32;
        tsc |= counter.LowPart;
#else
		rdtscll(tsc);
#endif
	}

#else

	struct timespec sleep_time = { .tv_sec = 0, .tv_nsec = nanoseconds };
	
	if (-1 == nanosleep(&sleep_time, NULL))
	{
		/* Call failed, try usleep() instead. */
		usleep(1 + (nanoseconds / 1000));
	}
#endif /* CUSTOM_SLEEP_CODE */
}

void dagutil_tsc64_read(uint64_t* tsc)
{
#if defined(_WIN32)
	*tsc = rdtscll();
#elif defined (__sun)
//CHECKME:
        *tsc = tsc_read();
#elif !defined (__ppc__)
	rdtscll((*tsc));
#endif
}


void dagutil_tsc32_read(uint32_t* tsc)
{
#if defined(_WIN32)
	int cycles;
	_asm mov ebx,tsc
	_asm rdtsc; // won't work on 486 or below - only pentium or above
	_asm mov [ebx],eax;
	return;
#elif !defined (__ppc__)
# ifdef __ia64__
//CHECKME for the type casting  from 64>32bit
    rdtscll((*tsc));
# else 
 __asm__ __volatile__("rdtsc" : "=a" (*tsc) : : "edx");
# endif 
#endif
}


/* Borrowed from coffdump.c. */
void*
dagutil_readfile(char *filename, char *flags, unsigned int* out_length)
{
	unsigned int length = 0;
	int len;
	int rlen;
	off_t off;
	FILE* infile;
#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	void* p = NULL;
	void* wp = NULL;

#elif defined(_WIN32)

	char* p = NULL;
	char* wp = NULL;

#endif /* Platform-specific code. */

	infile = fopen(filename, flags);
	if (infile == NULL)
		dagutil_panic("fopen %s: %s\n", filename, strerror(errno));

	for (len = BUFSIZ; ; len += BUFSIZ)
	{
		off = wp - p;
		p = realloc(p, len);
		if (wp == NULL)
		{
			wp = p;
		}
		else
		{
			wp = p + off + BUFSIZ;
		}

		rlen = fread(wp, 1, BUFSIZ, infile);
		length += rlen;

		if (rlen < BUFSIZ)
		{
			break;
		}
	}

	if (ferror(infile))
	{
		dagutil_free(p);
		p = NULL;
		length = 0;
	}

	fclose(infile);

	(*out_length) = length;
	return p;
}

/* 
 * Hooks for memory allocation/disposal routines.
 * Currently these are simple wrappers for malloc/free/strdup, but once all the tools have been converted to use them
 * we could insert code to check for memory leaks, redirect requests to a garbage-collected library (libgc) etc.
 */
void*
dagutil_malloc(size_t size)
{
	return malloc(size);
}

void*
dagutil_malloc_aligned(size_t size)
{
	void *memptr =  NULL;
	int res = 0;
	
#if defined(__linux__)
	res = posix_memalign((void**)&memptr, sysconf(_SC_PAGE_SIZE), size);

	if (res != 0)
	{
		fprintf(stderr,"dagapi: internal allocations failed error in %s line %u\n", __FILE__, __LINE__);
		exit(1);
	}

#elif defined (_WIN32)
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	memptr = _aligned_malloc(size, si.dwPageSize);

#elif (defined(__sun) && defined(__SVR4)) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	memptr = malloc(size);
 
#endif

	return memptr;

};

void
dagutil_free(void* ptr)
{
	free(ptr);
}

void 
dagutil_free_aligned(void* ptr)
{
#if (defined(__sun) && defined(__SVR4)) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	free(ptr);

#elif defined(_WIN32)
	_aligned_free(ptr);

#endif

}

const char*
dagutil_strdup(const char* source)
{
	return strdup(source);
}


coprocessor_t
dagutil_detect_coprocessor(volatile uint8_t* iom)
{
	dag_reg_t registers[DAG_REG_MAX_ENTRIES];

	if (dag_reg_find((char*) iom, DAG_REG_COPRO, registers) < 1)
	{
        if (dag_reg_find((char*) iom, DAG_REG_PPF, registers) < 1) {
            return kCoprocessorNotSupported;
        }
        dagutil_verbose("Found built-in coprocessor\n");
        return kCoprocessorBuiltin;
	}
	else
	{
		uint32_t address = DAG_REG_ADDR(*registers);
		uint32_t safety = 1000;
		uint32_t val;

		*(volatile uint32_t *)(iom + address) = 0;
		while (0 == (*(volatile uint32_t *)(iom + address) & BIT31))
		{
			usleep(1000);

			if (0 == safety)
			{
				dagutil_warning("timed out trying to determine coprocessor type");
				return kCoprocessorUnknown;
			}
			safety--;
		}
		
		/* Read result of type probe */
		val = *(volatile uint32_t *)(iom + address);
		
		switch (val & 0xffff)
		{
			case 0: return kCoprocessorNotFitted;
			case 240: return kCoprocessorPrototype;
			case 255: return kCoprocessorSC128;
			case 254: return kCoprocessorSC256;
			case 253: return kCoprocessorSC256C;
			case 0xff00: return kCoprocessorNotSupported;
			default: return kCoprocessorUnknown;
		}
	}

	return kCoprocessorUnknown;
}



bool 
dagutil_coprocessor_programmed(volatile uint8_t* iom)
{
	dag_reg_t registers[DAG_REG_MAX_ENTRIES];

	if (dag_reg_find((char*) iom, DAG_REG_COPRO, registers) < 1)
	{
		return false;
	}
	else
	{
		uint32_t address = DAG_REG_ADDR(*registers);
		uint32_t safety = 1000;
		uint32_t val;

		*(volatile uint32_t *)(iom + address) = 0;
		while (0 == (*(volatile uint32_t *)(iom + address) & BIT31))
		{
			usleep(1000);

			if (0 == safety)
			{
				dagutil_warning("timed out trying to determine coprocessor type");
				return false;
			}
			safety--;
		}
		
		/* Read result of type probe */
		val = *(volatile uint32_t *)(iom + address);
		if (!(val & (1<<24)))
			return false;
		else 
			return true;
	}
}

pbm_offsets_t*
dagutil_get_pbm_offsets(void)
{
	return &uPbmOffset[0];
}


int
dagutil_lockstream(int dagfd, int stream)
{
	int lock = (stream << 16) | 1; /* lock */

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	if(ioctl(dagfd, DAGIOCLOCK, &lock) < 0)
	{
		return -1; /* errno set accordingly */
	}

#elif defined(_WIN32)

	ULONG BytesTransfered;

	if (DeviceIoControl(dag_gethandle(dagfd),
		IOCTL_LOCK,
		&lock,
		sizeof(lock),
		NULL,
		0,
		&BytesTransfered,
		NULL) == FALSE) 
	{
		panic("DeviceIoControl IOCTL_LOCK\n");
	}

#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */

	return 0;
}


int
dagutil_unlockstream(int dagfd, int stream)
{
	int lock = (stream << 16) | 0; /* unlock */

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	if(ioctl(dagfd, DAGIOCLOCK, &lock) < 0)
	{
		return -1;		/* errno set accordingly */
	}

#elif defined(_WIN32)

	ULONG BytesTransfered;

	if (DeviceIoControl(dag_gethandle(dagfd),
		IOCTL_LOCK,
		&lock,
		sizeof(lock),
		NULL,
		0,
		&BytesTransfered,
		NULL) == FALSE)
	{
		panic("DeviceIoControl IOCTL_LOCK\n");
	}
	
#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */

	return 0;
}


int
dagutil_lockallstreams(int dagfd)
{
	int stream;

	for (stream = 0; stream < DAG_STREAM_MAX; stream++)
	{
		if (0 != dagutil_lockstream(dagfd, stream))
		{
			return -1;
		}
	}
	
	return 0;
}


void
dagutil_unlockallstreams(int dagfd)
{
	int stream;

	for (stream = 0; stream < DAG_STREAM_MAX; stream++)
	{
		if (0 != dagutil_unlockstream(dagfd, stream))
		{
			dagutil_warning("Cannot release lock on stream %d: %s\n", stream, strerror(errno));
		}
	}
}


uint32_t
dagutil_iomread(volatile uint8_t* iom, uint32_t addr)
{
	return *(volatile uint32_t*) (iom + addr);
}


void
dagutil_iomwrite(volatile uint8_t* iom, uint32_t addr, uint32_t value)
{
	*(volatile uint32_t*)(iom + addr) = value;
}

void
dagutil_steerstatus(int dagfd, volatile uint8_t* dagiom, uint32_t base, int ver)
{
	uint32_t val;

	val = dagutil_iomread(dagiom, (uint32_t) (base));

	if (base)
	{
		switch(ver)
		{
			case -1:
				if ((val & BIT18) == 0x0)
				{
					printf("rx\tsteer=stream0");
				}
				else if ((val & BIT18) == BIT18)
				{
					printf("rx\tsteer=iface");
				}
				else
				{
					printf("unexpected steering register value: 0x%08x", val);
				}
				break;
				
			case 0:
				printf("ipf\t");
				/* Coprocessor related status information. */
				printf("%sdrop ", (val & BIT15) ? "" : "no");
				printf("steer=");
	
				switch ((val >> 12) & 0x03)
				{
					case 0x00:
						printf("stream0 ");
						break;
						
					case 0x01:
						printf("colour ");
						break;
						
					case 0x02:
						printf("payload ");
						break;
						
					case 0x03:
						printf("reserved ");
						break;
					
					default:
						dagutil_panic("internal error at line %d\n", __LINE__);
				}
				break;
				
			case 1:
				printf("balance\t");
				/* Coprocessor related status information. */
				printf("%sdrop ", (val & BIT15) ? "" : "no");
				printf("steer=");
	
				switch ((val >> 12) & 0x03)
				{
					case 0x00:
						printf("stream0 ");
						break;
					
					case 0x01:
						printf("parity ");
						break;
					
					case 0x02:
						printf("crc ");
						break;
					
					case 0x03:
						printf("iface ");
						break;
					
					default:
						dagutil_panic("internal error at line %d\n", __LINE__);
				}
				break;
			
			default:
				break;
		}
	
		printf("\n");
	}
}

void
dagutil_pbmstatus(int dagfd, volatile uint8_t* dagiom, uint32_t pbm_base, daginf_t* daginf, int pbm_ver)
{
	uint32_t val, rxstreams, txstreams;
	int c, size;

	if (pbm_base)
	{
		val = dagutil_iomread(dagiom, (uint32_t) (pbm_base + uPbmOffset[pbm_ver].globalstatus));
		printf("pci%s\t", (val & BIT9)?"x":"");
		switch ((val >> 28) & 0xf)
		{
			case 0x0:
				break;
			
			case 0x1:
				printf("33MHz ");
				break;
			
			case 0x2:
				printf("66MHz ");
				break;
			
			case 0x3:
				printf("100MHz ");
				break;
			
			case 0x4:
				printf("133MHz ");
				break;
			
			case 0xe:
				printf("unknownMHz ");
				break;
			
			case 0xf:
				printf("unstableMHz ");
				break;
			
			default:
				dagutil_panic("internal error at line %d\n", __LINE__);
		}
		printf("%s-bit ", (val & BIT5) ? "64" : "32");

		printf("buf=%dMiB ", (daginf->buf_size / ONE_MEBI));

		rxstreams = dag_rx_get_stream_count(dagfd);
		txstreams = dag_tx_get_stream_count(dagfd);

		printf("rxstreams=%d txstreams=%d ", rxstreams, txstreams);

		printf("mem=");
		for (c = 0; c < (signed) (rxstreams + txstreams + abs(rxstreams - txstreams)); c++)
		{
			size = dagutil_iomread(dagiom, (uint32_t) (pbm_base + uPbmOffset[pbm_ver].streambase + c*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_size));
			if (size < 0)
			{
				size = 0;
			}
			
			if (c < (signed) (rxstreams + txstreams + abs(rxstreams - txstreams) - 1))
			{
				printf("%d:", (size / ONE_MEBI));
			}
			else
			{
				printf("%d", (size / ONE_MEBI));
			}
		}

		printf("\n");
	}
}


int
dagutil_pbmdefault(int dagfd, volatile uint8_t* dagiom, uint32_t pbm_base, daginf_t* daginf, int pbm_ver)
{
	int stream = 0;
	int used = 0;
	int retval = 0;
	int rxstreams;
	int txstreams;
	int total_mebibytes;
	int streammax;
	int rxsize;

	total_mebibytes = daginf->buf_size / ONE_MEBI;
	rxstreams = dag_rx_get_stream_count(dagfd);
	txstreams = dag_tx_get_stream_count(dagfd);
	streammax = dagutil_max(rxstreams*2 - 1, txstreams*2);

	if (dagutil_lockallstreams(dagfd))
	{
		retval = EACCES;
		goto exit;
	}

	if (txstreams * PBM_TX_DEFAULT_SIZE + rxstreams > total_mebibytes) /* not enough for recommended config. */
		return ENOMEM;

	rxsize = (total_mebibytes - txstreams * PBM_TX_DEFAULT_SIZE) / rxstreams;
	for (stream = 0; stream < streammax; stream++)
	{
		int stream_mebibytes = 0;
		
		if (stream & 1)
		{
			/* Transmit stream. */
			if (stream-1 < txstreams*2)
			{
				stream_mebibytes = PBM_TX_DEFAULT_SIZE;
			}
		}
		else
		{
			/* Receive stream. */
			if (stream < rxstreams*2)
			{
				stream_mebibytes = rxsize;
			}
		}
		
		/* assignment checks out, set up pbm stream */
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_addr, daginf->phy_addr + used*ONE_MEBI);
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_size, stream_mebibytes*ONE_MEBI);
		
		/* update for next loop */
		used += stream_mebibytes;
	}
	
exit:
	dagutil_unlockallstreams(dagfd);

	return retval;
}


int
dagutil_pbmconfigmem(int dagfd, volatile uint8_t* dagiom, uint32_t pbm_base, daginf_t* daginf, int pbm_ver, const char* cstr)
{
	int requested = 0;
	int retval = 0;
	int stream = 0;
	int used = 0;
	char* tok_state= NULL;
	char* tok;
	int rxstreams;
	int streammax;
	int total;
	int txstreams;
	int val;

	total = daginf->buf_size / ONE_MEBI;
	rxstreams = dag_rx_get_stream_count(dagfd);
	txstreams = dag_tx_get_stream_count(dagfd);
	streammax = dagutil_max(rxstreams*2 - 1, txstreams*2);

	if (dagutil_lockallstreams(dagfd))
	{
		retval = EACCES;
		goto exit;
	}
	
	tok = strtok_r(((char*) cstr)+4, ":", &tok_state);
	while (tok)
	{
		val = (int) strtod(tok, NULL);
		requested += val;
		if (requested > total)
		{
			dagutil_warning("More memory requested (%dMiB) than available (%dMiB)!\n",  requested, total);
			retval = ENOMEM;
			goto exit;
		}
		
		if (stream&1)
		{
			if (val && (stream > txstreams*2))
			{
				dagutil_warning("Invalid assignment of memory to nonexistent tx stream %d!\n", stream);
				retval = ENODEV;
				goto exit;
			}
		}
		else
		{
			if (val && (stream > rxstreams*2))
			{
				dagutil_warning("Invalid assignment of memory to nonexistent rx stream %d!\n", stream);
				retval = ENODEV;
				goto exit;
			}
		}
		
		/* assignment checks out, set up pbm stream */
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_addr, daginf->phy_addr+used*ONE_MEBI);
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_size, val*ONE_MEBI);
		
		/* update for next loop */
		used += val;
		stream++;
		tok = strtok_r(NULL, ":", &tok_state);
	}

	for (; stream < streammax; stream++)
	{
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_addr, 0);
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_size, 0);
	}
		
	if (used < total)
	{
		dagutil_warning("Not all available memory (%dMiB) is assigned (%dMiB)\n", total, used);
	}
	
exit:
	dagutil_unlockallstreams(dagfd);

	return retval;
}
/*New Code for Dag 9.1Rx and Dag 9.1Tx*/

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
int
#elif defined(_WIN32)
HANDLE
#endif
dag91x_open_2nd_core(const char* dagname_1st_core)
{
	/* assumes that the 2nd core is at 1st core -11. eg if /dev/dag0 is the 1st core then /dev/dag1 is assumed to be the 2nd core */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	regex_t preg;
	regmatch_t pmatch[1];
	int dag_num = 0;
	char dagname_2nd_core[256];
	int dag_2nd_core_fd = 0;

	memset(pmatch, 0, sizeof(pmatch));
	memset(&preg, 0, sizeof(preg));
	memset(dagname_2nd_core, 0, sizeof(dagname_2nd_core));
	if (regcomp(&preg, ".*[0-9]", REG_EXTENDED) != 0)
	{
		return -1;
	}
	if (regexec(&preg, dagname_1st_core, 1, pmatch, 0) == REG_NOMATCH)
	{
		return -1;
	}
	if (pmatch[0].rm_so > -1 && pmatch[0].rm_so < strlen(dagname_1st_core))
	{
		dag_num = atoi(&dagname_1st_core[pmatch[0].rm_eo-1]);
		dag_num--;
		snprintf(dagname_2nd_core, 256, "/dev/dag%d", dag_num);
		dag_2nd_core_fd = open(dagname_2nd_core, O_RDWR);
	}
	regfree(&preg);
	return dag_2nd_core_fd;
#elif defined(_WIN32)
    /*
    We don't have a regex library on Windows so we need to do this differently, however we can make a simplifying assumption
    that the name of the device will always be 'dag[0-9]'.
    */
    int dag_num = 0;
    HANDLE dag_2nd_core_fd = 0;
    char dagname_2nd_core[256];
    DWORD last_error;
	LPVOID lpMsgBuf;

    sscanf(dagname_1st_core, "dag%d", &dag_num);
    dag_num--;
    snprintf(dagname_2nd_core, 256, "\\\\.\\dag%d", dag_num);
    dag_2nd_core_fd = CreateFile(
		dagname_2nd_core,			
		GENERIC_READ,		
		FILE_SHARE_READ,					
		NULL,				
		OPEN_EXISTING,		
		0,					
		0);
    if (dag_2nd_core_fd == INVALID_HANDLE_VALUE)
    {
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
		    NULL ))
            {
    		    fprintf(stderr, "Arg: CreateFile failed, FormatMessage failed\n");
	    	    return NULL;
	        }

		errno = EACCES;

	    LocalFree(lpMsgBuf);
		return NULL;	
    }
    return dag_2nd_core_fd;
#endif
}




#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
int
#elif defined(_WIN32)
HANDLE
#endif
dag82x_open_2nd_core(const char* dagname_1st_core)
{
	/* assumes that the 2nd core is at 1st core +1. eg if /dev/dag0 is the 1st core then /dev/dag1 is assumed to be the 2nd core */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	regex_t preg;
	regmatch_t pmatch[1];
	int dag_num = 0;
	char dagname_2nd_core[256];
	int dag_2nd_core_fd = 0;

	memset(pmatch, 0, sizeof(pmatch));
	memset(&preg, 0, sizeof(preg));
	memset(dagname_2nd_core, 0, sizeof(dagname_2nd_core));
	if (regcomp(&preg, ".*[0-9]", REG_EXTENDED) != 0)
	{
		return -1;
	}
	if (regexec(&preg, dagname_1st_core, 1, pmatch, 0) == REG_NOMATCH)
	{
		return -1;
	}
	if (pmatch[0].rm_so > -1 && pmatch[0].rm_so < strlen(dagname_1st_core))
	{
		dag_num = atoi(&dagname_1st_core[pmatch[0].rm_eo-1]);
		dag_num++;
		snprintf(dagname_2nd_core, 256, "/dev/dag%d", dag_num);
		dag_2nd_core_fd = open(dagname_2nd_core, O_RDWR);
	}
	regfree(&preg);
	return dag_2nd_core_fd;
#elif defined(_WIN32)
    /*
    We don't have a regex library on Windows so we need to do this differently, however we can make a simplifying assumption
    that the name of the device will always be 'dag[0-9]'.
    */
    int dag_num = 0;
    HANDLE dag_2nd_core_fd = 0;
    char dagname_2nd_core[256];
    DWORD last_error;
	LPVOID lpMsgBuf;

    sscanf(dagname_1st_core, "dag%d", &dag_num);
    dag_num++;
    snprintf(dagname_2nd_core, 256, "\\\\.\\dag%d", dag_num);
    dag_2nd_core_fd = CreateFile(
		dagname_2nd_core,			
		GENERIC_READ,		
		FILE_SHARE_READ,					
		NULL,				
		OPEN_EXISTING,		
		0,					
		0);
    if (dag_2nd_core_fd == INVALID_HANDLE_VALUE)
    {
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
		    NULL ))
            {
    		    fprintf(stderr, "Arg: CreateFile failed, FormatMessage failed\n");
	    	    return NULL;
	        }

		errno = EACCES;

	    LocalFree(lpMsgBuf);
		return NULL;	
    }
    return dag_2nd_core_fd;
#endif
}

int
dagutil_pbmconfigmemoverlap(int dagfd, volatile uint8_t* dagiom, uint32_t pbm_base, daginf_t* daginf, int pbm_ver)
{
	int retval = 0;
	int base;
	int rxstreams;
	int size;
	int stream;
	int streammax;
	int txstreams;

	if (dagutil_lockallstreams(dagfd))
	{
		retval = EACCES;
		goto exit;
	}

	rxstreams = dag_rx_get_stream_count(dagfd);
	txstreams = dag_tx_get_stream_count(dagfd);
	streammax = dagutil_max(rxstreams*2 - 1, txstreams*2);

	if ((rxstreams < 1) || (txstreams < 1))
	{
		retval = ENODEV;
		goto exit;
	}

	for (stream = 0; stream < streammax; stream++)
	{
		if ((stream == 0) || (stream == 1))
		{
			base = daginf->phy_addr;
			size = daginf->buf_size;
		}
		else
		{
			base = 0;
			size = 0;
		}
		
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_addr, base);
		dagutil_iomwrite(dagiom, pbm_base + uPbmOffset[pbm_ver].streambase + stream*uPbmOffset[pbm_ver].streamsize + uPbmOffset[pbm_ver].mem_size, size);
	}

exit:
	dagutil_unlockallstreams(dagfd);

	return retval;
}


#define DAG_CONFIG 0x88

void
dagutil_reset_datapath(volatile uint8_t* iom)
{
	int val;
	
	val = dagutil_iomread(iom, DAG_CONFIG);
	val |= 0x80000000;
	dagutil_iomwrite(iom, DAG_CONFIG, val);
}


/* Maps from physical line number to logical line */
static int uPhysicalToLogical[] =
{
	13,
	5,
	7,
	15,
	4,
	12,
	14,
	6,
	11,
	3,
	8,
	0,
	2,
	10,
	1,
	9
};


uint32_t
dagutil_37t_line_get_logical(uint32_t uline)
{
	return uPhysicalToLogical[uline];
}











/**
 * Function to get the current time of a system timer, nanosecond accuracy will be supported
 * if the underlying OS / platform also supports it. This function is not
 * designed to give time-of-day type values (although it may on some platforms),
 * instead it was designed to be used for measuring time-out intervals inside
 * polling loops. The time values returned are only relevant when used in conjunction
 * with other time values returned by this same function.
 *
 * Caveats about using this function:
 *
 * On Windows platforms the system tick count is used, this gives a resolution of approx
 * 10ms (may be up to 55ms on some other versions). However more seriously the tick count
 * rolls over approximately every 48 days, during that transistion if code is using the
 * value returned by dagutil_get_time to busy wait for a particular time, or as an indication
 * of when to timeout, the result will be not what was expected.
 *
 * On posix (unix flavor) platforms the gettimeofday function is used, the problem with
 * this is that there is also a settimeofday function. As in windows if code is using
 * the value returned by dagutil_get_time to busy wait, or as a time out indicator, and
 * another processes sets the timeofday then the results are going to be unpredicable.
 *
 *
 * @param[out]  time_p  Pointer to dagutil_time_t structure that upon returning
 *                      from a successiful call will contain the current time.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully
 * @retval   EINVAL     Invalid parameter supplied (time_p was NULL).
 *
 * @sa dagutil_microsleep | dagutil_nanosleep
 */
int
dagutil_get_time(dagutil_time_t *time_p)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	struct timeval  tv;
#elif defined(_WIN32)
	DWORD           ticks;
#endif

	/* sanity check */
	if ( time_p == NULL )
		return EINVAL;

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	gettimeofday (&tv, NULL);
	time_p->sec  = tv.tv_sec;
	time_p->nsec = tv.tv_usec * 1000;

#elif defined(_WIN32)

	/* The high performance counters weren't used because there is no guarantee at what
	 * rates the counter will roll over.
	 */
	ticks = GetTickCount();
	time_p->sec  = (uint32_t)(ticks / 1000);
	time_p->nsec = (int32_t) ((ticks % 1000) * 1000000);

#endif   /* Platform-specific code. */

	return 0;
}






/**
 * Creates a new semaphore object or retreives a handle to an existing semaphore object.
 *
 * The semaphore handle may be global (system wide) or local, if the name parameter is
 * not NULL then the semaphore handle returned is to global semaphore. Global semaphores can
 * be used across processes.
 *
 * This group of semaphore functions, implement a binary type semaphore. It enforces a
 * semaphore maximum count of 1, and the 'take' and 'give' functions decrement and increment
 * the semaphore count by one.
 *
 * @param[out]  sem_p   Pointer to a semaphore type that is populated by this function, if
 *                      the function fails this value is undefined.
 * @param[in]   initial The initial state of the semaphore object, if this parameter is
 *                      zero then the semaphore is created unavailable, any other non-zero value
 *                      will cause the semaphore to have an initial value of 1 (available).
 * @param[in]   name    Pointer to a NULL terminated string containing the name of the semaphore,
 *                      this is used to gain access to a global (system wide) semaphore object.
 *                      If this name matches the name of an existing semaphore object then
 *                      the handle to that semaphore is returned and the initial argument
 *                      is ignored.
 *                      For maximum portablity the name of the semaphore should be no more
 *                      than 12 characters long and should contain no back or forward slashes.
 *                      Also standard filesystem naming conventions should be used, i.e. the
 *                      following characters shouldn't be used; \ / : * ? " < > |
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully
 * @retval   EINVAL     Invalid parameter supplied (sem_p was NULL).
 * @retval   EACCES     A semaphore exists with the given name, but the calling process does
 *                      not have permission to access it.
 * @retval   ENOMEM     Not enough memory for the semaphore data structure.
 * @retval   ENOSPC     The number of semaphores exceed the system maximum.
 * @retval   EEXIST     An unrelated synchronisation object already exists with
 *                      the same name.
 * @retval   ENAMETOOLONG The length of name exceeds MAX_PATH.
 * @retval   ENOENT     Invalid name parameter.
 *
 * @sa dagutil_semaphore_take
 * @sa dagutil_semaphore_give
 * @sa dagutil_semaphore_delete
 */
int
dagutil_semaphore_init (dagutil_sem_t *sem_p, int initial, const char *name)
{

	if ( sem_p == NULL )
		return EINVAL;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))
	{
		struct dagutil_sem_s * new_sem_p = NULL;
		int                    name_len = 0;

		/* create the mutex object structure */
		new_sem_p = (struct dagutil_sem_s *) dagutil_malloc(sizeof(struct dagutil_sem_s));
		if ( new_sem_p == NULL )
			return ENOMEM;
		memset (new_sem_p, 0x00, sizeof(struct dagutil_sem_s));

		if ( name != NULL )
		{
			sem_t       *sem_id_p;
			mode_t       mode = 0666;
			unsigned int init_value;

			/* sanity check the name length */
			if ( (name_len = strlen(name)) >= PATH_MAX )
				return ENAMETOOLONG;

			/* copy the name */
			new_sem_p->name_p = (char*) dagutil_malloc( (name_len + 2) * sizeof(char) );
			if ( new_sem_p->name_p == NULL )
				return ENOMEM;

#if defined(__sun)
			/* On Solaris the name of the semaphore should start with a / character and then
			 * the rest of the name shouldn't contain a slash. Therefore this function automatically
			 * prefixes the first slash and trusts the user hasn't entered any other slash characters.
			 */
			new_sem_p->name_p[0] = '/';
			strncpy (&new_sem_p->name_p[1], name, name_len);
			new_sem_p->name_p[name_len+1] = '\0';
#else
			strncpy (new_sem_p->name_p, name, name_len);
			new_sem_p->name_p[name_len] = '\0';
#endif

			/* generate a unique key or supply a value */
			init_value = ((initial == 0) ? 0 : 1);
			sem_id_p = sem_open(new_sem_p->name_p, O_CREAT, mode, init_value);
			if ( sem_id_p == (sem_t*)SEM_FAILED )
			{
				dagutil_free (new_sem_p->name_p);
				dagutil_free (new_sem_p);
				*sem_p = NULL;
				return errno;
			}

			/* store the mutex handle in the reply argument */
			new_sem_p->sem_p = sem_id_p;
			new_sem_p->named = 1;
			*sem_p = new_sem_p;
			return 0;
		}
		else
		{
			/* create a new local semaphore */
			new_sem_p->sem_p = dagutil_malloc(sizeof(sem_t));
			if ( new_sem_p->sem_p == NULL )
				return ENOMEM;

			if ( sem_init(new_sem_p->sem_p, 0, ((initial == 0) ? 0 : 1)) != 0 )
			{
				dagutil_free (new_sem_p->sem_p);
				dagutil_free (new_sem_p);
				*sem_p = NULL;
				return errno;
			}

			/* store the mutex handle in the reply argument */
			new_sem_p->named = 0;
			*sem_p = new_sem_p;
			return 0;
		}
	}

#elif defined(_WIN32)
	{
		HANDLE  hSemaphore;
		DWORD   dwError;

		/* sanity check the name length */
		if ( strlen(name) >= MAX_PATH )
			return ENAMETOOLONG;
	
		hSemaphore = CreateSemaphore (NULL, (initial == 0 ? 0 : 1), 1, name);
		if ( hSemaphore == NULL )
		{
			dwError = GetLastError();
			if ( (dwError == ERROR_ACCESS_DENIED) && (name != NULL) )
			{
				/* the caller has limited access rights, however the semaphore exists so
				* call OpenSemaphore to gain a handle to the existing object.
				*/
				hSemaphore = OpenSemaphore(SYNCHRONIZE | DELETE | SEMAPHORE_MODIFY_STATE, FALSE, name);
				if ( hSemaphore == NULL )
					return EACCES;
			}
	
			else if ( dwError == ERROR_INVALID_HANDLE )
			{
				return EEXIST;
			}
			else if ( dwError == ERROR_TOO_MANY_SEMAPHORES )
			{
				return ENOSPC;
			}
			else
			{
				return EINVAL;
			}
		}
	
		/* store the mutex handle in the reply argument */
		*sem_p = hSemaphore;
		return 0;
	}

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;

#endif
}




/**
 * If the semaphore is available then the calling thread is not blocked and the thread takes
 * the semaphore. If the thread is unavailable this function blocks until either another thread
 * gives up the semaphore or the timeout is reached. If the timeout is DAGUTIL_WAIT_FOREVER this
 * function blocks until the semaphore is available or an error occurs.
 *
 * Threads shouldn't call dagutil_semaphore_take more than once without an interleaved
 * dagutil_semaphore_give, this may cause deadlock on some platforms.
 *
 * @param[in]   sem     The semaphore handle returned by dagutil_semaphore_create.
 * @param[in]   timeout The time-out value in milliseconds, if the timeout is zero the
 *                      function tests the semaphore state and returns immediately. If timeout
 *                      is DAGUTIL_WAIT_FOREVER the function blocks until the semaphore is
 *                      available or an error occurs.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied.
 * @retval   ETIMEDOUT  Timedout waiting for the sempahore to become available.
 * @retval   EDEADLK    A deadlock condition was detected (non-Windows only)
 *
 * @sa dagutil_semaphore_init
 * @sa dagutil_semaphore_give
 * @sa dagutil_semaphore_delete
 */
int
dagutil_semaphore_take (dagutil_sem_t sem, uint32_t timeout)
{

	if ( sem == NULL )
		return EINVAL;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))

	/* First try to get lock the semaphore without waiting on it */
	while ( sem_trywait(sem->sem_p) != 0 )
	{
		switch(errno)
		{
			case EAGAIN:
				/* the semaphore is locked so we will have to wait on it */
				goto is_locked;
			case EINTR:
				/* a signal interrupted this wait ... try again  */
				continue;
			default:
				/* another error occuried */
				return errno;
		}
	}

	/* Wasn't locked */
	return 0;

is_locked:

	if ( timeout == 0 )
	{
		/* if a timeout of zero then, no need to do anything just returned because the semaphore is unavailable */
		return ETIMEDOUT;
	}
	else if ( timeout == DAGUTIL_WAIT_FOREVER )
	{
		/* if the time-out is forever then block until the semaphore becomes available or an error occurs */
		while ( sem_wait(sem->sem_p) != 0 )
		{
			switch (errno)
			{
				case EINTR:
					/* a signal interrupted this wait ... try again  */
					continue;
				default:
					return errno;
			}
		}
		return 0;
	}
	else
	{
		/* wait a specific time period for the semaphore to become available */
		struct timespec ts_timeout;
		struct timeval  tv_curtime;

		/* get the current time and add the timeout value to it to give an absolute timeout */
		gettimeofday (&tv_curtime, NULL);
	
		ts_timeout.tv_sec  = tv_curtime.tv_sec + (timeout / 1000);
		ts_timeout.tv_nsec = (tv_curtime.tv_usec * 1000) + ((timeout % 1000) * 1000000);
		if (ts_timeout.tv_nsec >= 1000000000)
		{
			ts_timeout.tv_sec++;
			ts_timeout.tv_nsec -= 1000000000;
		}

		/* wait for the semaphore to become avaiable */
		while ( sem_timedwait(sem->sem_p, &ts_timeout) != 0 )
		{
			switch (errno)
			{
				case EINTR:
					/* a signal interrupted this wait ... try again  */
					continue;
				case ETIMEDOUT:
				case EAGAIN:
					/* timed out waiting for the signal to be released */
					return ETIMEDOUT;
				default:
					return errno;
			}
		}
		return 0;
	}

#elif defined(_WIN32)

	switch ( WaitForSingleObject(sem, (timeout == DAGUTIL_WAIT_FOREVER) ? INFINITE : timeout) )
	{
		case WAIT_OBJECT_0:
			/* the calling thread now has the semaphore */
			return 0;

		case WAIT_TIMEOUT:
			/* timed-out waiting for the semaphore */
			return ETIMEDOUT;

		default:
			/* wait failed */
			return EINVAL;
			
	}

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;

#endif


}


/**
 * This function releases the next thread waiting on an available indication from the
 * semaphore. If no thread is waiting on this semaphore, the semaphore state becomes
 * available.
 *
 * @param[in]   sem    The semaphore handle returned by dagutil_semaphore_create.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied.
 * @retval   EDEADLK    A deadlock condition was detected (non-Windows only)
 *
 * @sa dagutil_semaphore_init
 * @sa dagutil_semaphore_take
 * @sa dagutil_semaphore_delete
 */
int
dagutil_semaphore_give (dagutil_sem_t sem)
{
	if ( sem == NULL )
		return EINVAL;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))
	/* to provide some measure of protection we ensure that the semaphore is
	 * locked prior to releasing the lock, this may fail if two unlock calls
	 * occur at the same time.
	 */
	while ( sem_trywait(sem->sem_p) != 0 )
	{
		switch (errno)
		{
			case EAGAIN:
				goto is_locked;
			case EINTR:
				continue;
			default:
				return errno;
		}
	}

is_locked:
	if ( sem_post(sem->sem_p) != 0 )
		return errno;
	else
		return 0;


#elif defined(_WIN32)
	if ( ReleaseSemaphore(sem, 1, NULL) == FALSE )
	{
		/* Unlike the unix style semaphores, windows semaphore complain when
		 * trying to increment the semaphore count above the maximum, which
		 * our case is 1. To keep the functionality similar across both unix
		 * and windows we ignore this error.
		 */
		if ( GetLastError() == ERROR_TOO_MANY_POSTS )
			return 0;
		else
			return EINVAL;
	}

	return 0;

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;
	
#endif


}





/**
 * Destroys the semaphore details created for this process, if the semaphore was a global
 * named semaphore then the semaphore is destroyed only when all processes have also
 * destroyed their semaphore handles.
 *
 *
 * @param[in]   sem     The semaphore handle returned by dagutil_semaphore_create.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied.
 * @retval   EDEADLK    A deadlock condition was detected (non-Windows only)
 *
 * @sa dagutil_semaphore_init
 * @sa dagutil_semaphore_take
 * @sa dagutil_semaphore_give
 */
int
dagutil_semaphore_delete (dagutil_sem_t sem)
{
	if ( sem == NULL )
		return EINVAL;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))

	if ( sem->named )
	{
		/* named mutex so close the semaphore and unlink it */
		if ( sem_close(sem->sem_p) != 0 )
			return errno;
		sem_unlink(sem->name_p);

		dagutil_free (sem->name_p);
		dagutil_free (sem);
		return 0;
	}
	else
	{
		/* local mutex so just destroy it */
		if ( sem_destroy(sem->sem_p) != 0 )
			return errno;

		dagutil_free (sem->sem_p);
		dagutil_free (sem);
		return 0;
	}

#elif defined(_WIN32)

	if ( CloseHandle(sem) == FALSE )
		return EINVAL;
	else
		return 0;

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;

#endif

}



/**
 * Creates a new mutex object that can be passed to subsequent mutex
 * functions.
 *
 * After a mutex object has been initialized, the threads of the
 * process can specify the object in the dagutil_mutex_lock or
 * dagutil_mutex_unlock function to provide mutually exclusive access
 * to a shared resource. For similar synchronization between the threads of
 * different processes, use a semaphore object.
 *
 * Once finished with a mutex call dagutil_mutex_delete to clean up the
 * mutex and release any memory associated with it.
 *
 * A mutex should not be initialised more than once, doing so will result
 * in unpredicable behaviour.
 *
 * @param[out]  mutex_p Pointer to a critical section object that
 *                      is initialised by this function.
 * @param[in]   initial The initial state of the mutex, a non-zero value
 *                      starts the mutex in a locked state, a zero value
 *                      starts the mutex in a unlocked state.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied (mutex_p is NULL)
 * @retval   ENOMEM     Insufficient memory exists to initialize the mutex.
 * @retval   EPERM      The caller does not have the privilege to perform the operation.
 * @retval   EAGAIN     The system lacked the necessary resources (other than memory)
 *                      to initialize another mutex.
 *
 * @sa dagutil_mutex_lock
 * @sa dagutil_mutex_unlock
 * @sa dagutil_mutex_delete
 */
int
dagutil_mutex_init (dagutil_mutex_t *mutex_p, int initial)
{
	if ( mutex_p == NULL )
		return EINVAL;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))
	{
		pthread_mutex_t *   new_mutex_p;
		pthread_mutexattr_t mutexattr;
		int                 res;


		/* initialise the attributes */
		res = pthread_mutexattr_init(&mutexattr);
		if ( res != 0 )       return res;

		/* Set the mutex as a recursive mutex */
	#if defined(__linux__)
		res = pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
	#else
		res = pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	#endif
		if ( res != 0 )
		{
			pthread_mutexattr_destroy(&mutexattr);
			return res;
		}

		/* Allocate some memory for the object */
		new_mutex_p = (pthread_mutex_t*) dagutil_malloc( sizeof(pthread_mutex_t) );
		if ( new_mutex_p == NULL )
		{
			pthread_mutexattr_destroy(&mutexattr);
			return ENOMEM;
		}

		/* create the mutex with the attributes set */
		res = pthread_mutex_init(new_mutex_p, &mutexattr);

		/* if the initial value is suppose to be locked ... do it now */
		if ( (res == 0) && (initial != 0) )
			pthread_mutex_lock (new_mutex_p);

		/* after initializing the mutex, the mutex attribute can be destroyed */
		pthread_mutexattr_destroy(&mutexattr);


		/* clean up if an error occuried */
		if ( res != 0 )
			dagutil_free (new_mutex_p);
		else
			*mutex_p = new_mutex_p;

		return res;
	}
	
#elif defined(_WIN32)
	{
		HANDLE hMutex;

		hMutex = CreateMutex (NULL, (initial == 0 ? FALSE : TRUE), NULL);
		if ( hMutex == NULL )
			return EPERM;
		else
		{
			*mutex_p = hMutex;
			return 0;
		}
	}
#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;

#endif

}


/**
 * Waits for ownership of the specified mutex object. The function
 * returns when the calling thread is granted ownership.
 *
 * Every call to dagutil_mutex_lock must have a matching call to dagutil_mutex_unlock.
 *
 * If a thread terminates while it has ownership of a mutex, the state of the mutex
 * is undefined.
 *
 * If a mutex is deleted while it is still owned, the state of the threads
 * waiting for ownership of the deleted mutex is undefined.
 *
 * @param[in]   mutex   The mutex object.
 * @param[in]   timeout The time-out value in milliseconds, if the timeout is zero the
 *                      function tests the mutex state and returns immediately. If timeout
 *                      is DAGUTIL_WAIT_FOREVER the function blocks until the mutex is
 *                      released or an error occurs.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully and the calling thread has
 *                      acquired the mutex.
 * @retval   EINVAL     Invalid parameter supplied.
 * @retval   ETIMEDOUT  Timed-out waiting for the mutex to become unlocked.
 * @retval   EAGAIN     Mutex couldn't be acquired because the number of recursize
 *                      locks for the mutex has been exceeded.
 * @retval   EDEADLK    A deadlock condition was detected or the current thread already
 *                      owns the mutex (non-Windows only).
 *
 * @sa dagutil_mutex_init
 * @sa dagutil_mutex_unlock
 * @sa dagutil_mutex_delete
 */
int
dagutil_mutex_lock (dagutil_mutex_t mutex, uint32_t timeout)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))
	int res;

	if ( timeout == DAGUTIL_WAIT_FOREVER )
	{
		/* block until the lock is obtained */
		return pthread_mutex_lock(mutex);
	}
	else if ( timeout == 0 )
	{
		/* only try the lock and return if we succeeded or failed */
		if ( (res = pthread_mutex_trylock(mutex)) == EBUSY )
			return ETIMEDOUT;
		else
			return res;
	}
	else
	{
		/* wait a specific time period for the semaphore to become available */
		struct timespec ts_timeout;
		struct timeval  tv_curtime;

		/* get the current time and add the timeout value to it to give an absolute timeout */
		gettimeofday (&tv_curtime, NULL);
	
		ts_timeout.tv_sec  = tv_curtime.tv_sec + (timeout / 1000);
		ts_timeout.tv_nsec = (tv_curtime.tv_usec * 1000) + ((timeout % 1000) * 1000000);
		if (ts_timeout.tv_nsec >= 1000000000)
		{
			ts_timeout.tv_sec++;
			ts_timeout.tv_nsec -= 1000000000;
		}

		/* block until either the lock is obtained or a time-out occurs */
		return pthread_mutex_timedlock(mutex, &ts_timeout);
	}

#elif defined(_WIN32)

	switch ( WaitForSingleObject(mutex, (timeout == DAGUTIL_WAIT_FOREVER) ? INFINITE : timeout) )
	{
		case WAIT_OBJECT_0:
			/* the calling thread now has the mutex */
			return 0;

		case WAIT_TIMEOUT:
			/* timed-out waiting for the semaphore */
			return ETIMEDOUT;

		default:
			/* wait failed */
			return EINVAL;
	}

	return 0;

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;

#endif
}





/**
 * Releases ownership of the specified mutex object.
 *
 * Unpredicable behaviour may occur if this function is called without a preceding
 * successiful call to dagutil_mutex_lock.
 *
 * @param[in]   mutex   Handle to the mutex object.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully and the calling thread has
 *                      released the mutex.
 * @retval   EINVAL     Invalid parameter supplied.
 * @retval   EPERM      The current thread does not own the mutex
 *
 * @sa dagutil_mutex_init
 * @sa dagutil_mutex_lock
 * @sa dagutil_mutex_delete
 */
int
dagutil_mutex_unlock (dagutil_mutex_t mutex)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))

	return pthread_mutex_unlock(mutex);

#elif defined(_WIN32)

	if ( ReleaseMutex(mutex) == FALSE )
	{
		if ( GetLastError() == ERROR_NOT_OWNER )
			return EPERM;
		else
			return EINVAL;
	}
	return 0;

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Sempahores
	return -1;

#endif
}


/**
 * Releases all resources used by an unlocked mutex.
 *
 * Unpredicable behaviour may occur if this function is called while the
 * mutex is locked by a thread (including the calling one).
 *
 * @param[in]   mutex   Handle to the mutex object
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied.
 * @retval   EBUSY      The mutex object is currently in use by a thread
 *                      (non-Windows platforms only).
 *
 * @sa dagutil_mutex_init
 * @sa dagutil_mutex_lock
 * @sa dagutil_mutex_unlock
 */
int
dagutil_mutex_delete (dagutil_mutex_t mutex)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))
	int res = pthread_mutex_destroy(mutex);
	
	/* free the memory allcoated for the mutex handle */
	if ( res == 0 )
		dagutil_free (mutex);

	return res;

#elif defined(_WIN32)

	CloseHandle (mutex);
	return 0;

#elif defined(__APPLE__)

	// TODO: Use Carbon Multiprocessing Binary Sempahores
	return -1;

#endif
}







/**
 * Creates a new thread that starts imediately from the start of the user
 * provided entry point procedure. The arg parameter defines custom arguments
 * that can be passed to the threads entry point function.
 *
 * @param[in]   proc    The entry point function for the thread.
 * @param[in]   arg     Optional argument that is past to the entry point function.
 * @param[out]  id_p    Pointer to a thread ID type, that upon return will contain
 *                      the ID of the thread just created. This parameter can be
 *                      NULL if the thread ID is not required.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied.
 *
 * @sa dagutil_thread_exit
 * @sa dagutil_thread_kill
 * @sa dagutil_thread_get_id
 */
int dagutil_thread_create (dagutil_thread_proc_t proc, void *arg, dagutil_thread_id_t *id_p)
{
	/* sanity check the entry point argument */
	if ( proc == NULL )
		return EINVAL;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	{
		pthread_attr_t  attr;
		int             res;

		if ( (res = pthread_attr_init(&attr)) != 0 )
			return res;
		if ( (res = pthread_attr_setschedpolicy(&attr, SCHED_OTHER)) != 0 )
			return res;
		if ( (res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0 )
			return res;

		res = pthread_create (id_p, &attr, (void *)proc, arg);

		pthread_attr_destroy (&attr);

		return res;
	}
#elif defined(_WIN32)
	{
		if ( CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, arg, 0, id_p) == NULL )
			return EINVAL;
			
		else
			return 0;
	}
#endif

}


/**
 * Causes the currently running thread to exit, this is equivalent to returning
 * from the entry point function.
 *
 * Warning about using this function with C++ code; object destructors are not
 * called by this function, therefore it is preferable to return from the entry
 * point function rather than calling this function. Another option in C++ is to
 * allocate the object dynamically and free them before calling this function.
 *
 * @sa dagutil_thread_create
 * @sa dagutil_thread_kill
 * @sa dagutil_thread_get_id
 */
void dagutil_thread_exit (void)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	pthread_exit (NULL);

#elif defined(_WIN32)
	
	ExitThread (0);

#endif
}


/**
 * Terminates a running thread. This function should be used with caution, it
 * causes the thread to terminate imediately, not giving it an opertunity to
 * clean up any resources it has allocated. Synchronisation objects in use by
 * the thread are not released gracefully, for example if a thread had a mutex
 * lock, the state of the lock after the thread had been terminated is undefined.
 *
 * @param[in]   id      The ID of the thread to kill.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied.
 *
 * @sa dagutil_thread_create
 * @sa dagutil_thread_exit
 * @sa dagutil_thread_get_id
 */
int dagutil_thread_kill (dagutil_thread_id_t id)
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	return pthread_cancel (id);

#elif defined(_WIN32)
	{
		HANDLE hThread = OpenThread (THREAD_TERMINATE, FALSE, id);
		if ( hThread == NULL )
		{
			/* For now assume all errors are caused by an invalid thread ID */
			return EINVAL;
		}

		TerminateThread (hThread, 0);
		return 0;
	}
#endif
}


/**
 * This function returns the ID of current thread.
 *
 * @param[out]  id_p    Pointer to a thread Id value that upon return will
 *                      contain the current threads ID.
 *
 * @returns             An error code indicating either success or failure.
 * @retval   0          Function complete successifully.
 * @retval   EINVAL     Invalid parameter supplied, id_p was NULL.
 *
 * @sa dagutil_thread_create
 * @sa dagutil_thread_exit
 * @sa dagutil_thread_kill
 */
int dagutil_thread_get_id (dagutil_thread_id_t *id_p)
{
	if ( id_p == NULL )
		return EINVAL;

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

	*id_p = pthread_self();
	return 0;

#elif defined(_WIN32)

	*id_p = GetCurrentThreadId();
	return 0;

#endif
}




/**
 * Utility function that performs the same functionality as snprintf, however it
 * concatenates the string in the printf format part onto the end of any existing
 * stext in the string. This is equivalent to this:
 *
 *      snprintf (new_string, (n - strlen(string)), fmt, ...);
 *      strncat (string, new_string, n);
 *
 * But all in one function and with more error checking.
 *
 * @param[in,out]  str     The buffer that gets the new string concatenated onto it.
 * @param[in]      size    The total size of the buffer in characters.
 * @param[in]      fmt     The format string.
 * @param[in]      ...     Variable number of arguments.
 *
 * @returns                On success, returns the number of characters that would have
 *                         been written had size been sufficiently large, not counting
 *                         the terminating nul character.
 *
 */
int dagutil_snprintfcat(char *str, size_t size, const char *fmt, ...)
{
	size_t  cur_len;
	va_list ap;
	int     res;

	if ( size == 0 || str == NULL )
		return 0;
		
	cur_len = strlen(str);
	if ( cur_len >= size )
		return cur_len;
		
	va_start(ap, fmt);
	
	res = vsnprintf ((str + cur_len), (size - cur_len), fmt, ap);
	
	va_end(ap);
	
	return (cur_len + res);
}

