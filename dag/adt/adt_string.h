/* ------------------------------------------------------------------------------
 adt_string from Koryn's Units.

 Miscellaneous string manipulation routines.

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

#ifndef ADT_STRING_H
#define ADT_STRING_H


/* C Standard Library headers. */
#include "dag_platform.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void canonicalize_string(char* buffer);
char* preprocess_string(char* line, char** remainder);

/*
 * strcasecmp, strncasecmp:
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

int adt_strcasecmp(const char *s1, const char *s2);
int adt_strncasecmp(const char *s1, const char *s2, size_t n);

/*
 * strlcat:
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t adt_strlcat(char *dst, const char *src, size_t siz);

/*
 * strlcpy:
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t adt_strlcpy(char *dst, const char *src, size_t siz);


int adt_strtobool(const char* source);
int adt_strtoi(int default_value, const char* source);
unsigned int adt_strtoui(unsigned int default_value, const char* source);
void adt_lowercase_string(const char* str, char* buffer, unsigned int buflen);
void adt_uppercase_string(const char* str, char* buffer, unsigned int buflen);


/* Algorithms found at http://www-igm.univ-mlv.fr/~lecroq/string/index.html */
/* String search routines. Return index of pattern or -1 if not found. */
int not_so_naive(const uint8_t *pattern, int m, const uint8_t *text, int n);
int morris_pratt(const uint8_t *pattern, int m, const uint8_t *text, int n);
int knuth_morris_pratt(const uint8_t *pattern, int m, const uint8_t *text, int n);
int apostolico_crochemore(const uint8_t *pattern, int m, const uint8_t *text, int n);
int boyer_moore(const uint8_t *pattern, int m, const uint8_t *text, int n);
int turbo_boyer_moore(const uint8_t *pattern, int m, const uint8_t *text, int n);
int tuned_boyer_moore(const uint8_t *pattern, int m, const uint8_t *text, int n);
int apostolico_giancarlo(const uint8_t *pattern, int m, const uint8_t *text, int n);
int quick_search(const uint8_t *pattern, int m, const uint8_t *text, int n);
int smith(const uint8_t *pattern, int m, const uint8_t *text, int n);
int colussi(const uint8_t *pattern, int m, const uint8_t *text, int n);
int galil_giancarlo(const uint8_t *pattern, int m, const uint8_t *text, int n);
int reverse_factor(const uint8_t *pattern, int m, const uint8_t *text, int n);
int turbo_reverse_factor(const uint8_t *pattern, int m, const uint8_t *text, int n);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_STRING_H */
