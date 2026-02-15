#define _GNU_SOURCE
#include "time_ns.h"
#include <time.h>

u64 get_time_ns(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (u64)now.tv_sec * 1000000000 + (u64)now.tv_nsec;
}
