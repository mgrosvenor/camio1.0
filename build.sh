#! /bin/bash

INCLUDES="-I deps -I src -I ."
CFLAGS="-D_GNU_SOURCE -D_XOPEN_SOURCE=700 -D_BSD_SOURCE -std=c11 -Werror -Wall -Wno-missing-field-initializers -Wno-unused-command-line-argument -Wno-missing-braces "
#CFLAGS="-std=c11 -Werror -Wall"
LINKFLAGS="-Ideps/chaste -lexanic -lm "

SRC="camio_cat.c"
cake $SRC \
    --append-CFLAGS="$INCLUDES $CFLAGS" \
    --append-LINKFLAGS="$LINKFLAGS" \
    --no-git-root\
    --no-git-parent\
    --begintests tests/test_num_parser.c --endtests\
    $@




