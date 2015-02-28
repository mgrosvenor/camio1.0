/* File header. */
#include "adt_hash_common.h"

/* ADT headers. */
#include "adt_memory.h"
#include "adt_memory_pool.h"

/* C Standard Library headers. */
#include <assert.h>
#include <string.h>


#define MAX_POOL_ITEMS 128
static MemoryPoolPtr uPool = NULL;


void
initialise_hash_common(void)
{
	if (!uPool)
	{
		uPool = adt_bounded_memory_pool_init(sizeof(HashEntry), MAX_POOL_ITEMS);
		assert(uPool);
	}
}



HashEntryPtr
new_hash_entry(AdtPtr data)
{
	HashEntryPtr entry = (HashEntryPtr) adt_memory_pool_get(uPool);

	if (entry)
	{
		entry->mElement = data;
		entry->mActive = 1;
		entry->mKey = 0;
	}

	return entry;
}


HashEntryPtr
new_keyed_hash_entry(unsigned int key, AdtPtr data)
{
	HashEntryPtr entry = (HashEntryPtr) adt_memory_pool_get(uPool);

	if (entry)
	{
		entry->mElement = data;
		entry->mActive = 1;
		entry->mKey = key;
	}

	return entry;
}


void
dispose_hash_entry(HashEntryPtr entry)
{
	adt_memory_pool_put(uPool, entry);
}


AdtPtr
allocate_array(unsigned int size)
{
	AdtPtr new_array = ADT_XALLOCATE(sizeof(AdtPtr) * size);

	if (new_array)
	{
		memset(new_array, 0, sizeof(AdtPtr) * size);
	}

	return new_array;
}


void
empty_hash_table(HashEntryPtr* table, unsigned int length, DisposeRoutine disposer)
{
	HashEntryPtr* entry = table;
	unsigned int index;

	if (disposer)
	{
		for (index = 0; index < length; index++)
		{
			if (*entry)
			{
				if ((*entry)->mElement)
				{
					disposer((*entry)->mElement);
				}

				dispose_hash_entry(*entry);
			}
			entry++;
		}
	}
	else
	{
		for (index = 0; index < length; index++)
		{
			if (*entry)
			{
				dispose_hash_entry(*entry);
			}

			entry++;
		}
	}

	memset(table, 0, sizeof(AdtPtr) * length);
}


#if ADT_MEMORY_DEBUG
void
adt_hash_common_note_ptrs(void)
{
	adt_memory_pool_note_ptrs(uPool);
}
#endif /* ADT_MEMORY_DEBUG */
