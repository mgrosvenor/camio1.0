/* ------------------------------------------------------------------------------
 adt_debugging from Koryn's Units.

 Assertions and memory-junking routines.

 Copyright (c) 1998-2003, Koryn Grant <koryn@clear.net.nz>.
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

#ifndef ADT_DEBUGGING_H
#define ADT_DEBUGGING_H

#ifndef NDEBUG
/* Endace headers */
#include "dag_platform.h"

/* Koryn's Units */
#include "adt_types.h"

/* C Standard Library headers. */
#include <stdlib.h>

#if defined(_WIN32)
	#define __func__     __FUNCTION__
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define ADT_ASSERT(b,msg) adt_assert_code2((b), (msg), __FILE__, __func__, __LINE__)
#define ADT_ASSERT_BUG(msg) adt_assert_false((msg), __FILE__, __func__, __LINE__)
#define ADT_ASSERT_PTR(p,msg) adt_assert_code2((uintptr_t) (p), (msg), __FILE__, __func__, __LINE__)
#define ADT_ASSERT_PTR_NIL(p,msg) adt_assert_valid_ptr_nil2((p), (msg), __FILE__, __func__, __LINE__)
#define ADT_ASSERT_REAL_PTR(p,msg) adt_assert_real_ptr((p), (msg), __FILE__, __func__, __LINE__)
#define ADT_ASSERT_REAL_PTR_NIL(p,msg) adt_assert_real_ptr_nil((p), (msg), __FILE__, __func__, __LINE__)
#define ADT_ASSERT_BOOLEAN(i,msg) adt_assert_int_is_boolean2((i), (msg), __FILE__, __func__, __LINE__)

void adt_assert_code(unsigned int b, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_int_is_boolean(int b, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_ptr(AdtPtr p, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_ptr_nil(AdtPtr p, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_object(AdtPtr obj, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_object_nil(AdtPtr obj, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_cstring(char* sp, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_cstring_nil(char* sp, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_pstring_ptr(PStringPtr sp, const char* message, const char* filename, unsigned int linenumber);
void adt_assert_valid_pstring_ptr_nil(PStringPtr sp, const char* message, const char* filename, unsigned int linenumber);

void adt_fill_ptr(AdtPtr p, size_t size, char value);
void adt_trash_ptr(AdtPtr p, size_t size);

#if USE_MACOS9_HANDLES
	void adt_assert_valid_handle(Handle h, const char* message, const char* filename, unsigned int linenumber);
	void adt_assert_valid_handle_nil(Handle h, const char* message, const char* filename, unsigned int linenumber);
	void adt_assert_valid_string_handle(StringHandle sh, const char* message, const char* filename, unsigned int linenumber);
	void adt_assert_valid_string_handle_nil(StringHandle sh, const char* message, const char* filename, unsigned int linenumber);
	void adt_fill_handle(Handle h, size_t size, char value);
	void adt_trash_handle(Handle h, size_t size);
#endif /* USE_MACOS9_HANDLES */

/* New routines that use the __func__ constant. */
void adt_assert_code2(uintptr_t b, const char* message, const char* filename, const char* function_name, unsigned int linenumber);
void adt_assert_false(const char* message, const char* filename, const char* function_name, unsigned int linenumber);
void adt_assert_int_is_boolean2(int b, const char* message, const char* filename, const char* function_name, unsigned int linenumber);
void adt_assert_valid_ptr_nil2(AdtPtr p, const char* message, const char* filename, const char* function_name, unsigned int linenumber);
void adt_assert_real_ptr(AdtPtr p, const char* message, const char* filename, const char* function_name, unsigned int linenumber);
void adt_assert_real_ptr_nil(AdtPtr p, const char* message, const char* filename, const char* function_name, unsigned int linenumber);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* Debugging code. */

#endif /* ADT_DEBUGGING_H */
