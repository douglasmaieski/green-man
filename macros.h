#ifndef MY_MACROS_H
#define MY_MACROS_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define MEMBER_TO_PARENT(type, member, member_name) \
    ((type *)((char *)(member) - offsetof(type, member_name)))

#define ERROR(msg) {\
  /*print msg file and number*/ \
  fprintf(stderr, "Error: %s at %s:%d\n", msg, __FILE__, __LINE__); \
  exit(EXIT_FAILURE); \
}

#define DEBUG(msg) {\
  /*print msg file and number*/ \
  fprintf(stderr, "Debug: %s at %s:%d\n", msg, __FILE__, __LINE__); \
}

#endif