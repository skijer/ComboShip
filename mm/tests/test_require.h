#ifndef MM_TEST_REQUIRE_H
#define MM_TEST_REQUIRE_H

#include <stdio.h>
#include <stdlib.h>

#define REQUIRE(expression)                                                                  \
    do {                                                                                     \
        if (!(expression)) {                                                                 \
            fprintf(stderr, "%s:%d: REQUIRE(%s) failed\n", __FILE__, __LINE__, #expression); \
            abort();                                                                         \
        }                                                                                    \
    } while (0)

#endif
