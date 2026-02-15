#include "queue.h"
#include "thread.h"
#include <unistd.h>

void queue_init(struct queue *q)
{
  q->dequeue_pos = q->data;
  q->enqueue_pos = q->data;
}

long enqueue(struct queue *q, void *ptr)
{
  barrier();
  void **next = q->enqueue_pos + 1;
  if (next == q->data + QUEUE_SIZE)
    next = q->data;

  if (next == q->dequeue_pos)
    return 0;
  
  *q->enqueue_pos = ptr;
  barrier();
  q->enqueue_pos = next;
  return 1;
}

void *dequeue(struct queue *q, int *ok)
{
  barrier();
  if (q->dequeue_pos == q->enqueue_pos) {
    *ok = 0;
    return NULL;
  }

  void **next = q->dequeue_pos + 1;
  if (next == q->data + QUEUE_SIZE)
    next = q->data;
  
  void *ptr = (void*)*q->dequeue_pos;
  barrier();
  q->dequeue_pos = next;

  *ok = 1;
  return ptr;
}

void *spin_dequeue(struct queue *q)
{
  barrier();
  while (q->dequeue_pos == q->enqueue_pos) {
    __asm__ __volatile__
    (
      "pause"
      :
      :
    );
    barrier();
  }

  void **next = q->dequeue_pos + 1;
  if (next == q->data + QUEUE_SIZE)
    next = q->data;
  
  void *ptr = (void*)*q->dequeue_pos;
  barrier();
  q->dequeue_pos = next;

  return ptr;
}