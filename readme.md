# GREEN MAN

GREEN MAN is a single-threaded amd64 green-threads runtime for C that "asyncfies" code by running many cooperative workers on one OS thread while integrating I/O via `io_uring`.

It is intended for high-concurrency servers and event-driven applications where you want C-level control and performance without turning your entire program into callbacks.

> **Status:** still evolving. APIs may change.

---

## Highlights

* Green threads (workers) scheduled cooperatively on a single OS thread
* `io_uring` under the hood for non-blocking file and socket operations
* Transparent-ish async style: write code as if it blocks, but the runtime yields
* Very small stacks compared to OS threads (you control stack size)
* Built-in sleeping and condition variables
* Well-suited for web servers and high-connection-count services

---

## Benchmarks

The following benchmarks compare **GREEN MAN** against two common server models:

* **Thread-per-connection**
* **Single-thread `epoll`**

All implementations serve HTTP/1.1 with keep-alive and respond once per complete request (requests are read until `\r\n\r\n`). The response body size is fixed per test.

### Test Environment

* **CPU:** Intel i3 (7th generation laptop)
* **Kernel:** Linux 6.14.0-29
* **Server CPU affinity:** pinned to a single core
* **Load generator:** `wrk`, allowed multiple cores to avoid client bottlenecks
* **Concurrency:** 1000 connections
* **Duration:** 30 seconds
* **Protocol:** HTTP/1.1 keep-alive
* **All servers:** identical response headers and payload size

### Commands

Servers (example):

```bash
taskset -c 0 ./server
```

Load generator:

```bash
taskset -c 1-3 wrk --latency -t3 -c1000 -d30s http://127.0.0.1:8080
```

---

## Results

### Small response (13 bytes body)

| Model                 |     Req/s |      p50 |        p99 |
| --------------------- | --------: | -------: | ---------: |
| Thread-per-connection |      ~52k |    ~17ms |      ~43ms |
| Epoll                 |      ~78k |    ~12ms |      ~16ms |
| **GREEN MAN**         | **~95k** | **~10ms** | **~11ms** |

---

### Large response (8 KB body)

| Model                 |   Req/s |        p50 |        p99 |
| --------------------- | ------: | ---------: | ---------: |
| Thread-per-connection |   45.4k |    18.64ms |    32.62ms |
| Epoll                 |   50.4k |    19.75ms |    21.08ms |
| **GREEN MAN**         | **70.3k** | **13.28ms** | **14.55ms** |

---

## Interpretation

* GREEN MAN consistently achieves **higher throughput** on a single core.
* Tail latency (p99) is **lower** than both threaded and epoll-based designs.
* Thread-per-connection suffers from scheduler overhead and context switching under high concurrency.
* Epoll improves throughput over threads but still exhibits higher tail latency.

### Notes

* Servers were CPU-bound on a single core.
* `wrk` was allowed multiple cores to avoid client-side saturation.
* Some threaded runs exhibited request timeouts under high load; GREEN MAN did not.

These results demonstrate that a single-threaded, cooperative green-thread runtime integrated with `io_uring` can outperform traditional concurrency models for high-connection-count workloads while providing lower latency.

---

## Core Concepts

### MAN

The runtime / scheduler. Owns:

* the worker run queue
* the timer queue
* the `io_uring` submission and completion machinery
* condition variable wait lists

The runtime is initialized with a `work_fun` callback that acts as the entry point for submitted jobs.

---

### MWorker

A green thread with:

* its own stack
* saved execution state
* a link back to its owning `MAN`

Workers run cooperatively. Any blocking operation **must** go through GREEN MAN APIs.

---

### MDatum

A small union for passing arguments into work functions:

```c
union mdatum {
  void *ptr;
  u64 u64;
  s64 s64;
};
```

---

## API Overview

### Runtime lifecycle

```c
int  man_init(MAN *man, void (*work_fun)(MWorker *worker, MDatum arg));
void man_work(MAN *man);
int  man_submit_work(MAN *man, MDatum arg);
void man_add_worker(MAN *man, MWorker *worker);
```

---

### Worker setup and returning

```c
void mworker_init(MWorker *worker, void *stack_top);
void man_return(MWorker *worker);
```

`man_return()` terminates the current worker and yields back to the scheduler.

---

### Async I/O wrappers

All of the following suspend the current worker until completion and return
either a positive result or `-errno`.

#### File I/O

```c
int man_openat(MWorker *worker, int dirfd, const char *pathname, int flags, int mode);
int man_read(MWorker *worker, int fd, long offset, void *buf, unsigned int length);
int man_write(MWorker *worker, int fd, long offset, const void *buf, unsigned int length);
int man_readv(MWorker *worker, int fd, long offset,
              const struct iovec *iov, unsigned int iovcnt);
int man_writev(MWorker *worker, int fd, long offset,
               const struct iovec *iov, unsigned int iovcnt);
int man_fsync(MWorker *worker, int fd);
int man_close(MWorker *worker, int fd);
int man_unlinkat(MWorker *worker, int dirfd, const char *pathname, int flags);
```

> Passing `offset = -1` uses the current file position (like `readv/writev`).

---

#### Sockets

```c
int man_socket(MWorker *worker, int domain, int type, int protocol);
int man_connect(MWorker *worker, int sockfd,
                const struct sockaddr *addr, socklen_t addrlen);
int man_accept(MWorker *worker, int sockfd,
               struct sockaddr *addr, socklen_t *addrlen, int flags);

int man_send(MWorker *worker, int sockfd,
             const void *buf, unsigned int len, int flags);
int man_recv(MWorker *worker, int sockfd,
             void *buf, unsigned int len, int flags);

int man_sendmsg(MWorker *worker,
                int sockfd, const struct msghdr *msg, int flags);
int man_recvmsg(MWorker *worker,
                int sockfd, struct msghdr *msg, int flags);

int man_shutdown(MWorker *worker, int sockfd, int how);
```

All common flags (`MSG_PEEK`, `MSG_WAITALL`, `MSG_TRUNC`, ancillary data, etc.)
are supported.

---

### Sleeping

```c
void man_sleep(MWorker *worker, u64 wakeup_time);
```

`sleep` uses the runtime’s internal timer queue.
`wakeup_time` is an absolute time in nanoseconds.

---

### Conditions

```c
void man_cond_wait(Cond *cond, WorkerCond *wcond);
void man_cond_wake_one(Cond *cond, MAN *man);
void man_cond_wake_n(Cond *cond, MAN *man, unsigned long n);
void man_cond_wake_all(Cond *cond, MAN *man);
```

Condition variables are cooperative and integrated with the scheduler.

---

## Minimal Example (Echo Server)

```c
#include <green_man.h>
#include <stdlib.h>
#include <netinet/in.h>

static void work_fun(MWorker *worker, MDatum arg)
{
  int server_fd = arg.s64;

  for (;;) {
    int client_fd = man_accept(worker, server_fd, NULL, NULL, 0);
    if (client_fd < 0)
      break;

    for (;;) {
      char buf[1024];
      int n = man_recv(worker, client_fd, buf, sizeof buf, 0);
      if (n <= 0)
        break;

      man_send(worker, client_fd, buf, n, 0);
    }

    man_close(worker, client_fd);
  }

  man_return(worker);
}

int main(void)
{
  MAN *man = malloc(sizeof(MAN));
  man_init(man, work_fun);

  for (int i = 0; i < 128; i++) {
    char *stack = malloc(32 * 1024);
    MWorker *w = malloc(sizeof(MWorker));
    mworker_init(w, stack + 32 * 1024);
    man_add_worker(man, w);
  }

  int server = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(8080),
    .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
  };

  int opt = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  bind(server, (struct sockaddr *)&addr, sizeof(addr));
  listen(server, 128);

  for (int i = 0; i < 128; i++)
    man_submit_work(man, (MDatum){ .s64 = server });

  man_work(man);
  return 0;
}
```

---

## Invariants and Notes

* **CPU-heavy work blocks all workers.** This is cooperative concurrency.
* Pointers passed to async calls (`iovec`, `msghdr`, buffers) must remain valid
  until the operation completes.
* Return values follow `io_uring` semantics: bytes transferred or `-errno`.

---

## Summary

GREEN MAN provides a small but complete async runtime for C built directly on
`io_uring`, with:

* real green threads
* no callbacks
* predictable control flow
* minimal abstraction overhead

It is suitable for experimentation, high-concurrency servers, and systems code
where explicit control matters.
