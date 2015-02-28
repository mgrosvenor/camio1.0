/* ------------------------------------------------------------------------------
 adt_array_stack from Koryn's Units.

 An array-based stack that automatically increases its size when necessary.
 The objects stored in the stack are generic Ptrs or Handles.

 Copyright (c) 2001-2004, Koryn Grant (koryn@clear.net.nz)
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

#ifndef ADT_ARRAY_STACK_H
#define ADT_ARRAY_STACK_H

/* Koryn's Units */
#include "adt_types.h"


typedef struct ArrayStack_ ArrayStack_;
typedef /*@abstract@*/ ArrayStack_* ArrayStackPtr; /* Type-safe opaque type. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Array-based Stack ADT */
/*@only@*/ /*@null@*/ ArrayStackPtr adt_array_stack_init(void);
/*@null@*/ ArrayStackPtr adt_array_stack_dispose(/*@only@*/ /*@notnull@*/ /*@in@*/ ArrayStackPtr stack);
unsigned int adt_array_stack_is_empty(/*@notnull@*/ /*@in@*/ ArrayStackPtr stack);
void adt_array_stack_make_empty(/*@notnull@*/ /*@in@*/ ArrayStackPtr stack);
void adt_array_stack_push(/*@notnull@*/ /*@in@*/ ArrayStackPtr stack, AdtPtr data);
void adt_array_stack_pop(/*@notnull@*/ /*@in@*/ ArrayStackPtr stack);
/*@null@*/ AdtPtr adt_array_stack_top(/*@notnull@*/ /*@in@*/ ArrayStackPtr stack);
/*@null@*/ AdtPtr adt_array_stack_top_and_pop(/*@notnull@*/ /*@in@*/ ArrayStackPtr stack);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_ARRAY_STACK_H */
