/* File header. */
#include "adt_string.h"

/* ADT headers. */
#include "adt_debugging.h"
#include "adt_memory.h"

/* C Standard Library headers. */
#include <assert.h>
#include <ctype.h>
#include "dag_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Profiling code deactivated but still present in case of future use. */
#define ADT_STRING_SEARCH_PROFILING 0
#define ALPHABET_SIZE 256
#define STRING_SIZE 2048

typedef struct
{
	int vertexNumber;
	int edgeNumber;
	int vertexCounter; 
	int initial;
	int* terminal; 
	int* target; 
	int* suffixLink;
	int* length; 
	int* position; 
	int* shift;
 
} _graph, * Graph;
   
#define UNDEFINED -1

/* Internal routines. */
static inline void pre_morris_pratt(const uint8_t *x, int m, int mpNext[]);
static inline void pre_knuth_morris_pratt(const uint8_t *x, int m, int kmpNext[]);
static inline void pre_boyer_moore_bad_char(const uint8_t *x, int m, int bmBc[]);
static inline void suffixes(const uint8_t *x, int m, int* suff);
static inline void pre_boyer_moore_good_suffix(const uint8_t *x, int m, int bmGs[], int suff[]);
static inline void pre_quick_search_bad_char(const uint8_t *x, int m, int qsBc[]);
static inline int pre_colussi(const uint8_t *x, int m, int h[], int next[], int shift[]);

/* Automata routines. */
static inline Graph newGraph(int v, int e); 
static inline Graph newAutomaton(int v, int e); 
static inline Graph newSuffixAutomaton(int v, int e); 
static inline int newVertex(Graph g); 
static inline int getInitial(Graph g); 
static inline int isTerminal(Graph g, int v); 
static inline void setTerminal(Graph g, int v); 
static inline int getTarget(Graph g, int v, uint8_t c); 
static inline void setTarget(Graph g, int v, uint8_t c, int t); 
static inline int getSuffixLink(Graph g, int v); 
static inline void setSuffixLink(Graph g, int v, int s); 
static inline int getLength(Graph g, int v); 
static inline void setLength(Graph g, int v, int ell); 
static inline int getPosition(Graph g, int v); 
static inline void setPosition(Graph g, int v, int p); 
static inline int getShift(Graph g, int v, uint8_t c); 
static inline void setShift(Graph g, int v, uint8_t c, int s); 
static inline void copyVertex(Graph g, int target, int source);


/* Implementation of internal routines. */
static inline void 
pre_morris_pratt(const uint8_t *x, int m, int mpNext[]) 
{
	int i;
	int j;
	
	i = 0;
	j = mpNext[0] = -1;
	while (i < m) 
	{
		while ((j > -1) && (x[i] != x[j]))
		{
			j = mpNext[j];
		}

		mpNext[++i] = ++j;
	}
}


static inline void 
pre_knuth_morris_pratt(const uint8_t *x, int m, int kmpNext[])
{
	int i;
	int j;

	i = 0;
	j = kmpNext[0] = -1;
	while (i < m) 
	{
		while ((j > -1) && (x[i] != x[j]))
		{
			j = kmpNext[j];
		}
	  
		i++;
		j++;
		if (x[i] == x[j])
		{
			kmpNext[i] = kmpNext[j];
		}
		else
		{
			kmpNext[i] = j;
		}
	}
}


static inline void
pre_boyer_moore_bad_char(const uint8_t *x, int m, int bmBc[])
{
	int i;
 
	for (i = 0; i < ALPHABET_SIZE; ++i)
	{
		bmBc[i] = m;
	}

	for (i = 0; i < m - 1; ++i)
	{
		bmBc[x[i]] = m - i - 1;
	}
}
 

static inline void
suffixes(const uint8_t *x, int m, int*suff)
{
	int f = 0;
	int g;
	int i;
 
	suff[m - 1] = m;
	g = m - 1;
	for (i = m - 2; i >= 0; --i)
	{
		if (i > g && suff[i + m - 1 - f] < i - g)
		{
			suff[i] = suff[i + m - 1 - f];
		}
		else 
		{
			if (i < g)
			{
				g = i;
			}

			f = i;
			while (g >= 0 && x[g] == x[g + m - 1 - f])
			{
				--g;
			}
			suff[i] = f - g;
		}
	}
}


static inline void
pre_boyer_moore_good_suffix(const uint8_t *x, int m, int bmGs[], int suff[])
{
	int i;
	int j;
 
	suffixes(x, m, suff);
 
	for (i = 0; i < m; ++i)
	{
		bmGs[i] = m;
	}

	j = 0;
	for (i = m - 1; i >= -1; --i)
	{      
		if (i == -1 || suff[i] == i + 1)
		{
			for (; j < m - 1 - i; ++j)
			{
				if (bmGs[j] == m)
				{
					bmGs[j] = m - 1 - i;
				}
			}
		}
	}

	for (i = 0; i <= m - 2; ++i)
	{
		bmGs[m - 1 - suff[i]] = m - 1 - i;
	}
}


static inline void
pre_quick_search_bad_char(const uint8_t *x, int m, int qsBc[])
{
	int i;
	
	for (i = 0; i < ALPHABET_SIZE; ++i)
	{
		qsBc[i] = m + 1;
	}
	
	for (i = 0; i < m; ++i)
	{
		qsBc[x[i]] = m - i;
	}
}


static inline int 
pre_colussi(const uint8_t *x, int m, int h[], int next[], int shift[])
{
	int i;
	int k;
	int nd;
	int q;
	int r = 0;
	int s;
	int hmax[STRING_SIZE];
	int kmin[STRING_SIZE];
	int nhd0[STRING_SIZE];
	int rmin[STRING_SIZE];

	/* Computation of hmax. */
	i = k = 1;
	do 
	{
		while (x[i] == x[i - k])
		{
			i++;
		}
		hmax[k] = i;
		q = k + 1;
		while (hmax[q - k] + k < i)
		{
			hmax[q] = hmax[q - k] + k;
			q++;
		}
		k = q;
		if (k == i + 1)
		{
			i = k;
		}
	} while (k <= m);

	/* Computation of kmin. */
	memset(kmin, 0, m*sizeof(int));
	for (i = m; i >= 1; --i)
	{
		if (hmax[i] < m)
		{
			kmin[hmax[i]] = i;
		}
	}

	/* Computation of rmin. */
	for (i = m - 1; i >= 0; --i)
	{
		if (hmax[i + 1] == m)
		{
			r = i + 1;
		}

		if (kmin[i] == 0)
		{
			rmin[i] = r;
		}
		else
		{
			rmin[i] = 0;
		}
	}

	/* Computation of h. */
	s = -1;
	r = m;
	for (i = 0; i < m; ++i)
	{
		if (kmin[i] == 0)
		{
			h[--r] = i;
		}
		else
		{
			h[++s] = i;
		}
	}
	nd = s;
 
	/* Computation of shift. */
	for (i = 0; i <= nd; ++i)
	{
		shift[i] = kmin[h[i]];
	}
	for (i = nd + 1; i < m; ++i)
	{
		shift[i] = rmin[h[i]];
	}
	shift[m] = rmin[0];

	/* Computation of nhd0. */
	s = 0;
	for (i = 0; i < m; ++i)
	{
		nhd0[i] = s;
		if (kmin[i] > 0)
		{
			++s;
		}
	}

	/* Computation of next. */
	for (i = 0; i <= nd; ++i)
	{
		next[i] = nhd0[h[i] - kmin[h[i]]];
	}
	for (i = nd + 1; i < m; ++i)
	{
		next[i] = nhd0[m - rmin[h[i]]];
	}

	next[m] = nhd0[m - rmin[h[m - 1]]];

	return(nd);
}


/* returns a new data structure for
   a graph with v vertices and e edges */
static inline Graph 
newGraph(int v, int e) 
{
	Graph g = (Graph) ADT_XALLOCATE(sizeof(_graph));
	
	assert(g);
	memset((AdtPtr) g, 0, sizeof(_graph));

	g->vertexNumber = v;
	g->edgeNumber = e;
	g->initial = 0;
	g->vertexCounter = 1;

	return g;
}


static inline void
disposeGraph(Graph g)
{
	if (g)
	{
		if (g->suffixLink)
		{
			adt_dispose_ptr((AdtPtr)g->suffixLink);
		}

		if (g->length)
		{
			adt_dispose_ptr((AdtPtr)g->length);
		}

		if (g->position)
		{
			adt_dispose_ptr((AdtPtr)g->position);
		}

		if (g->shift)
		{
			adt_dispose_ptr((AdtPtr)g->shift);
		}

		if (g->terminal)
		{
			adt_dispose_ptr((AdtPtr)g->terminal);
		}

		if (g->target)
		{
			adt_dispose_ptr((AdtPtr)g->target);
		}

		adt_dispose_ptr((AdtPtr)g);
	}
}


/* returns a new data structure for
   an automaton with v vertices and e edges */
static inline Graph 
newAutomaton(int v, int e) 
{
	Graph aut = newGraph(v, e);

	aut->target = (int*) ADT_XALLOCATE(e * sizeof(int));
	assert(aut->target);
	memset(aut->target, 0, e * sizeof(int));

	aut->terminal = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->terminal);
	memset(aut->terminal, 0, v * sizeof(int));

	return aut;
}


/* returns a new data structure for
   a suffix automaton with v vertices and e edges */
static inline Graph 
newSuffixAutomaton(int v, int e) 
{
	Graph aut = newAutomaton(v, e);

	memset(aut->target, UNDEFINED, e*sizeof(int));

	aut->suffixLink = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->suffixLink);
	memset(aut->suffixLink, 0, v * sizeof(int));

	aut->length = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->length);
	memset(aut->length, 0, v * sizeof(int));

	aut->position = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->position);
	memset(aut->position, 0, v * sizeof(int));

	aut->shift = (int*) ADT_XALLOCATE(e * sizeof(int));
	assert(aut->shift);
	memset(aut->shift, 0, e * sizeof(int));

	return aut;
}
 
 
/* returns a new data structure for
   a trie with v vertices and e edges */
static inline Graph 
newTrie(int v, int e) 
{
	Graph aut = newAutomaton(v, e);
 
	memset(aut->target, UNDEFINED, e*sizeof(int));

	aut->suffixLink = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->suffixLink);
	memset(aut->suffixLink, 0, v * sizeof(int));

	aut->length = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->length);
	memset(aut->length, 0, v * sizeof(int));

	aut->position = (int*) ADT_XALLOCATE(v * sizeof(int));
	assert(aut->position);
	memset(aut->position, 0, v * sizeof(int));

	aut->shift = (int*) ADT_XALLOCATE(e * sizeof(int));
	assert(aut->shift);
	memset(aut->shift, 0, e * sizeof(int));

	return aut;
}


/* returns a new vertex for graph g */
static inline int 
newVertex(Graph g) 
{
	assert(g);
	assert(g->vertexCounter <= g->vertexNumber);

	g->vertexCounter++;
	return g->vertexCounter;
}


/* returns the initial vertex of graph g */
static inline int 
getInitial(Graph g) 
{
	assert(g);

	return g->initial;
}


/* returns true if vertex v is terminal in graph g */
static inline int 
isTerminal(Graph g, int v) 
{
	assert(g);
	assert(g->terminal);
	assert(v < g->vertexNumber);

	return g->terminal[v];
}


/* set vertex v to be terminal in graph g */
static inline void 
setTerminal(Graph g, int v) 
{
	assert(g);
	assert(g->terminal);
	assert(v < g->vertexNumber);

	g->terminal[v] = 1;
}


/* returns the target of edge from vertex v
   labelled by character c in graph g */
static inline int 
getTarget(Graph g, int v, uint8_t c) 
{
	assert(g);
	assert(g->target);
	assert(v < g->vertexNumber);
	assert(v*c < g->edgeNumber);

	return g->target[c + v*(g->edgeNumber / g->vertexNumber)];
}


/* add the edge from vertex v to vertex t
   labelled by character c in graph g */
static inline void 
setTarget(Graph g, int v, uint8_t c, int t) 
{
	assert(g);
	assert(g->target);
	assert(v < g->vertexNumber);
	assert(v*c < g->edgeNumber);
	assert(t < g->vertexNumber);

	g->target[c + v*(g->edgeNumber / g->vertexNumber)] = t;
}


/* returns the suffix link of vertex v in graph g */
static inline int 
getSuffixLink(Graph g, int v) 
{
	assert(g);
	assert(g->suffixLink);
	assert(v < g->vertexNumber);

	return g->suffixLink[v];
}


/* set the suffix link of vertex v
   to vertex s in graph g */
static inline void 
setSuffixLink(Graph g, int v, int s) 
{
	assert(g);
	assert(g->suffixLink);
	assert(v < g->vertexNumber);
	assert(s < g->vertexNumber);

	g->suffixLink[v] = s;
}


/* returns the length of vertex v in graph g */
static inline int 
getLength(Graph g, int v) 
{
	assert(g);
	assert(g->length);
	assert(v < g->vertexNumber);

	return g->length[v];
}


/* set the length of vertex v to integer ell in graph g */
static inline void 
setLength(Graph g, int v, int ell) 
{
	assert(g);
	assert(g->length);
	assert(v < g->vertexNumber);

	g->length[v] = ell;
}


/* returns the position of vertex v in graph g */
static inline int 
getPosition(Graph g, int v) 
{
	assert(g);
	assert(g->position);
	assert(v < g->vertexNumber);
	
	return g->position[v];
}


/* set the length of vertex v to integer ell in graph g */
static inline void 
setPosition(Graph g, int v, int p) 
{
	assert(g);
	assert(g->position);
	assert(v < g->vertexNumber);
	
	g->position[v] = p;
}


/* returns the shift of the edge from vertex v
   labelled by character c in graph g */
static inline int 
getShift(Graph g, int v, uint8_t c) 
{
	assert(g);
	assert(g->shift);
	assert(v < g->vertexNumber);
	assert(v*c < g->edgeNumber);

	return g->shift[c + v * (g->edgeNumber / g->vertexNumber)];
}


/* set the shift of the edge from vertex v
   labelled by character c to integer s in graph g */
static inline void
setShift(Graph g, int v, uint8_t c, int s) 
{
	assert(g);
	assert(g->shift);
	assert(v < g->vertexNumber);
	assert(v*c < g->edgeNumber);

	g->shift[c + v * (g->edgeNumber / g->vertexNumber)] = s;
}


/* copies all the characteristics of vertex source
   to vertex target in graph g */
static inline void
copyVertex(Graph g, int target, int source) 
{
	assert(g);
	assert(target < g->vertexNumber);
	assert(source < g->vertexNumber);

	if (g->target)
	{
		memcpy(g->target + target * (g->edgeNumber / g->vertexNumber),
			   g->target + source * (g->edgeNumber / g->vertexNumber),
			   (g->edgeNumber / g->vertexNumber) * sizeof(int));
	}
		
	if (g->shift)
	{
		memcpy(g->shift + target * (g->edgeNumber / g->vertexNumber),
			   g->shift + source * (g->edgeNumber / g->vertexNumber),
			   (g->edgeNumber / g->vertexNumber) * sizeof(int));
	}
	
	if (g->terminal)
	{
		g->terminal[target] = g->terminal[source];
	}

	if (g->suffixLink)
	{
		g->suffixLink[target] = g->suffixLink[source];
	}

	if (g->length)
	{
		g->length[target] = g->length[source];
	}

	if (g->position)
	{
		g->position[target] = g->position[source];
	}	
}


/* External routines. */
void
canonicalize_string(char* buffer)
{
	unsigned int readpos = 0;
	unsigned int writepos = 0;
	size_t       buflen = strlen(buffer);
	unsigned int seen_char = 1;

	/* Ignore leading whitespace. */
	while (isspace(buffer[readpos]))
	{
		readpos++;
	}

	if (readpos == buflen)
	{
		fprintf(stdout, "adt_string_utilites: canonicalize_string(): string was entirely whitespace.\n");
		buffer[0] = '\0';
		return;
	}

	/* Now enter a basic state machine. */
	while (readpos < buflen)
	{
		char current = buffer[readpos];

		if (seen_char)
		{
			buffer[writepos] = current;
			writepos++;

			if (isspace(current))
			{
				seen_char = 0;
			}
		}
		else
		{
			if (isgraph(current))
			{
				buffer[writepos] = current;
				writepos++;
				seen_char = 1;
			}
			else
			{
				fprintf(stdout, "adt_string_utilites: canonicalize_string(): trimming whitespace.\n");
			}
		}

		readpos++;
	}

	/* Remove trailing whitespace. */
	if ((writepos > 0) && (buffer[writepos - 1] == ' '))
	{
		fprintf(stdout, "adt_string_utilites: canonicalize_string(): removing trailing whitespace.\n");
		writepos--;
	}

	buffer[writepos] = '\0';
}


static const unsigned int kWhitespace __attribute__((unused)) = 1;
static const unsigned int kInToken = 2;
static const unsigned int kBeyondToken = 3;
static const unsigned int kPreTokenWhitespace = 4;
static const unsigned int kPostTokenWhitespace = 5;
static const unsigned int kInRemainder __attribute__((unused)) = 6;


char*
preprocess_string(char* line, char** remainder)
{
	unsigned int token_index = 0;
	unsigned int remainder_index = 0;
	size_t length = strlen(line);
	char token_buffer[256] = "";
	char remainder_buffer[256] = "";

	char* token;
	unsigned int index;
	unsigned int state;

	/* Short-circuit test. */
	if ((line == NULL) || (line[0] == '\0') || (length > 255))
	{
		*remainder = NULL;
		return NULL;
	}
	else if ((length == 1) && isspace(line[0]))
	{
		*remainder = NULL;
		return NULL;
	}

	state = kPreTokenWhitespace;
	for (index = 0; index < length; index++)
	{
		if (state == kPreTokenWhitespace)
		{
			if (!isspace(line[index]))
			{
				state = kInToken;
				token_buffer[token_index] = line[index];
				token_index++;
			}
		}
		else if (state == kInToken)
		{
			if (isspace(line[index]))
			{
				state = kPostTokenWhitespace;
			}
			else
			{
				token_buffer[token_index] = line[index];
				token_index++;
			}
		}
		else if (state == kPostTokenWhitespace)
		{
			if (!isspace(line[index]))
			{
				state = kBeyondToken;
				remainder_buffer[remainder_index] = line[index];
				remainder_index++;
			}
		}
		else if (state == kBeyondToken)
		{
			remainder_buffer[remainder_index] = line[index];
			remainder_index++;
		}
	}

	/* Lowercase the token. */
	for (index = 0; index < token_index; index++)
	{
		token_buffer[index] = tolower(token_buffer[index]);
	}

	/* Terminate the strings. */
	token_buffer[token_index] = '\0';
	remainder_buffer[remainder_index] = '\0';

	token = ADT_XALLOCATE(token_index + 1);
	memcpy(token, token_buffer, token_index + 1);

	(*remainder) = ADT_XALLOCATE(remainder_index + 1);
	memcpy(*remainder, remainder_buffer, remainder_index + 1);

	return token;
}

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const uint8_t charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
	'\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
	'\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
	'\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};


int
adt_strcasecmp(const char *s1, const char *s2)
{
	const uint8_t *cm = charmap;
	const uint8_t *us1 = (const uint8_t *)s1;
	const uint8_t *us2 = (const uint8_t *)s2;

	while (cm[*us1] == cm[*us2++])
	{
		if (*us1++ == '\0')
		{
			return (0);
		}
	}

	return cm[*us1] - cm[*--us2];
}


int
adt_strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n != 0) 
	{
		const uint8_t *cm = charmap;
		const uint8_t *us1 = (const uint8_t *)s1;
		const uint8_t *us2 = (const uint8_t *)s2;

		do 
		{
			if (cm[*us1] != cm[*us2++])
			{
				return cm[*us1] - cm[*--us2];
			}

			if (*us1++ == '\0')
			{
				break;
			}

		} while (--n != 0);
	}

	return 0;
}


/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
adt_strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
	{
		d++;
	}

	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
	{
		return dlen + strlen(s);
	}

	while (*s != '\0') 
	{
		if (n != 1) 
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return dlen + (s - src);	/* count does not include NUL */
}


/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
adt_strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) 
	{
		do 
		{
			if ((*d++ = *s++) == 0)
			{
				break;
			}
			
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) 
	{
		if (siz != 0)
		{
			*d = '\0';		/* NUL-terminate dst */
		}
		
		while (*s++)
		{
			/* Do nothing. */
		}
	}

	return s - src - 1;	/* count does not include NUL */
}


int
adt_strtobool(const char* source)
{
	assert(source);
	
	if (source)
	{
		if (strcmp(source, "true") == 0)
		{
			return 1;
		}
		else if (strcmp(source, "false") == 0)
		{
			return 0;
		}
	}
	
	return -1;
}


int
adt_strtoi(int default_value, const char* source)
{
	int int_value;
	
	assert(source);
	
	if (source && (sscanf(source, "%d", &int_value) == 1))
	{
		return int_value;
	}
	
	return default_value;
}


unsigned int
adt_strtoui(unsigned int default_value, const char* source)
{
	unsigned int uint_value;
	
	assert(source);
	
	if (source && (sscanf(source, "%u", &uint_value) == 1))
	{
		return uint_value;
	}
	
	return default_value;
}


void
adt_lowercase_string(const char* str, char* buffer, unsigned int buflen)
{
	assert(buflen > 0);
	
	/* Ensure the buffer is terminated. */
	buffer[buflen - 1] = '\0';
	strncpy(buffer, str, buflen);
	
	while (*buffer)
	{
		*buffer = tolower(*buffer);
		buffer++;
	}
}


void
adt_uppercase_string(const char* str, char* buffer, unsigned int buflen)
{
	assert(buflen > 0);
	
	/* Ensure the buffer is terminated. */
	buffer[buflen - 1] = '\0';
	strncpy(buffer, str, buflen);
	
	while (*buffer)
	{
		*buffer = toupper(*buffer);
		buffer++;
	}
}


/*
 # preprocessing phase in constant time and space;
 # searching phase in O(nm) time complexity;
 # slightly (by coefficient) sub-linear in the average case.
 */
int
not_so_naive(const uint8_t *x, int m, const uint8_t *y, int n) 
{
	int j;
	int k;
	int ell;
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
  
	/* Preprocessing */
	if (x[0] == x[1]) 
	{
		k = 2;
		ell = 1;
	}
	else 
	{
		k = 1;
		ell = 2;
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uNotSoNaiveTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
  
	/* Searching */
	j = 0;
	while (j <= n - m)
	{
		if (x[1] != y[j + 1])
		{
			j += k;
		}
		else 
		{
			if ((memcmp(x + 2, y + j + 2, m - 2) == 0) && (x[0] == y[j]))
			{
#if ADT_STRING_SEARCH_PROFILING
				rdtscll(end_ticks);
				uNotSoNaiveTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

				return j; /* Originally OUTPUT(j); */
			}
			j += ell;
		}
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uNotSoNaiveTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
 # performs the comparisons from left to right;
 # preprocessing phase in O(m) space and time complexity;
 # searching phase in O(n+m) time complexity (independent from the alphabet size);
 # performs at most 2n-1 information gathered during the scan of the text;
 # delay bounded by m.
 */
int 
morris_pratt(const uint8_t *x, int m, const uint8_t *y, int n) 
{
   int i;
   int j;
   int mpNext[STRING_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

   /* Preprocessing */
   pre_morris_pratt(x, m, mpNext);

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uMorrisPrattTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

   /* Searching */
   i = j = 0;
   while (j < n) 
   {
      while (i > -1 && x[i] != y[j])
	  {
         i = mpNext[i];
      }

	  i++;
      j++;
      if (i >= m) 
	  {
#if ADT_STRING_SEARCH_PROFILING
		  rdtscll(end_ticks);
		  uMorrisPrattTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

		  return (j - i); /* Originally OUTPUT(j-i). */
         i = mpNext[i];
      }
   }

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uMorrisPrattTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

   return -1;
}


/*
 # performs the comparisons from left to right;
 # preprocessing phase in O(m) space and time complexity;
 # searching phase in O(n+m) time complexity (independent from the alphabet size);
 # delay bounded by log_phi(m) where phi is the golden ratio (1 + sqrt(5))/2.
 */
int
knuth_morris_pratt(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int i;
	int j;
	int kmpNext[STRING_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Preprocessing */
	pre_knuth_morris_pratt(x, m, kmpNext);

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uKnuthMorrisPrattTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Searching */
	i = 0;
	j = 0;
	while (j < n) 
	{
		while (i > -1 && x[i] != y[j])
		{
			i = kmpNext[i];
		}

		i++;
		j++;
		if (i >= m) 
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uKnuthMorrisPrattTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */
			
			return (j - i); /* Originally OUTPUT(j - i); */
			i = kmpNext[i];
		}
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uKnuthMorrisPrattTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # preprocessing phase in O(m) time and space complexity;
  # searching phase in O(n) time complexity;
  # performs 3n/2 text character comparisons in the worst case
*/
int
apostolico_crochemore(const uint8_t *x, int m, const uint8_t *y, int n) 
{
	int i; 
	int j; 
	int k; 
	int ell; 
	int kmpNext[STRING_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Preprocessing */
	pre_knuth_morris_pratt(x, m, kmpNext);

	ell = 1;
	while (x[ell - 1] == x[ell])
	{
		ell++;
	}

	if (ell == m)
	{
		ell = 0;
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uApostolicoCrochemoreTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Searching */
	i = ell;
	j = k = 0;
	while (j <= n - m) 
	{
		while ((i < m) && (x[i] == y[i + j]))
		{
			i++;
		}

		if (i >= m) 
		{
			while ((k < ell) && (x[k] == y[j + k]))
			{
				k++;
			}

			if (k >= ell)
			{
#if ADT_STRING_SEARCH_PROFILING
				rdtscll(end_ticks);
				uApostolicoCrochemoreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

				return j; /* Originally OUTPUT(j); */
			}
		}
		j += (i - kmpNext[i]);
		if (i == ell)
		{
			if (k > 1)
			{
				k--;
			}
			else
			{
				k = 0;
			}
		}
		else
		{
			if (kmpNext[i] <= ell) 
			{
				if (kmpNext[i] > 0)
				{
					k = kmpNext[i];
				}
				else
				{
					k = 0;
				}
				i = ell;
			}
			else 
			{
				k = ell;
				i = kmpNext[i];
			}
		}
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uApostolicoCrochemoreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # preprocessing phase in O(m+sigma)time and space complexity;
  # searching phase in O(mn)time complexity;
  # 3n text character comparisons in the worst case when searching for a non periodic pattern;
  # O(n / m)best performance.
*/
int
boyer_moore(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int i;
	int j;
	int first;
	int second;
	int bmGs[STRING_SIZE];
	int bmBc[ALPHABET_SIZE];
	int suff[STRING_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Preprocessing */
	pre_boyer_moore_good_suffix(x, m, bmGs, suff);
	pre_boyer_moore_bad_char(x, m, bmBc);
 
#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uBoyerMooreTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	/* Searching */
	j = 0;
	while (j <= n - m)
	{
		i = m - 1; 
		while (i >= 0 && x[i] == y[i + j])
		{
			--i;
		}

		if (i < 0)
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uBoyerMooreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			return j; /* Originally OUTPUT(j) */
			
			j += bmGs[0];
		}
		else
		{
			first = bmGs[i];
			second = bmBc[y[i + j]] - m + 1 + i;
			if (first > second)
			{
				j += first;
			}
			else
			{
				j += second;
			}
		}
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uBoyerMooreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
 # variant of the Boyer-Moore;
 # no extra preprocessing needed with respect to the Boyer-Moore algorithm;
 # constant extra space needed with respect to the Boyer-Moore algorithm;
 # preprocessing phase in O(m+) time and space complexity;
 # searching phase in O(n) time complexity;
 # 2n text character comparisons in the worst case.
 */
int
turbo_boyer_moore(const uint8_t *x, int m, const uint8_t *y, int n) 
{
	int bcShift;
	int i;
	int j;
	int shift;
	int u;
	int v;
	int turboShift;
	int bmGs[STRING_SIZE];
	int bmBc[ALPHABET_SIZE];
	int suff[STRING_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Preprocessing */
	pre_boyer_moore_good_suffix(x, m, bmGs, suff);
	pre_boyer_moore_bad_char(x, m, bmBc);

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uTurboBoyerMooreTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Searching */
	j = 0;
	u = 0;
	shift = m;
	while (j <= n - m) 
	{
		i = m - 1;
		while ((i >= 0) && (x[i] == y[i + j])) 
		{
			--i;
			if (u && (i == m - 1 - shift))
			{
				i -= u;
			}
		}
		if (i < 0) 
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uTurboBoyerMooreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			return j; /* Originally OUTPUT(j); */
			shift = bmGs[0];
			u = m - shift;
		}
		else 
		{
			v = m - 1 - i;
			turboShift = u - v;
			bcShift = bmBc[y[i + j]] - m + 1 + i;

			if (turboShift > bcShift)
			{
				shift = turboShift;
			}
			else
			{
				shift = bcShift;
			}

			if (bmGs[i] > shift)
			{
				shift = bmGs[i];
			}

			if (shift == bmGs[i])
			{
				if (m - shift < v)
				{
					u = m - shift;
				}
				else
				{
					u = v;
				}
			}
			else 
			{
				if (turboShift < bcShift)
				{
					if (u + 1 > shift)
					{
						shift = u + 1;
					}
				}
				u = 0;
			}
		}
		j += shift;
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uTurboBoyerMooreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
 # simplification of the Boyer-Moore algorithm;
 # easy to implement;
 # very fast in practice.
 */
int
tuned_boyer_moore(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int j;
	int k;
	int shift;
	int bmBc[ALPHABET_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	/* Preprocessing */
	pre_boyer_moore_bad_char(x, m, bmBc);
	shift = bmBc[x[m - 1]];
	bmBc[x[m - 1]] = 0;
	memset((uint8_t*) (y + n), x[m - 1], m);
 
#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uTunedBoyerMooreTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	/* Searching */
	j = 0;
	while (j < n)
	{
		k = bmBc[y[j + m -1]];
      
		while (k != 0)
		{
			j += k; 
			k = bmBc[y[j + m -1]];
         
			j += k; 
			k = bmBc[y[j + m -1]];
         
			j += k; 
			k = bmBc[y[j + m -1]];
		}
      
		if (memcmp(x, y + j, m - 1)== 0 && j < n)
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uTunedBoyerMooreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			return j; /* Originally OUTPUT(j)*/
		}
		j += shift;    /* shift */
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uTunedBoyerMooreTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # variant of the Boyer-Moore algorithm;
  # preprocessing phase in O(m+sigma)time and space complexity;
  # searching phase in O(n)time complexity;
  # 3n/2 comparisons in the worst case;
*/
int
apostolico_giancarlo(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int i;
	int j;
	int k;
	int s;
	int shift;
	int first;
	int second;
	int bmGs[STRING_SIZE];
	int skip[STRING_SIZE]; 
	int suff[STRING_SIZE];
	int bmBc[ALPHABET_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
  
	/* Preprocessing */
	pre_boyer_moore_good_suffix(x, m, bmGs, suff);
	pre_boyer_moore_bad_char(x, m, bmBc);
	memset(skip, 0, m*sizeof(int));
 
#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uApostolicoGiancarloTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
  
	/* Searching */
	j = 0;
	while (j <= n - m)
	{
		i = m - 1;
		while (i >= 0)
		{
			k = skip[i];
			s = suff[i];
			if (k > 0)
			{
				if (k > s)
				{
					if (i + 1 == s)
					{
						i = (-1);
					}
					else
					{
						i -= s;
					}
					break;
				}
				else 
				{
					i -= k;
					if (k < s)
					{
						break;
					}
				}
			}
			else 
			{
				if (x[i] == y[i + j])
				{
					--i;
				}
				else
				{
					break;
				}
			}
		}
	   
		if (i < 0)
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uApostolicoGiancarloTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			return j;

			/* Unreachable code due to original algorithm using a callback for matched positions:
			 * OUTPUT(j);
			 */
			skip[m - 1] = m;
			shift = bmGs[0];
		}
		else
		{
			skip[m - 1] = m - 1 - i;
			first = bmGs[i];
			second = bmBc[y[i + j]] - m + 1 + i;
			if (first > second)
			{
				shift = first;
			}
			else
			{
				shift = second;
			}
		}
		j += shift;
		memcpy(skip, skip + shift, (m - shift)*sizeof(int));
		memset(skip + m - shift, 0, shift*sizeof(int));
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uApostolicoGiancarloTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # preprocessing phase in O(m+sigma) time and O(sigma)space complexity;
  # searching phase in O(mn)time complexity;
  # very fast in practice for short patterns and large alphabets.
*/
int 
quick_search(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int j;
	int qsBc[ALPHABET_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Preprocessing. */
	pre_quick_search_bad_char(x, m, qsBc);
 
#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uQuickSearchTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	/* Searching. */
	j = 0;
	while (j <= n - m)
	{
		if (memcmp(x, y + j, m)== 0)
		{   
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uQuickSearchTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */
		   
			return j; /* Originally OUTPUT(j)*/
		}
		j += qsBc[y[j + m]];  /* shift */
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uQuickSearchTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # preprocessing phase in O(m+sigma)time and O(sigma)space complexity;
  # searching phase in O(mn)time complexity.
*/
int
smith(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int j;
	int first;
	int second;
	int bmBc[ALPHABET_SIZE];
	int qsBc[ALPHABET_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Preprocessing */
	pre_boyer_moore_bad_char(x, m, bmBc);
	pre_quick_search_bad_char(x, m, qsBc);
 
#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uSmithTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Searching */
	j = 0;
	while (j <= n - m)
	{
		if (memcmp(x, y + j, m)== 0)
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uSmithTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			return j; /* Originally OUTPUT(j)*/
		}

		first = bmBc[y[j + m - 1]];
		second = qsBc[y[j + m]];
		if (first > second)
		{
			j += first;
		}
		else
		{
			j += second;
		}
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uSmithTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # refinement of the Knuth, Morris and Pratt algorithm;
  # partitions the set of pattern positions into two disjoint subsets; the positions in the first set are scanned from left to right and when no mismatch occurs the positions of the second subset are scanned from right to left;
  # preprocessing phase in O(m)time and space complexity;
  # searching phase in O(n)time complexity;
  # performs n text character comparisons in the worst case.
*/
int
colussi(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int i;
	int j;
	int last;
	int nd;
	int h[STRING_SIZE];
	int next[STRING_SIZE];
	int shift[STRING_SIZE];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Processing */
	nd = pre_colussi(x, m, h, next, shift);
 
#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uColussiTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

	/* Searching */
	i = j = 0;
	last = -1;
	while (j <= n - m)
	{
		while (i < m && last < j + h[i] && x[h[i]] == y[j + h[i]])
		{
			i++;
		}
	  
		if (i >= m || last >= j + h[i])
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uColussiTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			return j;			

            /* Unreachable code due to original algorithm using a callback for matched positions:
			 * OUTPUT(j);
			 */
			i = m;
		}

		if (i > nd)
		{
			last = j + m - 1;
		}
		j += shift[i];
		i = next[i];
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uColussiTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


/*
  # preprocessing phase in O(m)time and space complexity;
  # searching phase in O(n)time complexity;
  # performs 4n/3 text character comparisons in the worst case.
*/
int
galil_giancarlo(const uint8_t *x, int m, const uint8_t *y, int n)
{
	int i;
	int j;
	int k;
	int ell;
	int last;
	int nd;
	int h[STRING_SIZE];
	int next[STRING_SIZE];
	int shift[STRING_SIZE];
	uint8_t heavy;
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	ell = 0; 
	while (x[ell] == x[ell + 1])
	{
		ell++;
	}

	if (ell == m - 1)
	{
		/* Searching for a power of a single character */
		ell = 0;
		for (j = 0; j < n; ++j)
		{
			if (x[0] == y[j])
			{
				++ell;
				if (ell >= m)
				{
#if ADT_STRING_SEARCH_PROFILING
					rdtscll(end_ticks);
					uGalilGiancarloTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

					return j - m + 1; /* Originally OUTPUT(j)*/
				}
			}
			else
			{
				ell = 0;
			}
		}
	}
	else 
	{
		/* Preprocessing */
		nd = pre_colussi(x, m, h, next, shift);
 
#if ADT_STRING_SEARCH_PROFILING
		rdtscll(end_ticks);
		uColussiTime[0].mTicks += end_ticks - start_ticks;
		rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */

		/* Searching */
		i = 0;
		j = 0;
		heavy = 0;
		last = -1;
		while (j <= n - m)
		{
			if (heavy && (i == 0))
			{
				k = last - j + 1;
				while (x[0] == y[j + k])
				{
					k++;
				}
				
				if ((k <= ell) || (x[ell + 1] != y[j + k]))
				{
					i = 0;
					j += (k + 1);
					last = j - 1;
				}
				else 
				{
					i = 1;
					last = j + k;
					j = last - (ell + 1);
				}
				heavy = 0;
			}
			else 
			{
				while ((i < m) && (last < j + h[i]) && (x[h[i]] == y[j + h[i]]))
				{
					++i;
				}

				if ((i >= m) || (last >= j + h[i]))
				{
#if ADT_STRING_SEARCH_PROFILING
					rdtscll(end_ticks);
					uGalilGiancarloTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

					return j; /* Originally OUTPUT(j) */
					i = m;
				}

				if (i > nd)
				{
					last = j + m - 1;
				}
				j += shift[i];
				i = next[i];
			}
			heavy = (j > last ? 0 : 1);
		}
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uGalilGiancarloTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	return -1;
}


static void 
buildSuffixAutomaton(const uint8_t *x, int m, Graph aut) 
{
	int i; 
	int art; 
	int init; 
	int last; 
	int p; 
	int q;
	int r;
	char c;
  
	init = getInitial(aut);
	art = newVertex(aut);
	setSuffixLink(aut, init, art);
	last = init;
	for (i = 0; i < m; ++i) 
	{
		c = x[i];
		p = last;
		q = newVertex(aut);
		setLength(aut, q, getLength(aut, p) + 1);
		setPosition(aut, q, getPosition(aut, p) + 1);
		while ((p != init) && (getTarget(aut, p, c) == UNDEFINED))
		{
			setTarget(aut, p, c, q);
			setShift(aut, p, c, getPosition(aut, q) - getPosition(aut, p) - 1);
			p = getSuffixLink(aut, p);
		}
		if (getTarget(aut, p, c) == UNDEFINED) 
		{
			setTarget(aut, init, c, q);
			setShift(aut, init, c, getPosition(aut, q) - getPosition(aut, init) - 1);
			setSuffixLink(aut, q, init);
		}
		else
		{
			if ((getLength(aut, p) + 1) == getLength(aut, getTarget(aut, p, c)))
			{
				setSuffixLink(aut, q, getTarget(aut, p, c));
			}
			else 
			{
				r = newVertex(aut);
				copyVertex(aut, r, getTarget(aut, p, c));
				setLength(aut, r, getLength(aut, p) + 1);
				setSuffixLink(aut, getTarget(aut, p, c), r);
				setSuffixLink(aut, q, r);
				while ((p != art) && (getLength(aut, getTarget(aut, p, c)) >= getLength(aut, r))) 
				{
					setShift(aut, p, c, getPosition(aut, getTarget(aut, p, c)) - getPosition(aut, p) - 1);
					setTarget(aut, p, c, r);
					p = getSuffixLink(aut, p);
				}
			}
		}
		last = q;
	}

	setTerminal(aut, last);
	while (last != init) 
	{
		last = getSuffixLink(aut, last);
		setTerminal(aut, last);
	}
}


static inline const uint8_t*
reverse(const uint8_t *x, int m) 
{
	int i;
	uint8_t *xR = (uint8_t*) ADT_XALLOCATE((m + 1)*sizeof(uint8_t));
 
	for (i = 0; i < m; ++i)
	{
		xR[i] = x[m - 1 - i];
	}
	xR[m] = '\0';

	return xR;
}
 

/* 
 # uses the suffix automaton of x^R;
 # fast on practice for long patterns and small alphabets;
 # preprocessing phase in O(m) time and space complexity;
 # searching phase in O(mn) time complexity;
 # optimal in the average.
 */
int 
reverse_factor(const uint8_t *x, int m, const uint8_t *y, int n) 
{
	int i; 
	int j;
	int shift; 
	int period; 
	int init; 
	int state;
	Graph aut;
	const uint8_t *xR;
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	/* Preprocessing */
	aut = newSuffixAutomaton(2 * (m + 2), 2 * (m + 2) * ALPHABET_SIZE);
	xR = reverse(x, m);
	buildSuffixAutomaton(xR, m, aut);
	init = getInitial(aut);
	period = m;

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uReverseFactorTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
 
	/* Searching */
	j = 0;
	while (j <= n - m) 
	{
		i = m - 1;
		state = init;
		shift = m;

		while ((i + j >= 0) && (getTarget(aut, state, y[i + j]) != UNDEFINED)) 
		{
			state = getTarget(aut, state, y[i + j]);
			if (isTerminal(aut, state)) 
			{
				period = shift;
				shift = i;
			}
			--i;
		}

		if (i < 0) 
		{
#if ADT_STRING_SEARCH_PROFILING
			rdtscll(end_ticks);
			uReverseFactorTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

			adt_dispose_ptr((char*) xR);
			disposeGraph(aut);
			return j; /* Originally OUTPUT(j), hence the unreachable code below. */

			shift = period;
		}
		j += shift;
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uReverseFactorTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	adt_dispose_ptr((char*) xR);
	disposeGraph(aut);
	return -1;
}


/*
 # refinement of the Reverse Factor algorithm;
 # preprocessing phase in O(m) time and space complexity;
 # searching phase in O(n) time complexity;
 # performs 2n text characters inspections in the worst case;
 # optimal in the average.
 */
int 
turbo_reverse_factor(const uint8_t *x, int m, const uint8_t *y, int n) 
{
	int period; 
	int i; 
	int j; 
	int shift; 
	int u; 
	int periodOfU; 
	int disp; 
	int init;
	int state; 
	int mu; 
	const uint8_t *xR;
	Graph aut;
	int mpNext[STRING_SIZE + 1];
#if ADT_STRING_SEARCH_PROFILING
	long long start_ticks;
	long long end_ticks = 0;

	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
  
	/* Preprocessing */
	aut = newSuffixAutomaton(2*(m + 2), 2 * (m + 2) * ALPHABET_SIZE);
	xR = reverse(x, m);
	buildSuffixAutomaton(xR, m, aut);
	init = getInitial(aut);
	pre_morris_pratt(x, m, mpNext);
	period = m - mpNext[m];
	i = 0;
	shift = m;

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uTurboReverseFactorTime[0].mTicks += end_ticks - start_ticks;
	rdtscll(start_ticks);
#endif /* ADT_STRING_SEARCH_PROFILING */
  
	/* Searching */
	j = 0;
	while (j <= n - m) 
	{
		i = m - 1;
		state = init;
		u = m - 1 - shift;
		if (shift != m)
		{
			periodOfU = m - shift - mpNext[m - shift];
		}
		else
		{
			periodOfU = 0;
		}
		shift = m;
		disp = 0;

		while ((i > u) && (getTarget(aut, state, y[i + j]) != UNDEFINED))
		{
			disp += getShift(aut, state, y[i + j]);
			state = getTarget(aut, state, y[i + j]);
			if (isTerminal(aut, state))
			{
				shift = i;
			}
			--i;
		}
		if (i <= u)
		{
			if (disp == 0) 
			{
#if ADT_STRING_SEARCH_PROFILING
				rdtscll(end_ticks);
				uTurboReverseFactorTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

				adt_dispose_ptr((char*) xR);
				disposeGraph(aut);
				return j;
				shift = period;
			}
			else 
			{
				mu = (u + 1) / 2;
				if (periodOfU <= mu) 
				{
					u -= periodOfU;
					while ((i > u) && (getTarget(aut, state, y[i + j]) != UNDEFINED)) 
					{
						disp += getShift(aut, state, y[i + j]);
						state = getTarget(aut, state, y[i + j]);
						if (isTerminal(aut, state))
						{
							shift = i;
						}
						--i;
					}
					if (i <= u)
					{
						shift = disp;
					}
				}
				else 
				{
					u = u - mu - 1;
					while ((i > u) && (getTarget(aut, state, y[i + j]) != UNDEFINED)) 
					{
						disp += getShift(aut, state, y[i + j]);
						state = getTarget(aut, state, y[i + j]);
						if (isTerminal(aut, state))
						{
							shift = i;
						}
						--i;
					}
				}
			}
		}
		j += shift;
	}

#if ADT_STRING_SEARCH_PROFILING
	rdtscll(end_ticks);
	uTurboReverseFactorTime[1].mTicks += end_ticks - start_ticks;
#endif /* ADT_STRING_SEARCH_PROFILING */

	adt_dispose_ptr((char*) xR);
	disposeGraph(aut);
	return -1;
}
