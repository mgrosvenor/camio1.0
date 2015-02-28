/* ------------------------------------------------------------------------------
 adt_comparison from Koryn's Units.

 Interface for a comparison routine, used in binary trees, hash tables and sorting.

 Copyright (c) 2000-2004, Koryn Grant (koryn@clear.net.nz)
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

#ifndef ADT_COMPARISON_H
#define ADT_COMPARISON_H

/* Koryn's Units */
#include "adt_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Standard constants to make understanding code that uses comparison functions easier to understand */
typedef enum
{
	kComparisonFirstIsSmaller = -1,
	kComparisonEqual = 0,
	kComparisonSecondIsSmaller = 1,

	/* Alternatives. */
	kComparisonFirstIsBigger = kComparisonSecondIsSmaller,
	kComparisonSecondIsBigger = kComparisonFirstIsSmaller

} comparison_t;


/* Prototype for the comparison function */
typedef comparison_t (*CompareRoutine)(AdtPtr first, AdtPtr second);


/* Standard comparison functions */
comparison_t adt_ptr_compare(AdtPtr first, AdtPtr second);
comparison_t adt_string_compare(const char* first, const char* second);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_COMPARISON_H */
