#include <types.h>
#include "queue.h"

void INIT_QUEUE(struct queue *q) {
  q->read = q->write = -1;
}

inline int is_full(struct queue *q) {
  return q->write == q->read ? 1 : 0;
}

int add_to_queue(char c, struct queue *q) {
  if (Q_IS_FULL(*q)) return 0;
  if (q->write == QUEUE_SIZE-1 && q->read != 0) q->write = -1;
  q->queue[++q->write] = c;
  if (q->read == -1) q->read = 0;
  return 1;
}

char get_first(struct queue *q) {
  if (Q_IS_EMPTY(*q)) return '\0';
  char c = q->queue[q->read++];
  if (q->read == QUEUE_SIZE) q->read = 0;
  if (q->read-1 == q->write) q->read = q->write = -1;
}

char *queue_get_chunk(struct queue *q, int size) {
  if (size > QUEUE_SIZE) return NULL;
  char buf[size]; int i;
  for (i = 0; i < size; ++i) {
    buf[i] = q->queue[q->read++];
    if (q->read == QUEUE_SIZE) q->read = 0;
    if (q->read-1 == q->write) q->read = q->write = -1;
  }
  return buf;
}

