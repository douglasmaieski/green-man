#ifndef MAN_H
#define MAN_H

#include "rings.h"
#include "avl_tree.h"
#include "types.h"
#include "macros.h"
#include "queue.h"
#include "time_ns.h"
#include "thread.h"

#include <sys/socket.h>

typedef union mdatum MDatum;
typedef struct mworker MWorker;
typedef struct man MAN;
typedef struct timer Timer;
typedef struct cond Cond;
typedef struct worker_cond WorkerCond;

union mdatum {
  void *ptr;
  u64 u64;
  s64 s64;
};

struct timer {
  AVLTreeNode node;
  u64 time;
};

struct mworker {
  MAN *man;
  MWorker *next;
  void *instruction_ptr;
  void *stack_top;
  void *stack;
  Timer timer;
};

struct cond {
  Cond *next;
  WorkerCond *waiters;
  unsigned long wake_n;
};

struct worker_cond {
  WorkerCond *next_waiter;
  MWorker *worker;
};

struct man {
  u64 registers[16];
  void *next_instruction_ptr;
  MWorker *next_worker;
  Rings rings;
  AVLTreeNode *timer_root;
  u64 time_now;
  void (*work_fun)(MWorker *worker, MDatum arg);
  Queue ops;
  Cond *conds;
};

int man_init(MAN *man, void (*work_fun)(MWorker *worker, MDatum arg));

void man_add_worker(MAN *man, MWorker *worker);

int man_submit_work(MAN *man, MDatum arg);

void man_work(MAN *man);

void mworker_init(MWorker *worker, void *stack_top);

void man_return(MWorker *worker);

int man_read(MWorker *worker,
             int fd,
             long offset,
             void *buf,
             unsigned int length);

int man_write(MWorker *worker,
              int fd,
              long offset,
              const void *buf,
              unsigned int length);

int man_close(MWorker *worker, int fd);

int man_openat(MWorker *worker,
               int dirfd,
               const char *pathname,
               int flags,
               int mode);

int man_fsync(MWorker *worker, int fd);

int man_socket(MWorker *worker, int domain, int type, int protocol);

int man_connect(MWorker *worker,
                int sockfd,
                const struct sockaddr *addr,
                socklen_t addrlen);

int man_accept(MWorker *worker,
               int sockfd,
               struct sockaddr *addr,
               socklen_t *addrlen,
               int flags);

int man_unlinkat(MWorker *worker,
                 int dirfd,
                 const char *pathname,
                 int flags);

int man_send(MWorker *worker,
             int sockfd,
             const void *buf,
             unsigned int len,
             int flags);

int man_recv(MWorker *worker,
             int sockfd,
             void *buf,
             unsigned int len,
             int flags);

int man_sendmsg(MWorker *worker,
                int sockfd,
                const struct msghdr *msg,
                int flags);

int man_recvmsg(MWorker *worker,
                int sockfd,
                struct msghdr *msg,
                int flags);

int man_writev(MWorker *w,
               int fd,
               long offset,
               const struct iovec *iov,
               unsigned int iovcnt);

int man_readv(MWorker *w,
              int fd,
              long offset,
              const struct iovec *iov,
              unsigned int iovcnt);

int man_shutdown(MWorker *worker, int sockfd, int how);

void man_sleep(MWorker *worker, u64 wakeup_time);


void man_cond_wait(Cond *cond, WorkerCond *wcond);
void man_cond_wake_one(Cond *cond, MAN *man);
void man_cond_wake_n(Cond *cond, MAN *man, unsigned long n);
void man_cond_wake_all(Cond *cond, MAN *man);

#endif