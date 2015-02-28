/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagclarg.c,v 1.16.2.3 2009/09/08 02:09:47 jomi.gregory Exp $
 */

/* File header. */
#include "dagclarg.h"


#define CLARG_MAGIC_NUMBER 0x19283746
#define TEMPLATE_BUFFER_BYTES 256
#define ALIAS_COUNT 4



typedef enum
{
	CLARG_NONE,
	CLARG_CHAR,
	CLARG_INT,
	CLARG_UINT,
	CLARG_STRING,
	CLARG_INT64

} clarg_kind_t;


typedef struct clarg_opt_t
{
	const char* description[ALIAS_COUNT]; /* description for the usage text */
	const char* argname;                      /* displayed argument name in usage text */
	const char* longopt[ALIAS_COUNT];         /* name of long option */
	char shortopt[ALIAS_COUNT];              /* equivalent short option */

	void* buffer;      /* Stores argument value when option found. */
	clarg_kind_t kind;
	uint16_t buflen;   /* Buffer size. */
	uint16_t code;     /* Used to set return code in dagclarg_parse(). */
	uint16_t description_count;
	uint16_t shortopt_count;
	uint16_t longopt_count;
	int display;       /* 1 => display this in usage, 0 => don't display in usage. */
	struct clarg_opt_t* next;

} clarg_opt_t;


typedef struct ClArgHeader
{
	uint32_t mMagicNumber;
	clarg_opt_t* mHead;
	int mArgc;
	const char* const * mArgv;
	int mUnprocessedArgc;
	char** mUnprocessedArgv;
	int mCurrentArg;
	int mCurrentPos; /* Position within the arg for the -rpvy type concatenated flags. */
	uint16_t mLongestOption;

} ClArgHeader, *ClArgHeaderPtr;


/* Internal routines. */
#ifndef _WIN32
#define INLINE inline
#endif /* _WIN32 */



static INLINE unsigned int valid_clarg_header(ClArgHeaderPtr header);
#ifndef NDEBUG
static void verify_header(ClArgHeaderPtr header);
#endif /* NDEBUG */
static clarg_opt_t* find_clarg(ClArgHeaderPtr header, uint16_t code);
static void clarg_add_common(ClArgHeaderPtr header, const char* description, const char* longopt, const char shortopt, const char* argname, void* buffer, uint16_t buflen, uint16_t code, clarg_kind_t kind);
static int clarg_get_argument(clarg_opt_t* option, FILE* errorfile, const char* current, const char* argument);
static unsigned int clarg_match_longopt(clarg_opt_t* option, const char* argument, int* option_length);
static unsigned int clarg_match_shortopt(clarg_opt_t* option, const char argument);
static void clarg_add_shortopt(clarg_opt_t* option, const char shortopt);
static void clarg_add_longopt(clarg_opt_t* option, const char* longopt);
static void clarg_add_description(clarg_opt_t* option, const char* line);
static void clarg_suppress_description(clarg_opt_t* option);


/* Implementation of internal routines. */
static INLINE unsigned int
valid_clarg_header(ClArgHeaderPtr header)
{
	if (header && (header->mMagicNumber == CLARG_MAGIC_NUMBER))
	{
		return 1;
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_header(ClArgHeaderPtr header)
{
	assert(NULL != header);
	assert(header->mMagicNumber == CLARG_MAGIC_NUMBER);
}
#endif /* NDEBUG */


static clarg_opt_t*
find_clarg(ClArgHeaderPtr header, uint16_t code)
{
	clarg_opt_t* option = header->mHead;

	while (NULL != option)
	{
		if (option->code == code)
		{
			return option;
		}
		
		option = option->next;
	}
	
	return NULL;
}


static void
clarg_add_common(ClArgHeaderPtr header, const char* description, const char* longopt, const char shortopt, const char* argname, void* buffer, uint16_t buflen, uint16_t code, clarg_kind_t kind)
{
	/* Create new clarg_opt_t. */
	clarg_opt_t* new_clarg = (clarg_opt_t*) malloc(sizeof(*new_clarg));
	memset(new_clarg, 0, sizeof(*new_clarg));
	
	/* Populate new clarg_opt_t with routine parameters. */
	clarg_add_description(new_clarg, description);
	clarg_add_longopt(new_clarg, longopt);
	clarg_add_shortopt(new_clarg, shortopt);
	new_clarg->argname = argname;
	new_clarg->kind = kind;
	new_clarg->buffer = buffer;
	new_clarg->buflen = buflen;
	new_clarg->code = code;
	new_clarg->next = NULL;
	new_clarg->display = 1;

	/* Set the length of the longest option. */
	if ((NULL != longopt) || (NULL != argname))
	{
		int option_length = 0;
		
		if (NULL != longopt)
		{
			option_length += (int)strlen(longopt);
		}
		
		if (NULL != argname)
		{
			option_length += 4 + (int)strlen(argname);
		}
		
		if (option_length > header->mLongestOption)
		{
			header->mLongestOption = option_length;
		}
	}

	/* Add to end of list, preserving user order. */
	if (NULL == header->mHead)
	{
		header->mHead = new_clarg;
	}
	else
	{
		clarg_opt_t* temp_clarg = header->mHead;
		
		while (NULL != temp_clarg->next)
		{
			temp_clarg = temp_clarg->next;
		}
		temp_clarg->next = new_clarg;
	}
}


static int
clarg_get_argument(clarg_opt_t* option, FILE* errorfile, const char* current, const char* argument)
{
	assert(NULL != option);
	assert(NULL != argument);

	if (CLARG_CHAR == option->kind)
	{
		*((uint8_t*) option->buffer) = argument[0];
	}
	else if (CLARG_INT == option->kind)
	{
		if (1 != sscanf(argument, "%i", (int32_t*) option->buffer))
		{
			if (errorfile)
			{
				fprintf(errorfile, "error: option '%s' requires an integer argument\n", current);
			}
			return -1;
		}
	}
	else if (CLARG_UINT == option->kind)
	{
		if (1 != sscanf(argument, "%u", (uint32_t*) option->buffer))
		{
			if (errorfile)
			{
				fprintf(errorfile, "error: option '%s' requires an unsigned integer argument\n", current);
			}
			return -1;
		}
	}
	else if (CLARG_INT64 == option->kind)
	{
		if (1 != sscanf(argument, "%"SCNi64, (int64_t*) option->buffer))
		{
			if (errorfile)
			{
				fprintf(errorfile, "error: option '%s' requires a 64-bit integer argument\n", current);
			}
			return -1;
		}
	}
	else if (CLARG_STRING == option->kind)
	{
		strncpy((char*) option->buffer, argument, option->buflen);
	}
	else
	{
		assert(0); /* Internal error: kind wasn't set correctly. */
	}
	
	return 1;
}


static unsigned int
clarg_match_longopt(clarg_opt_t* option, const char* argument, int* option_length)
{
	int index;
	
	for (index = 0; index < option->longopt_count; index++)
	{
		int optlen = (int)strlen(option->longopt[index]);
		
		if ((NULL != option->longopt[index]) && (0 != optlen) && (0 == strncmp(option->longopt[index], argument, optlen)))
		{
			*option_length = optlen;
			return 1;
		}
	}
	
	return 0;
}


static unsigned int
clarg_match_shortopt(clarg_opt_t* option, const char argument)
{
	int index;
	
	for (index = 0; index < option->shortopt_count; index++)
	{
		if ((0 != option->shortopt[index]) && (argument == option->shortopt[index]))
		{
			return 1;
		}
	}
	
	return 0;
}


static void
clarg_add_shortopt(clarg_opt_t* option, const char shortopt)
{
	assert(NULL != option);

	if ('\0' == shortopt)
	{
		return;
	}
	
	if (option->shortopt_count == ALIAS_COUNT)
	{
		return;
	}
	
	option->shortopt[option->shortopt_count] = shortopt;
	option->shortopt_count++;
}


static void
clarg_add_longopt(clarg_opt_t* option, const char* longopt)
{
	assert(NULL != option);
	assert(NULL != longopt);

	if (NULL == longopt)
	{
		return;
	}
	
	if (option->longopt_count == ALIAS_COUNT)
	{
		return;
	}
	
	option->longopt[option->longopt_count] = longopt;
	option->longopt_count++;
}


static void
clarg_add_description(clarg_opt_t* option, const char* description)
{
	assert(NULL != option);

	if (option->description_count == ALIAS_COUNT)
	{
		return;
	}

	if (NULL != description)
	{
		option->description[option->description_count] = description;
	}
	else
	{
		option->description[option->description_count] = "(no description available)";
	}
	
	option->description_count++;
}


static void
clarg_suppress_description(clarg_opt_t* option)
{
	assert(NULL != option);
	
	option->display = 0;
}


ClArgPtr
dagclarg_init(int argc, const char* const * argv)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) malloc(sizeof(*header));

	memset(header, 0, sizeof(*header));
	
	header->mArgc = argc;
	header->mArgv = argv;
	header->mCurrentArg = 1;
	header->mCurrentPos = 0;
	header->mLongestOption = 8;

	header->mUnprocessedArgv = (char**) malloc(argc * sizeof(char*));
	memset(header->mUnprocessedArgv, 0, argc * sizeof(char*));

	header->mMagicNumber = CLARG_MAGIC_NUMBER;

	return (ClArgPtr) header;
}


ClArgPtr
dagclarg_dispose(ClArgPtr clarg)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		header->mMagicNumber = 0;
		
		while (header->mHead)
		{
			clarg_opt_t* temp_clarg = header->mHead;
			
			header->mHead = temp_clarg->next;
			free(temp_clarg);
		}
		
		if (NULL != header->mUnprocessedArgv)
		{
			int index;

			for (index = 0; index < header->mArgc; index++)
			{
				if (NULL != header->mUnprocessedArgv[index])
				{
					free(header->mUnprocessedArgv[index]);
				}
			}
			free(header->mUnprocessedArgv);
		}

		free(header);
	}

	return (ClArgPtr) NULL;
}


void
dagclarg_add(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_add_common(header, description, longopt, shortopt, NULL, NULL, 0, code, CLARG_NONE);
	}
}


void
dagclarg_add_char(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, uint8_t* storage, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_add_common(header, description, longopt, shortopt, argname, storage, sizeof(uint8_t), code, CLARG_CHAR);
	}
}


void
dagclarg_add_int(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, int32_t* storage, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_add_common(header, description, longopt, shortopt, argname, storage, sizeof(int32_t), code, CLARG_INT);
	}
}


void
dagclarg_add_uint(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, uint32_t* storage, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_add_common(header, description, longopt, shortopt, argname, storage, sizeof(uint32_t), code, CLARG_UINT);
	}
}


void
dagclarg_add_int64(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, int64_t* storage, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_add_common(header, description, longopt, shortopt, argname, storage, sizeof(int64_t), code, CLARG_INT64);
	}
}


void
dagclarg_add_string(ClArgPtr clarg, const char* description, const char* longopt, const char shortopt, const char* argname, char* buffer, uint32_t buflen, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_add_common(header, description, longopt, shortopt, argname, buffer, (uint16_t)buflen, code, CLARG_STRING);
	}
}


void
dagclarg_add_short_option(ClArgPtr clarg, uint16_t code, const char shortopt)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_opt_t* option = find_clarg(header, code);

		if (NULL != option)
		{
			clarg_add_shortopt(option, shortopt);
		}
	}
}


void
dagclarg_add_long_option(ClArgPtr clarg, uint16_t code, const char* longopt)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_opt_t* option = find_clarg(header, code);

		if (NULL != option)
		{
			clarg_add_longopt(option, longopt);
		}
	}
}


void
dagclarg_add_description(ClArgPtr clarg, uint16_t code, const char* line)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_opt_t* option = find_clarg(header, code);

		if (NULL != option)
		{
			clarg_add_description(option, line);
		}
	}
}


void
dagclarg_suppress_display(ClArgPtr clarg, uint16_t code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;

#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_opt_t* option = find_clarg(header, code);

		if (NULL != option)
		{
			clarg_suppress_description(option);
		}
	}
}


int
dagclarg_parse(ClArgPtr clarg, FILE* errorfile, int* argindex, int* code)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;
	clarg_opt_t* option = NULL;
	const char* current;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (0 == valid_clarg_header(header))
	{
		return 0;
	}
	
	*argindex = header->mCurrentArg;
	if (header->mCurrentArg >= header->mArgc)
	{
		/* No more options. */
		return 0;
	}
	
	current = header->mArgv[header->mCurrentArg];
	if (current[0] == '-')
	{
		int current_length = (int)strlen(current);
		
		if (current[1] == '-')
		{
            clarg_opt_t* best_matched_long_option = NULL;
            int best_matched_option_length = 0;
			/* Long option. */
			header->mCurrentPos = 0;

			if (2 == current_length)
			{
				/* -- indicates end of options (GNU convention). */
				header->mCurrentArg++;
				return 0;
			}
			
			option = header->mHead;
			
			while (NULL != option)
			{
				if (NULL != option->longopt[0])
				{
					int option_length;
					if (1 == clarg_match_longopt(option, current, &option_length))
					{
						/* check whether the new match is better than prvious match*/
                        if ( option_length > best_matched_option_length)
                        {
                            /* we got a better match */
                            best_matched_option_length = option_length;
                            best_matched_long_option = option;
                        }
					}
				}
				option = option->next;
			}
            /* out of the while loop.all the long options are compared. */
            /* best_matched_option will have the match, if no match found NULL */
            option = best_matched_long_option;
            if (( NULL != option) &&  (current_length > best_matched_option_length) )
            {
                /* Potentially a long option with a contained argument, e.g. "--int 29". */
                const char* argument = &current[best_matched_option_length];
                int result;

                result = clarg_get_argument(option, NULL, current, argument);
                if (1 == result)
                {
                    /* Found a concatenated argument.  Otherwise we'll check the next argv entry. */
                    *code = option->code;
                    header->mCurrentArg++;
                    return 1;
                }
            }
		}
		else if (2 == current_length)
		{
			/* Exact short option. */
			header->mCurrentPos = 0;

			option = header->mHead;
			while (NULL != option)
			{
				if (1 == clarg_match_shortopt(option, current[1]))
				{
					break;
				}
				option = option->next;
			}
		}
		else if (2 < current_length)
		{
			/* Either a short option with concatenated argument, e.g. -i9. 
			 * or a bunch of short options run together, e.g. -rpvy.
			 */
			int index;

			if (0 == header->mCurrentPos)
			{
				header->mCurrentPos = 1;
			}

			for (index = header->mCurrentPos; index < current_length; index++)
			{
				char option_char = current[index];

				option = header->mHead;
				while (NULL != option)
				{
					if (1 == clarg_match_shortopt(option, option_char))
					{
						if (NULL != option->buffer)
						{
							/* Option requires an argument. */
							header->mCurrentPos = 0;

							if (current_length > index + 1)
							{
								const char* argument = &current[index + 1];
								int result = clarg_get_argument(option, NULL, current, argument);

								if (1 == result)
								{
									/* Found a concatenated argument.  Otherwise we'll check the next argv entry. */
									*code = option->code;
									header->mCurrentArg++;
									return 1;
								}
							}
						}
						else
						{
							/* Option doesn't require an argument. */
							*code = option->code;
							if (index + 1 == current_length)
							{
								header->mCurrentPos = 0;
								header->mCurrentArg++;
							}
							else
							{
								header->mCurrentPos = index + 1;
							}
							return 1;
						}
						break;
					}
					option = option->next;
				}

				if (NULL == option)
				{
					/* Option was unrecognised, should stop trying to parse this argument. */
					*code = -1;
					header->mCurrentPos = 0;
					header->mCurrentArg++;
					return 1;
				}
			}
		}
	}
	
	if (option)
	{
		*code = option->code;
		if ((NULL != option->buffer) || (CLARG_NONE != option->kind))
		{
			const char* argument;
			
			/* Option expects an argument... */
			header->mCurrentArg++;
			if (header->mArgc <= header->mCurrentArg)
			{
				/* ...but there wasn't one. */
				if (NULL != errorfile)
				{
					fprintf(errorfile, "error: option '%s' requires an argument\n", current);
				}
				return -1;
			}
			
			argument = header->mArgv[header->mCurrentArg];
			if (-1 == clarg_get_argument(option, errorfile, current, argument))
			{
				return -1;
			}
		}
	}
	else
	{
		if (NULL != errorfile)
		{
			if(3 == dagutil_get_verbosity())
				fprintf(errorfile, "error: unrecognised option '%s'\n", current);
		}
		*code = -1;

		/* Add the option to the unprocessed argument list. */
		header->mUnprocessedArgv[header->mUnprocessedArgc] = strdup(current);
		header->mUnprocessedArgc++;

		header->mCurrentPos = 0;
	}
	
	header->mCurrentArg++;
	return 1;
}


char**
dagclarg_get_unprocessed_args(ClArgPtr clarg, int* unprocessed_argc)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		*unprocessed_argc = header->mUnprocessedArgc;
		return header->mUnprocessedArgv;
	}
	
	*unprocessed_argc = 0;
	return NULL;
}


void
dagclarg_display_usage(ClArgPtr clarg, FILE* outfile)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;
	
	assert(outfile);
	
#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */

	if (valid_clarg_header(header))
	{
		clarg_opt_t* current_clarg = header->mHead;
		char template[TEMPLATE_BUFFER_BYTES];
		
		/* Create the template. */
		snprintf(template, TEMPLATE_BUFFER_BYTES, "    %%-%us  %%s\n", 4 + header->mLongestOption);
		
		fprintf(outfile, "Options:\n");
		while (NULL != current_clarg)
		{
			int index;
			char buffer[TEMPLATE_BUFFER_BYTES];
			
			if (0 == current_clarg->display)
			{
				/* Permits parsing and action for options that are invisible to the user. */
				current_clarg = current_clarg->next;
				continue;
			}
			
			/* Put extra short and long options first. */
			for (index = ALIAS_COUNT - 1; index > 0; index--)
			{
				buffer[0] = '\0';
				
				if ((NULL != current_clarg->longopt[index]) && ('\0' != current_clarg->shortopt[index]))
				{
					/* Both long and short options. */
					if (NULL != current_clarg->argname)
					{
						snprintf(buffer, TEMPLATE_BUFFER_BYTES, "-%c,%s <%s>", current_clarg->shortopt[index], current_clarg->longopt[index], current_clarg->argname);
					}
					else
					{
						snprintf(buffer, TEMPLATE_BUFFER_BYTES, "-%c,%s", current_clarg->shortopt[index], current_clarg->longopt[index]);
					}
				}
				else if (NULL != current_clarg->longopt[index])
				{
					/* Only a long option. */
					if (current_clarg->argname)
					{
						snprintf(buffer, TEMPLATE_BUFFER_BYTES, "%s <%s>", current_clarg->longopt[index], current_clarg->argname);
					}
					else
					{
						strncpy(buffer, current_clarg->longopt[index], TEMPLATE_BUFFER_BYTES);
					}
				}
				else if ('\0' != current_clarg->shortopt[index])
				{
					/* Only a short option. */
					buffer[0] = '-';
					buffer[1] = current_clarg->shortopt[index];
					buffer[2] = '\0';
				}
				else
				{
					/* Print nothing. */
				}
	
				if (0 != strlen(buffer))
				{
					/* Print the option. */
					fprintf(outfile, template, buffer, "");
				}
			}
			
			/* Then the first short and long options plus a description. */
			if ((NULL != current_clarg->longopt[0]) && ('\0' != current_clarg->shortopt[0]))
			{
				/* Both long and short options. */
				if (current_clarg->argname)
				{
					snprintf(buffer, TEMPLATE_BUFFER_BYTES, "-%c,%s <%s>", current_clarg->shortopt[0], current_clarg->longopt[0], current_clarg->argname);
				}
				else
				{
					snprintf(buffer, TEMPLATE_BUFFER_BYTES, "-%c,%s", current_clarg->shortopt[0], current_clarg->longopt[0]);
				}
			}
			else if (NULL != current_clarg->longopt[0])
			{
				/* Only a long option. */
				if (current_clarg->argname)
				{
					snprintf(buffer, TEMPLATE_BUFFER_BYTES, "%s <%s>", current_clarg->longopt[0], current_clarg->argname);
				}
				else
				{
					strncpy(buffer, current_clarg->longopt[0], TEMPLATE_BUFFER_BYTES);
				}
			}
			else if ('\0' != current_clarg->shortopt[0])
			{
				/* Only a short option. */
				buffer[0] = '-';
				buffer[1] = current_clarg->shortopt[0];
				buffer[2] = '\0';
			}
			else
			{
				/* Don't print any options. */
				buffer[0] = '\0';
			}
			fprintf(outfile, template, buffer, current_clarg->description[0]);
			
			/* Finally add extra description lines. */
			for (index = 1; index < current_clarg->description_count; index++)
			{
				fprintf(outfile, template, "", current_clarg->description[index]);
			}
			
			current_clarg = current_clarg->next;
		}
	}
}

void
dagclarg_reset(ClArgPtr clarg)
{
	ClArgHeaderPtr header = (ClArgHeaderPtr) clarg;
#ifndef NDEBUG
	verify_header(header);
#endif /* NDEBUG */
	
	if (valid_clarg_header(header))
	{
		header->mCurrentArg = 1;
		header->mCurrentPos = 0;
		if (NULL != header->mUnprocessedArgv)
		{
			int index;

			for (index = 0; index < header->mArgc; index++)
			{
				if (NULL != header->mUnprocessedArgv[index])
				{
					free(header->mUnprocessedArgv[index]);
				}
			}
			free(header->mUnprocessedArgv);
		}
		header->mUnprocessedArgv = (char**)malloc(header->mArgc * sizeof(char*));
		memset(header->mUnprocessedArgv, 0, header->mArgc * sizeof(char*));
		header->mUnprocessedArgc = 0;
	}
}

