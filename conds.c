#include "man.h"

MWorker *_get_conds_wake(MAN *man)
{
  Cond *cond = man->conds;
  WorkerCond *waiter;

  while (cond) {
    if (cond->wake_n > 0) {
      --cond->wake_n;
      waiter = cond->waiters;
      if (waiter) {
        MWorker *worker = waiter->worker;
        cond->waiters = waiter->next_waiter;
        return waiter->worker;
      }
    }

    man->conds = cond->next;
    cond = cond->next;
  }

  return NULL;
}

void _man_cond_wait(Cond *cond, WorkerCond *w)
{
  w->next_waiter = cond->waiters;
  cond->waiters = w;
}

void man_cond_wake_one(Cond *cond, MAN *man)

{
  cond->wake_n = 1;
  cond->next = man->conds;
  man->conds = cond;
}

void man_cond_wake_n(Cond *cond, MAN *man, unsigned long n)
{
  cond->wake_n = n;
  cond->next = man->conds;
  man->conds = cond;
}


void man_cond_wake_all(Cond *cond, MAN *man)
{
  cond->wake_n = 0xffffffffffffffffUL;
  cond->next = man->conds;
  man->conds = cond;
}