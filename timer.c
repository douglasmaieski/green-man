#define _GNU_SOURCE

#include "man.h"
#include "avl_tree.h"

#include <time.h>

static int cmp(const struct avl_tree_node *a,
               const struct avl_tree_node *b)
{
  const struct timer *ta = MEMBER_TO_PARENT(struct timer, a, node);
  const struct timer *tb = MEMBER_TO_PARENT(struct timer, b, node);

  if (ta->time < tb->time)
    return -1;

  if (ta->time > tb->time)
    return 1;
  
  if (a < b)
    return -1;
  
  if (a > b)
    return 1;

  return 0;
}

void man_update_time(MAN *man)
{
  man->time_now = get_time_ns();
}

MWorker *man_get_timer_worker(MAN *man)
{
  struct avl_tree_node *node = avl_tree_first_in_postorder(man->timer_root);
  if (!node)
    return NULL;

  Timer *timer = MEMBER_TO_PARENT(Timer, node, node);
  MWorker *worker = MEMBER_TO_PARENT(MWorker, timer, timer);

  if (worker->timer.time > man->time_now)
    return NULL;

  // remove the worker from the tree
  avl_tree_remove(&man->timer_root, &worker->timer.node);

  return worker;
}

void _man_add_timer_worker(MAN *man, MWorker *worker, u64 time)
{
  worker->timer.time = time;
  avl_tree_insert(&man->timer_root, &worker->timer.node, cmp);
}