#ifndef RINGS_H
#define RINGS_H

#include <linux/io_uring.h>

struct sq_ring {
  unsigned int *head;
  unsigned int *tail;
  unsigned int *entries;
  unsigned int *flags;
  unsigned int *dropped;
  unsigned int *array;
  struct io_uring_sqe *sqes;
  unsigned int mask;
  int fd;
};

struct cq_ring {
  unsigned int *head;
  unsigned int *tail;
  unsigned int *entries;
  unsigned int *overflow;
  unsigned int *flags;
  struct io_uring_cqe *cqes;
  unsigned int mask;
  int fd;
};

typedef struct rings Rings;
struct rings {
  struct sq_ring sq;
  struct cq_ring cq;
  int sq_lock; // must be locked before cq_lock when both are being used
  int cq_lock;
};

// returns 0 on success and -1 on failure
int rings_setup(struct rings *rings, int entries);

// returns 0 on success and -1 on failure
int rings_submit(struct rings *rings, struct io_uring_sqe *sqe);

// flush pending submissions to the kernel, optionally waiting for wait_nr completions
// returns number of SQEs submitted, or -1 on error
int rings_flush(struct rings *rings, unsigned int wait_nr);

// return the number of items reaped
int rings_reap(struct rings *rings, struct io_uring_cqe *cqes, int max_count);

#endif