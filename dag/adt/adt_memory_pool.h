/* ------------------------------------------------------------------------------
 adt_memory_pool from Koryn's Units.

 ADT for a memory pool that implements a free list for common structures.

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

#ifndef ADT_MEMORY_POOL_H
#define ADT_MEMORY_POOL_H

/* Koryn's Units. */
#include "adt_types.h"

/* C Standard Library headers. */
#include <stdlib.h>


typedef struct MemoryPool_ MemoryPool_;
typedef MemoryPool_* MemoryPoolPtr; /* Type-safe opaque type. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


MemoryPoolPtr adt_memory_pool_init(size_t size);
MemoryPoolPtr adt_bounded_memory_pool_init(size_t size, unsigned int max_items);
MemoryPoolPtr adt_memory_pool_dispose(MemoryPoolPtr pool);

AdtPtr adt_memory_pool_get(MemoryPoolPtr pool);
void adt_memory_pool_put(MemoryPoolPtr pool, AdtPtr pointer);

#if ADT_MEMORY_DEBUG
void adt_memory_pool_note_ptrs(MemoryPoolPtr pool);
#endif /* ADT_MEMORY_DEBUG */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_MEMORY_POOL_H */
