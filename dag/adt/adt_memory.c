/* File header */
#include "adt_memory.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef NDEBUG
#include "adt_debugging.h"
#endif /* Debugging code. */


#if ADT_MEMORY_DEBUG
/* ADT headers. */
#include "adt_adler32.h"

/* POSIX headers. */
#include <pthread.h>

/* C Standard Library headers. */
#include <time.h>


#define TAIL_CANARY_SIZE 8
#define TAIL_CANARY_BYTE 0xf9


typedef enum
{
	kFlagReferenced = 1

} allocation_flags_t;

typedef enum
{
	kAllocationMagicNumber = 0xcafe

} allocation_magic_t;

typedef struct AllocationHeader AllocationHeader;
typedef AllocationHeader* AllocationPtr;

#define SOURCEFILE_BYTES 32
struct AllocationHeader
{
	uint16_t magic_number;
	uint16_t line_number;
	uint32_t size;
	uint32_t flags;
	uint32_t allocation_id;
	AllocationPtr next;
	AllocationPtr prev;
	uint32_t timestamp; /* from time(NULL) */
	char sourcefile[SOURCEFILE_BYTES];
	uint32_t checksum;

};

static pthread_mutex_t uMutex = PTHREAD_MUTEX_INITIALIZER;
static AllocationPtr uHead = NULL;
static AllocationPtr uTail = NULL;
static uint32_t uAllocationId = 1;
#endif /* ADT_MEMORY_DEBUG */


static const unsigned int kPascalStringLength = 256;


/* Internal routines. */
#ifndef NDEBUG
static void debug_error_routine(size_t size);
#endif /* Debugging code. */
static inline AdtPtr adt_allocate_ptr(size_t size);
#if ADT_MEMORY_DEBUG
static inline AllocationPtr get_allocation_ptr(AdtPtr p);
static inline AdtPtr get_application_ptr(AllocationPtr p);
static inline uint8_t* get_canary_ptr(AllocationPtr header);
static inline void checksum_header(AllocationPtr header);
static inline unsigned int is_valid_allocation_header(AllocationPtr header);
static inline unsigned int is_valid_ptr(AdtPtr p);
#endif /* ADT_MEMORY_DEBUG */


#ifndef NDEBUG
static MemoryErrorRoutine gErrorRoutine = debug_error_routine;
#else
static MemoryErrorRoutine gErrorRoutine = NULL;
#endif /* Debugging code. */


/* Implementation of internal routines. */
#ifndef NDEBUG
static void
debug_error_routine(size_t size)
{
	/* Useful during debugging, since the abort call preserves the execution state in (e.g.) gdb. */
	fprintf(stderr, "Error: memory allocation request for %u bytes failed.\n", (unsigned int) size);
	abort();
}
#endif /* Debugging code. */


static inline AdtPtr
adt_allocate_ptr(size_t size)
{
#ifndef NDEBUG
	AdtPtr p = (AdtPtr) malloc(size);
#endif /* Debugging code. */

#ifndef NDEBUG
	if (p != NULL)
	{
		adt_trash_ptr(p, size);
	}

	return p;
#else

	/* Build version. */
	return (AdtPtr) malloc(size);

#endif /* Debugging code. */
}


#if ADT_MEMORY_DEBUG
static inline AllocationPtr
get_allocation_ptr(AdtPtr p)
{
	return (AllocationPtr) (((char*) p) - sizeof(AllocationHeader));
}


static inline AdtPtr
get_application_ptr(AllocationPtr p)
{
	return (AdtPtr) (((char*) p) + sizeof(AllocationHeader));
}


static inline uint8_t*
get_canary_ptr(AllocationPtr header)
{
	return (uint8_t*) (((char*) header) + sizeof(AllocationHeader) + header->size);
}


static inline void
checksum_header(AllocationPtr header)
{
	header->checksum = 0;
	header->checksum = adt_adler32((const char*) header, sizeof(AllocationHeader));
}


/* Caller must hold the memory subsystem mutex. */
static unsigned int
is_valid_allocation_header(AllocationPtr header)
{
	uint32_t checksum;
	uint8_t* canary;
	int index;

	/* Check fields. */
	if ((header == NULL) || (header->magic_number != kAllocationMagicNumber))
	{
		assert(0);
		return 0;
	}
	
	if ((header->line_number == 0) || (header->sourcefile[0] == '\0'))
	{
		assert(0);
		return 0;
	}

	/* Verify checksum. */
	checksum = header->checksum;
	header->checksum = 0;
	header->checksum = adt_adler32((const char*) header, sizeof(AllocationHeader));

	if (checksum != header->checksum)
	{
		assert(0);
		return 0;
	}

	/* Verify tail canary. */
	canary = get_canary_ptr(header);
	for (index = 0; index < TAIL_CANARY_SIZE; index++)
	{
		if (*canary != TAIL_CANARY_BYTE)
		{
			assert(0);
			return 0;
		}
		canary++;
	}

	return 1;
}


/* Caller must hold the memory subsystem mutex. */
static unsigned int
is_valid_ptr(AdtPtr p)
{
	if (p)
	{
		AllocationPtr header = get_allocation_ptr(p);
		return is_valid_allocation_header(header);
	}
	
	assert(0);
	return 0;
}
#endif /* ADT_MEMORY_DEBUG */


/* To encourage the NULL'ing of free()'d pointers, this routine returns a AdtPtr.
 * Intended usage is:
 *
 *		my_ptr = adt_dispose_ptr(my_ptr);
 *
 * which frees the memory and NULLs the pointer, with the added bonus of giving a compiler warning if called as
 *
 *		adt_dispose_ptr(my_ptr);
 *
 * with the potential to later treat my_ptr as valid.
 */
AdtPtr
adt_dispose_ptr(AdtPtr p)
{
	if (p)
	{
    #if ADT_MEMORY_DEBUG
		AllocationPtr header;
		AllocationPtr next_header;
		AllocationPtr prev_header;
		unsigned int valid;

		pthread_mutex_lock(&uMutex);

		/* Check header and neighbours. */
		header = get_allocation_ptr(p);
		next_header = header->next;
		prev_header = header->prev;
		valid = is_valid_ptr(p);

		if (next_header)
		{
			valid = is_valid_allocation_header(next_header);
		}

		if (prev_header)
		{
			valid = is_valid_allocation_header(prev_header);
		}

		/* Rearrange list. */
		if (next_header)
		{
			next_header->prev = prev_header;
			checksum_header(next_header);
		}

		if (prev_header)
		{
			prev_header->next = next_header;
			checksum_header(prev_header);
		}

		if (header == uTail)
		{
			uTail = prev_header;
		}

		if (header == uHead)
		{
			uHead = next_header;
		}
		pthread_mutex_unlock(&uMutex);

		p = (AdtPtr) header;
    #endif /* ADT_MEMORY_DEBUG */

		free(p);
	}

	return NULL;
}


PStringPtr
adt_allocate_pstring_ptr(const char* pstring)
{
	unsigned char* result = NULL;
	size_t length = (size_t) (pstring[0]) + 1;
	size_t bytes;

	assert(length < kPascalStringLength);

		/* round up to nearest multiple of 4 bytes. */
	bytes = 1 + ((length - 1) | 0x3);

	assert(bytes < kPascalStringLength);
	assert(bytes % 4 == 0);

	result = (unsigned char*) adt_xallocate_ptr(bytes);
	if (result != NULL)
	{
		/* Copy string content (including length byte). */
		memcpy(result, &pstring[0], length);
	}

#ifndef NDEBUG
	adt_assert_valid_pstring_ptr(result, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	return (PStringPtr) result;
}


PStringPtr
adt_dispose_pstring_ptr(PStringPtr pstring_ptr)
{
#ifndef NDEBUG
	adt_assert_valid_pstring_ptr(pstring_ptr, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	if (pstring_ptr)
	{
		(void) adt_dispose_ptr((AdtPtr) pstring_ptr);
	}

	return NULL;
}


const char*
adt_allocate_cstring(const char* cstring)
{
	size_t length = strlen(cstring) + 1;
	size_t total_bytes;
	char* result;

	/* Round up to nearest multiple of 4 bytes.  Most platforms probably do something like this anyway. */
	total_bytes = 1 + ((length - 1) | 0x3);

	assert(cstring[length - 1] == '\0');
	assert(total_bytes % 4 == 0);

	result = (char*) adt_xallocate_ptr(total_bytes);

	if (result)
	{
		/* Copy string content, including null terminator. */
		memcpy(result, cstring, length);
	}

#ifndef NDEBUG
	adt_assert_valid_cstring(result, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	return (const char*) result;
}


void
adt_set_xallocate_error_routine(MemoryErrorRoutine error_routine)
{
	gErrorRoutine = error_routine;
}


AdtPtr
adt_xallocate_ptr(size_t size)
{
	AdtPtr p = adt_allocate_ptr(size);

	if ((p == NULL) && (gErrorRoutine != NULL))
	{
		gErrorRoutine(size);

		/* Retry. */
		p = adt_allocate_ptr(size);
	}

	return p;
}


PStringPtr
adt_xallocate_pstring_ptr(const char* cstring)
{
	PStringPtr p = adt_allocate_pstring_ptr(cstring);

	if ((p == NULL) && (gErrorRoutine != NULL))
	{
		gErrorRoutine(kPascalStringLength);

		/* Retry. */
		p = adt_allocate_pstring_ptr(cstring);
	}

	return p;
}


const char*
adt_xallocate_cstring(const char* cstring)
{
	const char* p = adt_allocate_cstring(cstring);

	if ((p == NULL) && (gErrorRoutine != NULL))
	{
		gErrorRoutine(strlen(cstring) + 1);

		/* Retry. */
		p = adt_allocate_cstring(cstring);
	}

	return p;
}


#if ADT_MEMORY_DEBUG
AdtPtr
adt_xallocate_ptr_ex(size_t size, const char* sourcefile, unsigned int linenumber)
{
	size_t new_size = sizeof(AllocationHeader) + size + sizeof(uint8_t) * TAIL_CANARY_SIZE;
	AdtPtr p = adt_xallocate_ptr(new_size);
	AllocationPtr header = (AllocationPtr) p;
	uint8_t* canary;
	int index;

	header->size = size;
	header->flags = 0;
	header->line_number = linenumber;
	header->timestamp = time(NULL);
	header->allocation_id = uAllocationId;
	uAllocationId++;

	/* Copy the source file name and guarantee termination. */
	strncpy(header->sourcefile, sourcefile, SOURCEFILE_BYTES);
	header->sourcefile[SOURCEFILE_BYTES - 1] = '\0';

	/* Add the canary bytes after the user data space. */
	canary = get_canary_ptr(header);
	for (index = 0; index < TAIL_CANARY_SIZE; index++)
	{
		*canary = TAIL_CANARY_BYTE;
		canary++;
	}

	/* Add to tail of linked list. */
	pthread_mutex_lock(&uMutex);
	header->next = NULL;
	header->prev = uTail;
	if (uTail)
	{
		uTail->next = header;
		checksum_header(uTail);
	}
	else
	{
		uHead = header;
	}
	uTail = header;

	header->magic_number = kAllocationMagicNumber;
	checksum_header(header);
	pthread_mutex_unlock(&uMutex);

	return (AdtPtr) (((char*) p) + sizeof(AllocationHeader));
}


const char*
adt_xallocate_cstring_ex(const char* cstring, const char* sourcefile, unsigned int linenumber)
{
	size_t length = strlen(cstring) + 1;
	char* result;
	size_t total_bytes;

	/* Round up to nearest multiple of 4 bytes.  Most platforms probably do something like this anyway. */
	total_bytes = 1 + ((length - 1) | 0x3);

	assert(cstring[length - 1] == '\0');
	assert((total_bytes % 4 == 0));

	result = (char*) adt_xallocate_ptr_ex(total_bytes, sourcefile, linenumber);

	if (result)
	{
		/* Copy string content, including null terminator. */
		memcpy(result, cstring, length);
	}

#ifndef NDEBUG
	adt_assert_valid_cstring(result, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	return (const char*) result;
}


void
adt_memory_clear_refs(void)
{
	AllocationPtr current;
	uint32_t and_value = ~((uint32_t) kFlagReferenced);

	pthread_mutex_lock(&uMutex);
	current = uHead;

	while (current)
	{
		current->flags &= and_value;
		checksum_header(current);
		current = current->next;
	}

	pthread_mutex_unlock(&uMutex);
}


void
adt_memory_note_ptr(AdtPtr p)
{
	AllocationPtr header;
	unsigned int valid;

	assert(p);

	pthread_mutex_lock(&uMutex);

	valid = is_valid_ptr(p);
	header = get_allocation_ptr(p);
	header->flags |= kFlagReferenced;
	checksum_header(header);
	valid = is_valid_allocation_header(header);

	pthread_mutex_unlock(&uMutex);
}


void
adt_memory_verify_refs(const char* logfilename, unsigned int nasty)
{
	AllocationPtr current;
	FILE* logfile;
	const char* filename = "memory_verification.log";

	if (logfilename)
	{
		filename = logfilename;
	}

	logfile = fopen(filename, "w");
	pthread_mutex_lock(&uMutex);
	current = uHead;

	while (current)
	{
		AllocationPtr next = current->next;
		
		(void) is_valid_allocation_header(current);
		if ((current->flags & kFlagReferenced) == 0)
		{
			/* Found a block that is not referenced. */
			AdtPtr user_ptr = get_application_ptr(current);

			fprintf(logfile, "Block %8u [%u] %32s:%5u %6u bytes has been leaked.\n", current->allocation_id, current->timestamp, current->sourcefile, current->line_number, current->size);
			fflush(logfile);
			
			/* Especially nasty trick to locate leaked blocks: overwrite the data with a distinctive pattern. */
			if (nasty)
			{
				unsigned int count = current->size / 2;
				unsigned int index;
				
				for (index = 0; index < count; index++)
				{
					((uint16_t*) user_ptr)[index] = 0xdead;
				}
				
				/* Don't worry about the last byte (if there is one). */
			}
		}
		current = next;
	}

	pthread_mutex_unlock(&uMutex);
	fclose(logfile);
}


unsigned int
adt_memory_valid_ptr(AdtPtr p)
{
	unsigned int result;

	pthread_mutex_lock(&uMutex);
	result = is_valid_ptr(p);
	pthread_mutex_unlock(&uMutex);
	
	return result;
}


/* Support routines.  If filename is null then 'adt_memory_dump_YYYYMMDD.txt' is used. */
void
adt_memory_log(const char* filename_hint)
{
	const char* filename = filename_hint;
	time_t timestamp = time(NULL);
	FILE* logfile;
	char filenamebuf[128];

	if (!filename)
	{
		size_t bytes;

		bytes = strftime(filenamebuf, 128, "adt_memory_dump_%Y%m%d.txt", localtime(&timestamp));
		filename = filenamebuf;
	}

	pthread_mutex_lock(&uMutex);
	logfile = fopen(filename, "w");
	if (logfile)
	{
		AllocationPtr current = uHead;
		
		fprintf(logfile, "# Outstanding memory allocations.\n\n");

		while (current)
		{
			unsigned int valid __attribute__((unused)) = is_valid_allocation_header(current);

			fprintf(logfile, "%u %32s:%u %u bytes.\n", current->timestamp, current->sourcefile, current->line_number, current->size);
			current = current->next;
		}
		
		fclose(logfile);
	}
	pthread_mutex_unlock(&uMutex);
}
#endif /* ADT_MEMORY_DEBUG */
