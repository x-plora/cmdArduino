#ifndef CMD_TEST_PGMSPACE_H
#define CMD_TEST_PGMSPACE_H

#include <string.h>
#define PROGMEM
#define PSTR(value) value
#define strcmp_P strcmp
typedef const char *PGM_P;

#endif
