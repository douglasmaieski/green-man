/*
 * green_man.h - public API for the green-man cooperative I/O framework
 *
 * Include only this header in application code. All struct internals are
 * opaque; use the provided functions for field access.
 */
#ifndef GREEN_MAN_H
#define GREEN_MAN_H

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/uio.h>

/* ── Portable integer aliases ────────────────────────────────────── */

typedef uint64_t u64;
typedef int64_t  s64;

/* ── Sized opaque types ──────────────────────────────────────────── */
/*   The structs below have the correct size and u64 alignment so    */
/*   they can live on the stack, in arrays, or be malloc'd with      */
/*   plain sizeof.  The u64 fields are opaque; never access          */
/*   them directly.                                                  */

typedef struct man         MAN;
typedef struct mworker     MWorker;
typedef struct cond        Cond;
typedef struct worker_cond WorkerCond;

struct man         { u64 _opaque[32806]; };
struct mworker     { u64 _opaque[9];     };
struct cond        { u64 _opaque[4];     };
struct worker_cond { u64 _opaque[2];     };

/* ── Value type (must be visible so callers can construct literals) ─ */

typedef union mdatum MDatum;
union mdatum {
  void *ptr;
  u64   u64;
  s64   s64;
};

/* ── Lifecycle ───────────────────────────────────────────────────── */

MAN *man_setup(long worker_count,
               long stack_size,
               void (*work_fun)(MWorker *worker, MDatum arg));

int  man_init(MAN *man, void (*work_fun)(MWorker *worker, MDatum arg));
void man_add_worker(MAN *man, MWorker *worker);
void mworker_init(MWorker *worker, void *stack_top);

/* ── Accessors ───────────────────────────────────────────────────── */

MAN *mworker_get_man(MWorker *worker);
WorkerCond *cond_get_waiters(Cond *cond);

/* ── Event loop ──────────────────────────────────────────────────── */

int  man_submit_work(MAN *man, MDatum arg);
void man_work(MAN *man);
void man_return(MWorker *worker);

/* ── I/O ─────────────────────────────────────────────────────────── */

int man_read(MWorker *worker,
             int fd, long offset,
             void *buf, unsigned int length);

int man_write(MWorker *worker,
              int fd, long offset,
              const void *buf, unsigned int length);

int man_readv(MWorker *w,
              int fd, long offset,
              const struct iovec *iov, unsigned int iovcnt);

int man_writev(MWorker *w,
               int fd, long offset,
               const struct iovec *iov, unsigned int iovcnt);

int man_close(MWorker *worker, int fd);

int man_fsync(MWorker *worker, int fd);

/* ── File system ─────────────────────────────────────────────────── */

int man_openat(MWorker *worker,
               int dirfd, const char *pathname,
               int flags, int mode);

int man_unlinkat(MWorker *worker,
                 int dirfd, const char *pathname,
                 int flags);

/* ── Networking ──────────────────────────────────────────────────── */

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

int man_send(MWorker *worker,
             int sockfd, const void *buf,
             unsigned int len, int flags);

int man_recv(MWorker *worker,
             int sockfd, void *buf,
             unsigned int len, int flags);

int man_sendmsg(MWorker *worker,
                int sockfd,
                const struct msghdr *msg,
                int flags);

int man_recvmsg(MWorker *worker,
                int sockfd,
                struct msghdr *msg,
                int flags);

int man_shutdown(MWorker *worker, int sockfd, int how);

/* ── Condition variables ─────────────────────────────────────────── */

void cond_init(Cond *cond);
void worker_cond_init(WorkerCond *wcond, MWorker *worker);

void man_cond_wait(Cond *cond, WorkerCond *wcond);
void man_cond_wake_one(Cond *cond, MAN *man);
void man_cond_wake_n(Cond *cond, MAN *man, unsigned long n);
void man_cond_wake_all(Cond *cond, MAN *man);

/* ── Timers / clock ──────────────────────────────────────────────── */

void man_sleep(MWorker *worker, u64 wakeup_time);
u64  get_time_ns(void);

#endif /* GREEN_MAN_H */
