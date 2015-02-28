/* File header */
#include "adt_debugging.h"

/* C Standard Library headers. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#ifndef NDEBUG

static const char kTrashFillByte = (char) 0xE7;
static const char* kAuthorEmail = "koryn (at) clear (dot) net (dot) nz";


/* Internal routines. */
static void adt_abort(void);


/* Implementation of internal routines. */
static void
adt_abort(void)
{
	/* This routine exists so that a breakpoint can be placed here.  Useful when running under Cygwin. */
	abort();
}


/* Debugging API. */
void
adt_assert_code(unsigned int b, const char* message, const char* filename, unsigned int linenumber)
{
	if (!b)
	{
	#if USE_MAC_DEBUGSTR
		char error[256];

		snprintf((char*) &error[1], 255, "Error at line %d in file %s\n", linenumber, filename);
		error[0] = (unsigned char) strlen((char*) &error[1]);

		DebugStr(error);
	#else
		fprintf(stderr, "Error at line %d in file %s\n", linenumber, filename);
	#endif /* USE_MAC_DEBUGSTR */

		if (message && (strcmp(message, "") != 0))
		{
		#if USE_MAC_DEBUGSTR
			sprintf((char*) &error[1], "  Description: %s\n", message);
			error[0] = (unsigned char) strlen((char*) &error[1]);
			DebugStr(error);
		#else
			fprintf(stderr, "  Description: %s\n", message);
			adt_abort();
		#endif /* USE_MAC_DEBUGSTR */
		}
		else
		{
		#if USE_MAC_DEBUGSTR
		    snprintf((char*) &error[1], 255, "  No description available, contact %s for help.\n", kAuthorEmail);
			error[0] = (unsigned char) strlen((char*) &error[1]);
			DebugStr(error);
		#else
			fprintf(stderr, "  No description available, contact %s for help.\n", kAuthorEmail);
			adt_abort();
		#endif /* USE_MAC_DEBUGSTR */
		}
	}
}


void
adt_assert_int_is_boolean(int i, const char* message, const char* filename, unsigned int linenumber)
{
	adt_assert_code((i == 0) || (i == 1), message, filename, linenumber);
}


void
adt_assert_valid_ptr(AdtPtr p, const char* message, const char* filename, unsigned int linenumber)
{
	/*
	char* chars = (char *) p;
	char temp;
	*/
	adt_assert_code(p != NULL, message, filename, linenumber);
	adt_assert_code((uintptr_t) p % 4 == 0, message, filename, linenumber);

	/*
	 * Particularly good on protected memory systems.
	 * Should cause a segfault if the pointer is invalid.
	 * Unfortunately disabled because it doesn't play nicely with static data on systems
	 * like Mac OS X that can mark static data read-only :(
	 */

	/* If we get here then (chars) because of the assertion above. */
	/*
	temp = chars[0];
	chars[0] = kTrashFillByte;
	chars[0] = temp;
	*/
}


void
adt_assert_valid_ptr_nil(AdtPtr p, const char* message, const char* filename, unsigned int linenumber)
{
	if (p)
	{
		adt_assert_valid_ptr(p, message, filename, linenumber);
	}
}


void
adt_assert_valid_object(AdtPtr obj, const char* message, const char* filename, unsigned int linenumber)
{
	adt_assert_valid_ptr((AdtPtr) obj, message, filename, linenumber);
}


void
adt_assert_valid_object_nil(AdtPtr obj, const char* message, const char* filename, unsigned int linenumber)
{
	if (obj)
	{
		adt_assert_valid_object(obj, message, filename, linenumber);
	}
}


void
adt_assert_valid_cstring(char* sp, const char* message, const char* filename, unsigned int linenumber)
{
	adt_assert_valid_ptr((AdtPtr) sp, message, filename, linenumber);
#if USE_MAC_MEMORY
	adt_assert_code((SInt16) (sp[0]) <= GetPtrSize((AdtPtr) sp), message, filename, linenumber);
#endif /* USE_MAC_MEMORY */
}


void
adt_assert_valid_cstring_nil(char* sp, const char* message, const char* filename, unsigned int linenumber)
{
	if (sp)
	{
	    adt_assert_valid_cstring(sp, message, filename, linenumber);
	}
}


void
adt_assert_valid_pstring_ptr(PStringPtr sp, const char* message, const char* filename, unsigned int linenumber)
{
	adt_assert_valid_ptr((AdtPtr) sp, message, filename, linenumber);
#if USE_MAC_MEMORY
	adt_assert_code((SInt16) (sp[0]) <= GetPtrSize((AdtPtr) sp), message, filename, linenumber);
#endif /* USE_MAC_MEMORY */
}


void
adt_assert_valid_pstring_ptr_nil(PStringPtr sp, const char* message, const char* filename, unsigned int linenumber)
{
	if (sp)
	{
	    adt_assert_valid_pstring_ptr(sp, message, filename, linenumber);
	}
}


void
adt_fill_ptr(AdtPtr p, size_t size, char value)
{
	adt_assert_code(size > 0, "", __FILE__, __LINE__);
	adt_assert_valid_ptr_nil(p, "", __FILE__, __LINE__);

	if (p)
	{
		memset(p, value, size);
	}
}


void
adt_trash_ptr(AdtPtr p, size_t size)
{
	adt_fill_ptr(p, size, kTrashFillByte);
}


#if USE_MAC_MEMORY
void
adt_assert_valid_handle(Handle h, const char* message, const char* filename, unsigned int linenumber)
{
	adt_assert_valid_ptr((AdtPtr) h, message, filename, linenumber);
	adt_assert_valid_ptr(*h, message, filename, linenumber);
	adt_assert_code(RecoverHandle(*h) == h, message, filename, linenumber);
}


void
adt_assert_valid_handle_nil(Handle h, const char* message, const char* filename, unsigned int linenumber)
{
	if (h)
	{
		adt_assert_valid_handle(h, message, filename, linenumber);
	}
}


void
adt_assert_valid_string_handle(StringHandle sh, const char* message, const char* filename, unsigned int linenumber)
{
	adt_assert_valid_ptr((AdtPtr) sh, message, filename, linenumber);
	adt_assert_valid_ptr((AdtPtr) (*sh), message, filename, linenumber);
	adt_assert_code(RecoverHandle( *((Handle) sh) ) == (Handle) sh, message, filename, linenumber);
}


void
adt_assert_valid_string_handle_nil(StringHandle sh, const char* message, const char* filename, unsigned int linenumber)
{
	if (sh)
	{
		adt_assert_valid_string_handle(sh, message, filename, linenumber);
	}
}


void
adt_fill_handle(Handle h, size_t size, SInt16 value)
{
	adt_assert_valid_handle_nil(h, "", __FILE__, __LINE__);
	adt_fill_ptr(*h, size, value);
}


void
adt_trash_handle(Handle h, size_t size)
{
	adt_fill_handle(h, size, kTrashFillByte);
}
#endif /* USE_MAC_MEMORY */


void
adt_assert_code2(uintptr_t b, const char* message, const char* filename, const char* function_name, unsigned int linenumber)
{
	if (!b)
	{
		fprintf(stderr, "Error in %s (file %s, line %u)\n", function_name, filename, linenumber);

		if (message && (strcmp(message, "") != 0) && (strlen(message) > 0))
		{
			fprintf(stderr, "  Description: %s\n", message);
			adt_abort();
		}
		else
		{
			fprintf(stderr, "  No description available, contact %s for help.\n", kAuthorEmail);
			adt_abort();
		}
	}
}


void
adt_assert_false(const char* message, const char* filename, const char* function_name, unsigned int linenumber)
{
	fprintf(stderr, "Bug in %s (file %s, line %u)\n", function_name, filename, linenumber);

	if (message && (message[0] != '\0'))
	{
		fprintf(stderr, "  Description: %s\n", message);
		adt_abort();
	}
	else
	{
		fprintf(stderr, "  No description available, contact %s for help.\n", kAuthorEmail);
		adt_abort();
	}
}


void
adt_assert_int_is_boolean2(int i, const char* message, const char* filename, const char* function_name, unsigned int linenumber)
{
	adt_assert_code2((i == 0) || (i == 1), message, filename, function_name, linenumber);
}


void
adt_assert_valid_ptr_nil2(AdtPtr p, const char* message, const char* filename, const char* function_name, unsigned int linenumber)
{
	if (p)
	{
		adt_assert_code2((uintptr_t) p, message, filename, function_name, linenumber);
	}
}


void
adt_assert_real_ptr(AdtPtr p, const char* message, const char* filename, const char* function_name, unsigned int linenumber)
{
	char* chars = (char *) p;
	char temp;

	adt_assert_code2(p != NULL, message, filename, function_name, linenumber);
	adt_assert_code2((uintptr_t) p % 4 == 0, message, filename, function_name, linenumber);
	
	/*
	 * Particularly good on protected memory systems.
	 * Should cause a segfault if the pointer is invalid.
	 */

	/* If we get here then (chars) because of the assertion above. */
	temp = chars[0];
	chars[0] = kTrashFillByte;
	chars[0] = temp;
}


void
adt_assert_real_ptr_nil(AdtPtr p, const char* message, const char* filename, const char* function_name, unsigned int linenumber)
{
	if (p)
	{
		adt_assert_real_ptr(p, message, filename, function_name, linenumber);
	}
}


#endif /* Debugging code. */
