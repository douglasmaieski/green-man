#include "man.h"
#include <sys/mman.h>
#include <string.h>

MWorker *man_get_timer_worker(MAN *man);
void man_update_time(MAN *man);
void wake_up_worker(MAN *man, MWorker *worker);
MWorker *_get_conds_wake(MAN *man);

/* ── Static size checks ─────────────────────────────────────────── */
/*   These must match the _opaque[] array sizes in green_man.h.      */
/*   If a struct changes, the build breaks here as a reminder.       */

_Static_assert(sizeof(struct man)         == 32806 * 8, "update MAN _opaque size in green_man.h");
_Static_assert(sizeof(struct mworker)     ==     9 * 8, "update MWorker _opaque size in green_man.h");
_Static_assert(sizeof(struct cond)        ==     4 * 8, "update Cond _opaque size in green_man.h");
_Static_assert(sizeof(struct worker_cond) ==     2 * 8, "update WorkerCond _opaque size in green_man.h");

MAN *man_setup(long worker_count,
               long stack_size,
               void (*work_fun)(MWorker *worker, MDatum arg))
{
  if (worker_count < 1 || stack_size < 1024)
    goto err;

  MAN *man = malloc(sizeof(MAN));
  if (!man)
    goto err;

  man_init(man, work_fun);

  u8 *stacks = malloc(worker_count * (4096 + stack_size) + 4095);
  if (!stacks)
    goto err1;

  MWorker *workers = malloc(sizeof(MWorker) * worker_count);
  if (!workers)
    goto err2;

  // align to 4096
  u64 ptr = (u64)stacks;
  ptr += 4095;
  ptr &= ~4095;

  for (long i = 0; i < worker_count; ++i) {
    // protect the stack to catch bugs more easily
    if (mprotect((void *)ptr, 4096, PROT_NONE) != 0)
      goto err3;

    MWorker *w = workers + i;
    ptr += 4096 + stack_size;  // stack is the top address
    mworker_init(w, (void *)ptr);
    man_add_worker(man, w);
  }

  return man;

err3:
  free(workers);
err2:
  free(stacks);
err1:
  free(man);
err:
  return NULL;
}

MAN *mworker_get_man(MWorker *worker)
{
  return worker->man;
}

WorkerCond *cond_get_waiters(Cond *cond)
{
  return cond->waiters;
}

void cond_init(Cond *cond)
{
  memset(cond, 0, sizeof(Cond));
}

void worker_cond_init(WorkerCond *wcond, MWorker *worker)
{
  memset(wcond, 0, sizeof(WorkerCond));
  wcond->worker = worker;
}

int man_init(MAN *man, void (*work_fun)(MWorker *worker, MDatum arg))
{
  memset(man, 0, sizeof(MAN));

  if (rings_setup(&man->rings, 32768) == -1) {
    return 0;
  }

  man->timer_root = NULL;
  man->time_now = get_time_ns();
  man->work_fun = work_fun;
  man->next_worker = NULL;
  man->next_instruction_ptr = NULL;

  queue_init(&man->ops);

  return 1;
}

void man_add_worker(MAN *man, MWorker *worker)
{
  worker->man = man;
  worker->next = man->next_worker;
  man->next_worker = worker;
}

int man_submit_work(MAN *man, MDatum arg)
{
  return enqueue(&man->ops, arg.ptr);
}

void back_to_worker(MAN *man, MWorker *worker, int res);
void prepare_worker(MAN *man, MDatum arg, MWorker *worker);

void man_work(MAN *man)
{
  while (1) {
    man_update_time(man);

    rings_flush(&man->rings, 0);

    // timer entry point
    while (1) {
      MWorker *worker = man_get_timer_worker(man);
      if (!worker)
        break;

      wake_up_worker(man, worker);
    }

    // conditions entry point
    while (1) {
      MWorker *worker = _get_conds_wake(man);
      if (!worker)
        break;

      wake_up_worker(man, worker);
    }

    // IO URING entry point
    while (1) {
      struct io_uring_cqe cqe;
      int reaped = rings_reap(&man->rings, &cqe, 1);
      if (reaped < 1)
        break;

      MWorker *worker = (MWorker *)cqe.user_data;
      int ret = cqe.res;

      back_to_worker(worker->man, worker, ret);
    }

    // work queue entry point, spawn new workers
    int ok = 1;
    while (ok && man->next_worker) {
      void *ptr = dequeue(&man->ops, &ok);
      if (!ok)
        break;

      MDatum arg = {.ptr = ptr};
      MWorker *worker = man->next_worker;
      man->next_worker = worker->next;

      prepare_worker(man, arg, worker);
    }
  }
}

void mworker_init(MWorker *worker, void *stack_top)
{
  memset(worker, 0, sizeof(MWorker));
  worker->stack_top = stack_top;
}

void man_do_return(MWorker *worker);

void man_return(MWorker *worker)
{
  man_do_return(worker);
}