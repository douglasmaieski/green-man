#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_CAPACITY (32768 - 3)
#define QUEUE_SIZE (QUEUE_CAPACITY + 1)

typedef struct queue Queue;
struct queue {
  void **dequeue_pos;
  void **enqueue_pos;
  void *data[QUEUE_SIZE];
};

void queue_init(struct queue *q);
long enqueue(struct queue *q, void *ptr);
void *dequeue(struct queue *q, int *ok);
void *spin_dequeue(struct queue *q);

#endif