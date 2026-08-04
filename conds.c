#include "man.h"

static Cond *_conds_unlink_head(MAN *man, Cond *cond)
{
  Cond *next = cond->next;

  man->conds = next;
  cond->next = NULL;
  cond->wake_n = 0;
  cond->queued = 0;

  return next;
}

MWorker *_get_conds_wake(MAN *man)
{
  Cond *cond = man->conds;

  while (cond) {
    if (cond->wake_n > 0) {
      WorkerCond *waiter = cond->waiters;
      if (waiter) {
        --cond->wake_n;
        cond->waiters = waiter->next_waiter;
        return waiter->worker;
      }
    }

    cond = _conds_unlink_head(man, cond);
  }

  return NULL;
}

void _man_cond_wait(Cond *cond, WorkerCond *w)
{
  w->next_waiter = cond->waiters;
  cond->waiters = w;
}

void man_cond_wake_n(Cond *cond, MAN *man, unsigned long n)
{
  if (n == 0)
    return;

  cond->wake_n += n;

  if (cond->queued)
    return;

  cond->queued = 1;
  cond->next = man->conds;
  man->conds = cond;
}

void man_cond_wake_one(Cond *cond, MAN *man)
{
  man_cond_wake_n(cond, man, 1);
}

void man_cond_wake_all(Cond *cond, MAN *man)
{
  unsigned long n = 0;

  for (WorkerCond *w = cond->waiters; w; w = w->next_waiter)
    ++n;

  man_cond_wake_n(cond, man, n);
}