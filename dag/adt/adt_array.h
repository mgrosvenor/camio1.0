/* ------------------------------------------------------------------------------
 adt_array from Koryn's Units.

 An array with bounds checking and automatic disposal of pointer-based items.

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

#ifndef ADT_ARRAY_H
#define ADT_ARRAY_H

/* Koryn's Units */
#include "adt_disposal.h"
#include "adt_iterate.h"


typedef struct Array_ Array_;
typedef Array_* ArrayPtr; /* Type-safe opaque type. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Array ADT routines. */
ArrayPtr adt_array_init(DisposeRoutine disposer, unsigned int size); /* If size is 0 the array is completely auto-sized when items are added. */
ArrayPtr adt_array_dispose(ArrayPtr array);
void adt_array_make_empty(ArrayPtr array);
unsigned int adt_array_put(ArrayPtr array, unsigned int location, AdtPtr data); /* Returns 1 if succeeded. */
AdtPtr adt_array_get(ArrayPtr array, unsigned int location);
unsigned int adt_array_items(ArrayPtr array);
void adt_array_iterate(ArrayPtr array, IterationCallback iterator, AdtPtr user_data);

#if ADT_MEMORY_DEBUG
void adt_array_note_ptrs(ArrayPtr array);
#endif /* ADT_MEMORY_DEBUG */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_ARRAY_H */
