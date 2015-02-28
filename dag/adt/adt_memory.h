/* ------------------------------------------------------------------------------
 adt_memory from Koryn's Units.

 Allocation and deallocation of Ptrs and Handles that is a little nicer than the
 Memory Managers routines.  Using these consistently also allows you to hook in
 a memory checking subsystem like that described in Writing Solid Code if desired.

 Copyright (c) 1998-2004, Koryn Grant <koryn@clear.net.nz>.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 *	Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.
 *	Neither the name of the author nor the names of other contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------ */

#ifndef ADT_MEMORY_H
#define ADT_MEMORY_H

/* Koryn's Units. */
#include "adt_types.h"

/* C Standard Library headers. */
#include "dag_platform.h"  //#include <inttypes.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef ADT_MEMORY_DEBUG
	#define ADT_MEMORY_DEBUG 0
#endif 

#if ADT_MEMORY_DEBUG
#define ADT_XALLOCATE(x) adt_xallocate_ptr_ex(x, __FILE__, __LINE__)
#define ADT_XALLOCATE_CSTRING(x) adt_xallocate_cstring_ex(x, __FILE__, __LINE__)
#else
#define ADT_XALLOCATE(x) adt_xallocate_ptr(x)
#define ADT_XALLOCATE_CSTRING(x) adt_xallocate_cstring(x)
#endif /* ADT_MEMORY_DEBUG */

#ifdef USE_MACOS9_HANDLES 
#if USE_MACOS9_HANDLES
Handle adt_allocate_handle(size_t size);
Handle adt_dispose_handle(Handle h);
#endif /* USE_MACOS9_HANDLES */
#endif /* USE_MACOS9_HANDLES */

PStringPtr adt_allocate_pstring_ptr(const char* line);
PStringPtr adt_dispose_pstring_ptr(PStringPtr theStringPtr);

const char* adt_allocate_cstring(const char* line);


/* The X versions call the MemoryErrorRoutine (if set) on an error.
 * The default MemoryErrorRoutine simply returns a NULL pointer().
 */

typedef void (*MemoryErrorRoutine)(size_t size);

void adt_set_xallocate_error_routine(MemoryErrorRoutine error_routine);

/* General allocation routines. */
AdtPtr adt_xallocate_ptr(size_t size);
AdtPtr adt_dispose_ptr(AdtPtr p);

/* Pascal string to C string. */
PStringPtr adt_xallocate_pstring_ptr(const char* cstring);

/* Duplicate C string. */
const char* adt_xallocate_cstring(const char* cstring);

#if ADT_MEMORY_DEBUG
/* Advanced debugging routines. */
AdtPtr adt_xallocate_ptr_ex(size_t size, const char* sourcefile, unsigned int linenumber);
const char* adt_xallocate_cstring_ex(const char* cstring, const char* sourcefile, unsigned int linenumber);

/* Memory verification routines. */
void adt_memory_clear_refs(void);
void adt_memory_note_ptr(AdtPtr p);
void adt_memory_verify_refs(const char* logfilename, unsigned int nasty);
unsigned int adt_memory_valid_ptr(AdtPtr p);

/* Support routines.  If filename is null then 'adt_memory_dump_YYYYMMDD.txt' is used. */
void adt_memory_log(const char* filename);
#endif /* ADT_MEMORY_DEBUG */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_MEMORY_H */
