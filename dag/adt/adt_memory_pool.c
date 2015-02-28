/* File header. */
#include "adt_memory_pool.h"

/* ADT headers. */
#include "adt_debugging.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"

/* C Standard Library. */
#include <assert.h>
#include <stdlib.h>


typedef struct MemoryPoolHeader
{
	adt_magic_number_t mMagicNumber;
	size_t mItemSize;
	unsigned int mItemLimit;
	unsigned int mItems;
	AdtPtr mHead;

} MemoryPoolHeader, *MemoryPoolHeaderPtr;


/* Internal routines. */
static unsigned int valid_memory_pool(MemoryPoolHeaderPtr header);
#ifndef NDEBUG
static void verify_memory_pool(MemoryPoolHeaderPtr header);
#endif /* Debugging code. */


/* Implementation of internal routines. */
static unsigned int
valid_memory_pool(MemoryPoolHeaderPtr header)
{
	if (header && (header->mMagicNumber == kMemoryPoolMagicNumber))
	{
		return 1;
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_memory_pool(MemoryPoolHeaderPtr header)
{
	ADT_ASSERT_REAL_PTR(header, "");
	assert(header->mMagicNumber == kMemoryPoolMagicNumber);
	assert(0 < header->mItemSize);
	if (header->mItemLimit)
	{
		assert(header->mItems <= header->mItemLimit);
	}
	ADT_ASSERT_REAL_PTR_NIL(header->mHead, "");
}
#endif /* Debugging code. */


MemoryPoolPtr
adt_memory_pool_init(size_t size)
{
	return adt_bounded_memory_pool_init(size, 0);
}


MemoryPoolPtr
adt_bounded_memory_pool_init(size_t size, unsigned int max_items)
{
	MemoryPoolHeaderPtr result = (MemoryPoolHeaderPtr) ADT_XALLOCATE(sizeof(MemoryPoolHeader));

	if (!result)
	{
		return NULL;
	}

	result->mItemSize = size;
	result->mHead = NULL;
	result->mItems = 0;
	result->mItemLimit = max_items; /* 0 means unbounded. */
	result->mMagicNumber = kMemoryPoolMagicNumber;

#ifndef NDEBUG
	verify_memory_pool(result);
#endif /* Debugging code. */

	return (MemoryPoolPtr) result;
}


MemoryPoolPtr
adt_memory_pool_dispose(MemoryPoolPtr pool)
{
	MemoryPoolHeaderPtr header = (MemoryPoolHeaderPtr) pool;

#ifndef NDEBUG
	verify_memory_pool(header);
#endif /* Debugging code. */

	if (valid_memory_pool(header))
	{
		while (header->mHead)
		{
			AdtPtr temp = header->mHead;
	
			header->mHead = *((AdtPtr*) header->mHead);
	
			temp = adt_dispose_ptr(temp);
		}
	
		return (MemoryPoolPtr) adt_dispose_ptr(header);
	}

	return NULL;
}


AdtPtr
adt_memory_pool_get(MemoryPoolPtr pool)
{
	MemoryPoolHeaderPtr header = (MemoryPoolHeaderPtr) pool;

#ifndef NDEBUG
	verify_memory_pool(header);
#endif /* Debugging code. */

	if (valid_memory_pool(header))
	{
		if (header->mHead == NULL)
		{
			return ADT_XALLOCATE(header->mItemSize);
		}
		else
		{
			AdtPtr result = header->mHead;
			header->mHead = *((AdtPtr*) header->mHead);
			header->mItems--;
	
			return result;
		}
	}
	
	return NULL;
}


void
adt_memory_pool_put(MemoryPoolPtr pool, AdtPtr chunk)
{
	MemoryPoolHeaderPtr header = (MemoryPoolHeaderPtr) pool;

#ifndef NDEBUG
	verify_memory_pool(header);
	ADT_ASSERT_REAL_PTR(chunk, "");
	adt_trash_ptr(chunk, header->mItemSize);
#endif /* Debugging code. */

	if (valid_memory_pool(header))
	{
		if (header->mItemLimit && (header->mItems == header->mItemLimit))
		{
			chunk = adt_dispose_ptr(chunk);
		}
		else
		{
			*((AdtPtr*) chunk) = header->mHead;
			header->mHead = chunk;
			header->mItems++;
		}
	}
}


#if ADT_MEMORY_DEBUG
void
adt_memory_pool_note_ptrs(MemoryPoolPtr pool)
{
	MemoryPoolHeaderPtr header = (MemoryPoolHeaderPtr) pool;
	AdtPtr current;

#ifndef NDEBUG
	verify_memory_pool(header);
#endif /* Debugging code. */

	adt_memory_note_ptr(header);
	current = header->mHead;
	while (current)
	{
		adt_memory_note_ptr(current);
		
		current = *((AdtPtr*) current);
	}
}
#endif /* ADT_MEMORY_DEBUG */
