/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>

#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024
#define DEFAULT_QUANTUM 10
#define NR_SEMAPHORES 20

extern int current_quantum;

enum state_t { ST_RUN, ST_READY, ST_BLOCKED};

struct task_struct {
  int PID;			/* Process ID. This MUST be the first field of the struct. */
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;
  int *kernel_esp;

  unsigned int quantum;
  enum state_t state;
  struct stats p_stats;
  int dir_number;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

struct semaphore {
  unsigned int in_use;
  int counter;
  struct list_head blocked;
  int owner;
};

extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task; /* Vector de tasques */
extern struct task_struct *idle_task;
extern struct list_head freequeue;
extern struct list_head readyqueue;
extern union task_union *idle_union;
extern int allocated_dirs[NR_TASKS];
extern struct semaphore semaphores[NR_SEMAPHORES];



#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

struct task_struct * current();

void task_switch(union task_union *new);

void inner_task_switch(union task_union *new);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

/* SCHEDULER */
void schedule();

/* Additional methods */
unsigned int get_quantum(struct task_struct *t);
void set_quantum(struct task_struct *t, unsigned int new_quantum);

#endif  /* __SCHED_H__ */
