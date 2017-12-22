#ifndef ZEOS_QUEUE_H
#define ZEOS_QUEUE_H

#define QUEUE_SIZE 8

struct queue {
  int read;
  int write;
  char queue[QUEUE_SIZE];
};
#define Q_IS_FULL(q) \
  ({((q).write == QUEUE_SIZE-1 && (q).read == 0) || (q).read == (q).write+1 ? 1 : 0;})

#define Q_IS_EMPTY(q) \
  ({(q).read == -1 && (q).write == -1 ? 1 : 0;})


void INIT_QUEUE(struct queue *q);
int add_to_queue(char c, struct queue *q);
char get_first(struct queue *q);
int queue_count(struct queue *q);

#endif //ZEOS_QUEUE_H
