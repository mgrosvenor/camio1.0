/* ------------------------------------------------------------------------------
 adt_hash from Koryn's Units.

 Interface for a hash routine.

 Copyright (c) 2000-2003, Koryn Grant (koryn@clear.net.nz)
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

#ifndef ADT_HASH_H
#define ADT_HASH_H

/* Koryn's Units */
#include "adt_types.h"

/* C Standard Library headers. */
#include "dag_platform.h"  //#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Prototype for the hash function */
typedef uint32_t (*HashRoutine)(AdtPtr data, unsigned int size);


/* Standard hash functions */
uint32_t next_prime(uint32_t value);
uint32_t adt_ptr_hash(AdtPtr data, unsigned int size);
uint32_t adt_string_hash(const char* data, unsigned int size);
uint32_t adt_pstring_ptr_hash(AdtPtr data, unsigned int size);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_HASH_H */
