#ifndef ZEOS_QUEUE_H
#define ZEOS_QUEUE_H

#define QUEUE_SIZE 32

extern struct queue {
  int read;
  int write;
  char queue[QUEUE_SIZE];
};
#define Q_IS_FULL(q) \
  ({__typeof__(q) _q = (q); \
    ((q).write == QUEUE_SIZE-1 && (q).read == 0) || (q).read == (q).write+1 ? 1 : 0;})

#define Q_IS_EMPTY(q) \
  ({__typeof__(q) _q = (q); \
    (q).read == -1 && (q).write == -1 ? 1 : 0;})

#define Q_COUNT(q) \
  ({__typeof__(q) _q = (q); \
    ((q).write - (q).read)%QUEUE_SIZE + 1;})

void INIT_QUEUE(struct queue *q);
int is_full(struct queue *q);
int add_to_queue(char c, struct queue *q);
char get_first(struct queue *q);
char *queue_get_chunk(struct queue *q, int size);


#endif //ZEOS_QUEUE_H
