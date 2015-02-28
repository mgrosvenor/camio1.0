/* File header */
#include "adt_hash.h"

/* Koryn's Units */
#include "adt_debugging.h"
#include "adt_memory.h"

/* C Standard Library headers. */
#include <string.h>


/* Internal routines. */
static unsigned int is_prime(unsigned int value);


/* Implementation of internal routines. */
static unsigned int
is_prime(unsigned int value)
{
	unsigned int composite = 0;
	unsigned int next = 3;

	while ((composite == 0) && (next * next < value))
	{
		composite = (value % next) == 0;
		next++;
	}

	return !composite;
}



/* Standard hash functions */
uint32_t
next_prime(uint32_t value)
{
	unsigned int result = value;

	if (value % 2 == 0)
	{
		result++;
	}

	while (is_prime(result) == 0)
	{
		result += 2;
	}

	return result;
}


uint32_t
adt_ptr_hash(AdtPtr data, unsigned int size)
{
	/* Default is to hash the pointer address (probably not the best default). */
	return (uint32_t) ((uintptr_t) data) % size;
}


uint32_t
adt_string_hash(const char* data, unsigned int size)
{
	unsigned char* current = (unsigned char*) data;
	unsigned int result = 0;

	while (*current)
	{
		result = 37 * result + (*current);
		current++;
	}

	return result % size;
}


uint32_t
adt_pstring_ptr_hash(AdtPtr data, unsigned int size)
{
	char* key = (char*) data;
	unsigned int result = 0;
	unsigned int index;

	for (index = (unsigned char) key[0]; index > 0; index--)
	{
		result = 37 * result + (unsigned int) key[index];
	}

	return result % size;
}

