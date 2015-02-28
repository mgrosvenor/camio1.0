/* File header. */
#include "adt_quadratic_hash_table.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>

/* ADT headers. */
#include "adt_debugging.h"
#include "adt_hash_common.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"


static const unsigned int kDefaultHashTableSize = 7;
static const unsigned int kExpansionFactor = 2;


typedef struct QuadHashTableHeader QuadHashTableHeader;
typedef QuadHashTableHeader* QuadHashTableHeaderPtr;


struct QuadHashTableHeader
{
	adt_magic_number_t mMagicNumber;
	unsigned int mEntries;
	unsigned int mActiveEntries;
	unsigned int mCurrentSize;
	CompareRoutine mComparer;
	DisposeRoutine mDisposer;
	HashRoutine mHasher;
	HashEntryPtr* mTable; /* Array of HashEntryPtr. */
};


/* Internal routines. */
static unsigned int valid_header(QuadHashTableHeaderPtr header);
#ifndef NDEBUG
static void verify_hash_table(QuadHashTableHeaderPtr header);
#endif /* Debugging code. */
static unsigned int find_position(QuadHashTableHeaderPtr header, AdtPtr data);


/* Implementation of internal routines. */
static unsigned int
valid_header(QuadHashTableHeaderPtr header)
{
	if (header && (header->mMagicNumber == kQuadraticHashTableMagicNumber))
	{
		return 1;
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_hash_table(QuadHashTableHeaderPtr header)
{
	unsigned int entries = 0;
	unsigned int active_entries = 0;
	unsigned int index;

	assert(NULL != header);
	assert(NULL != header->mTable);
	assert(NULL != header->mComparer);
	assert(NULL != header->mHasher);

	assert(header->mCurrentSize > 0);
	assert(header->mActiveEntries <= header->mEntries);

	for (index = 0; index < header->mCurrentSize; index++)
	{
		HashEntryPtr entry = header->mTable[index];

		if (entry)
		{
			entries++;
			if (entry->mElement)
			{
				active_entries++;
			}
		}
	}

	assert(header->mEntries == entries);
	assert(header->mActiveEntries == active_entries);
}
#endif /* Debugging code. */


static unsigned int
find_position(QuadHashTableHeaderPtr header, AdtPtr data)
{
	unsigned int position = header->mHasher(data, header->mCurrentSize);
	int empty_position = -1;
	unsigned int collision = 0;
	HashEntryPtr entry;

#ifndef NDEBUG
	assert(NULL != data);
	verify_hash_table(header);
#endif /* Debugging code. */

	entry = header->mTable[position];
	while (entry)
	{
		if (NULL == entry->mElement)
		{
			if (-1 == empty_position)
			{
				/* First inactive entry in the chain. */
				empty_position = position;
			}
		}
		else if (header->mComparer(entry->mElement, data) == kComparisonEqual)
		{
			/* Found an exact match. */
#ifndef NDEBUG
			verify_hash_table(header);
#endif /* Debugging code. */

			return position;
		}

		collision++;
		position += 2 * collision - 1;
		if (position >= header->mCurrentSize)
		{
			position -= header->mCurrentSize;
		}
			
		entry = header->mTable[position];
	}

#ifndef NDEBUG
	verify_hash_table(header);
#endif /* Debugging code. */

	/* At this point position points to the first open slot after the chain,
	 * and empty_position points to the first open slot within the chain.
	 */
	if (-1 != empty_position)
	{
		/* Didn't get an exact match, but did find an inactive element within the chain. */
		return empty_position;
	}

	return position;
}



/* Implementation of Quadratic Probing Hash Table ADT. */
QuadHashTablePtr
adt_quad_hash_table_init(CompareRoutine comparer, DisposeRoutine disposer, HashRoutine hasher)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) ADT_XALLOCATE(sizeof(QuadHashTableHeader));
	if (!header)
	{
		return NULL;
	}

	header->mTable = allocate_array(kDefaultHashTableSize);
	if (!header->mTable)
	{
		(void) adt_dispose_ptr((AdtPtr) header);
		return NULL;
	}

	header->mCurrentSize = kDefaultHashTableSize;
	header->mEntries = 0;
	header->mActiveEntries = 0;
	header->mComparer = comparer;
	header->mDisposer = disposer;
	header->mHasher = hasher;
	header->mMagicNumber = kQuadraticHashTableMagicNumber;

#ifndef NDEBUG
	verify_hash_table(header);
#endif /* Debugging code. */

	initialise_hash_common();

	return (QuadHashTablePtr) header;
}


QuadHashTablePtr
adt_quad_hash_table_dispose(QuadHashTablePtr table)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	verify_hash_table(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		empty_hash_table(header->mTable, header->mCurrentSize, header->mDisposer);

		(void) adt_dispose_ptr((AdtPtr) header->mTable);
		(void) adt_dispose_ptr((AdtPtr) header);
	}

	return NULL;
}


void
adt_quad_hash_table_make_empty(QuadHashTablePtr table)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	verify_hash_table(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		empty_hash_table(header->mTable, header->mCurrentSize, header->mDisposer);
		header->mEntries = 0;
		header->mActiveEntries = 0;
		
#ifndef NDEBUG
		verify_hash_table(header);
#endif /* Debugging code. */
	}
}


unsigned int
adt_quad_hash_table_items(QuadHashTablePtr table)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	unsigned int count = 0;
	unsigned int length;
	unsigned int index;

	verify_hash_table(header);

	length = header->mCurrentSize;
	for (index = 0; index < length; index++)
	{
		HashEntryPtr entry = header->mTable[index];

		if (entry && entry->mElement)
		{
			count++;
		}
	}
	assert(count == header->mActiveEntries);
#endif /* Debugging code. */
	
	return header->mActiveEntries;
}


unsigned int
adt_quad_hash_table_insert(QuadHashTablePtr table, AdtPtr data)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	assert(NULL != data);
	verify_hash_table(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		unsigned int position = find_position(header, data);
		HashEntryPtr entry = header->mTable[position];

		if (entry == NULL)
		{
			/* Add a new entry to the table. */
			header->mTable[position] = new_hash_entry(data);
			header->mEntries++;
			header->mActiveEntries++;
			if (header->mEntries > header->mCurrentSize / 2)
			{
				/* Rehash if table is more than half full. */
				HashEntryPtr* old_array = header->mTable;
				unsigned int old_size = header->mCurrentSize;
				unsigned int new_size = next_prime(kExpansionFactor * old_size);
				unsigned int index;

				header->mTable = allocate_array(new_size);
				if (NULL == header->mTable)
				{
					/* Could not allocate new memory, so replace the old table and return. */
					header->mTable = old_array;
					return 0;
				}
				header->mCurrentSize = new_size;
				header->mEntries = 0;
				header->mActiveEntries = 0;

				/* Copy the entries to the new array. */
				for (index = 0; index < old_size; index++)
				{
					entry = old_array[index];
					if (NULL != entry)
					{
						if (NULL != entry->mElement)
						{
							/* Already allocated enough space so this should never fail. */
							position = find_position(header, entry->mElement);

							assert(NULL == header->mTable[position]);

							header->mTable[position] = new_hash_entry(entry->mElement);
							header->mEntries++;
							header->mActiveEntries++;
						}

						dispose_hash_entry(entry);
					}
				}

				/* Dispose of the old table. */
				(void) adt_dispose_ptr((AdtPtr) old_array);
			}
		}
		else
		{
			/* Use the existing entry, replacing the mElement field with new data. */
			if (NULL != entry->mElement)
			{
				assert(header->mDisposer); /* If a disposer has not been provided then memory will leak. */
				if (header->mDisposer)
				{
					header->mDisposer(entry->mElement);
				}
			}
			else
			{
				/* Adding a new element. */
				header->mActiveEntries++;
			}

			entry->mElement = data;
		}

#ifndef NDEBUG
		verify_hash_table(header);
#endif /* Debugging code. */

		return 1;
	}

	return 0;
}


void
adt_quad_hash_table_remove(QuadHashTablePtr table, AdtPtr data, unsigned int dispose)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	assert(NULL != data);
	verify_hash_table(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		unsigned int position = find_position(header, data);
		HashEntryPtr entry = header->mTable[position];

		if (entry && entry->mElement)
		{
			if (dispose)
			{
				assert(header->mDisposer);
				header->mDisposer(entry->mElement);
			}
			entry->mElement = NULL;
			header->mActiveEntries--;
		}

#ifndef NDEBUG
		verify_hash_table(header);
#endif /* Debugging code. */
	}
}


AdtPtr
adt_quad_hash_table_find(QuadHashTablePtr table, AdtPtr data)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	assert(NULL != data);
	verify_hash_table(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		unsigned int position = find_position(header, data);
		HashEntryPtr entry = header->mTable[position];

#ifndef NDEBUG
		verify_hash_table(header);
#endif /* Debugging code. */

		if (entry && entry->mElement)
		{
			return entry->mElement;
		}
#ifndef NDEBUG
		else
		{
			/* Verify that the entry was not present. */
			unsigned int index;
			
			for (index = 0; index < header->mCurrentSize; index++)
			{
				HashEntryPtr temp_entry = header->mTable[index];

				if (temp_entry && (temp_entry->mElement) && 
					(header->mComparer(temp_entry->mElement, data) == kComparisonEqual))
				{
					assert(0); /* Hash table couldn't find an entry which was present. */
				}
			}	
		}
#endif /* Debugging code. */
	}

	return NULL;
}


AdtPtr
adt_quad_hash_table_retrieve(QuadHashTablePtr table, unsigned int count)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	assert(0 < count);
	assert(count <= header->mActiveEntries);
	verify_hash_table(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		unsigned int length = header->mCurrentSize;
		unsigned int test = 0;
		unsigned int index;

		for (index = 0; index < length; index++)
		{
			HashEntryPtr entry = header->mTable[index];

			if (entry && entry->mElement)
			{
				test++;
				if (test == count)
				{
					return entry->mElement;
				}
			}
		}

#ifndef NDEBUG
		verify_hash_table(header);
#endif /* Debugging code. */
	}
	
	return NULL;
}


void
adt_quad_hash_table_iterate(QuadHashTablePtr table, IterationCallback iterator, AdtPtr context)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;

#ifndef NDEBUG
	verify_hash_table(header);
	assert(iterator);
#endif /* Debugging code. */

	if (valid_header(header) && iterator)
	{
		unsigned int length = header->mCurrentSize;
		unsigned int index;

		for (index = 0; index < length; index++)
		{
			HashEntryPtr entry = header->mTable[index];

			if (entry && entry->mElement)
			{
				if (0 == iterator(entry->mElement, context))
				{
					break;
				}
			}
		}

#ifndef NDEBUG
		verify_hash_table(header);
#endif /* Debugging code. */
	}
}


#if ADT_MEMORY_DEBUG
void
adt_quad_hash_table_note_ptrs(QuadHashTablePtr table)
{
	QuadHashTableHeaderPtr header = (QuadHashTableHeaderPtr) table;
	unsigned int count;
	unsigned int index;

	verify_hash_table(header);

	adt_memory_note_ptr((AdtPtr) header);
	adt_memory_note_ptr(header->mTable);
	adt_hash_common_note_ptrs();
	
	count = header->mCurrentSize;
	for (index = 0; index < count; index++)
	{
		HashEntryPtr entry = header->mTable[index];
		if (entry)
		{
			adt_memory_note_ptr((AdtPtr) entry);
		}
	}

	verify_hash_table(header);
}
#endif /* ADT_MEMORY_DEBUG */
