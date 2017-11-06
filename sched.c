/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct *idle_task;
//struct task_struct *init_task;
unsigned int current_quantum;

//int count = 0;

/*void tsk_sw(){
	if(!count){
		task_switch(idle_task);
		count = 1;
	}
	else {
		task_switch(init_task);
		count = 0;
	}
}*/


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t)
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");
	while(1)
	{
	printk("IDLE");
	}
}

void init_idle (void)
{
  //Getting the a free task_union for allocate the process
  struct list_head *element = list_first(&freequeue);
  list_del(element);
  union task_union *idle_union = list_entry(element, union task_union, task.list);

  //Setting the PID to 0
  idle_union->task.PID = 0;

  //Setting quantum and stats
  idle_union->task.quantum = DEFAULT_QUANTUM;
  init_stats(&idle_union->task.p_stats);

  //Initialize the new directory
  allocate_DIR(&idle_union->task);

  idle_union->stack[KERNEL_STACK_SIZE-1] = (unsigned long) &cpu_idle;
  idle_union->stack[KERNEL_STACK_SIZE-2] = 0;
  idle_union->task.kernel_esp = (int *) &idle_union->stack[KERNEL_STACK_SIZE-2];


  idle_task = &idle_union->task;
}

void init_task1(void)
{
	struct list_head *element = list_first(&freequeue);
	list_del(element);
	union task_union *task1_union = list_entry(element, union task_union, task.list);

	task1_union->task.PID = 1;

	//Setting quantum and stats
	task1_union->task.quantum = DEFAULT_QUANTUM;
	init_stats(&task1_union->task.p_stats);

	allocate_DIR(&task1_union->task);

	set_user_pages(&task1_union->task);
	tss.esp0 = (DWord) &task1_union->stack[KERNEL_STACK_SIZE];
	set_cr3(task1_union->task.dir_pages_baseAddr);
	//init_task = &task1_union->task;
}


void init_sched(){
  INIT_LIST_HEAD(&freequeue);
  INIT_LIST_HEAD(&readyqueue);
	int i = 0;
  for (i = 0; i < NR_TASKS; ++i) {
    list_add(&(task[i].task.list), &freequeue);
  }
}

struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

void task_switch(union task_union *new) {
	__asm__ __volatile__(
	"push %esi;"
  "push %edi;"
  "push %ebx"
	);
  inner_task_switch(new);
  __asm__ __volatile__(
  "pop %ebx;"
  "pop %edi;"
  "pop %esi"
  );
}

void inner_task_switch(union task_union *new) {
  /*tss.esp0 = (DWord) new->stack[KERNEL_STACK_SIZE];
  set_cr3(new->task.dir_pages_baseAddr);
  void * old = current()->kernel_esp;
  void  *next = (new->task).kernel_esp;
  __asm__ __volatile__(
  "movl %%ebp,%0;"
  "movl %1, %%esp;"
  "popl %%ebp;"
  "ret"
  :
  : "g" (old), "g" (next)
  );
  */

  page_table_entry *new_DIR = get_DIR(&new->task);
  tss.esp0 = (int)&(new->stack[KERNEL_STACK_SIZE]);
  set_cr3(new_DIR);

  __asm__ __volatile__(
	"movl %%ebp, %0;"
	: "=g" (current()->kernel_esp)
	:
	);
  __asm__ __volatile__(
	"movl %0, %%esp;"
	:
	:"g" (new->task.kernel_esp)
	);
  __asm__ __volatile__ (
	"popl %%ebp;"
	:
	: );
  __asm__ __volatile__(
	"ret;"
	:
	: );

}


void update_sched_data_rr() {
  --current_quantum;
  --current()->p_stats.remaining_ticks;
}

int needs_sched_rr() {
  if (!current_quantum && !list_empty(&freequeue)) return 1;
  else {
    current_quantum = get_quantum(current());
    return 0;
  }
}

void update_process_state_rr(struct task_struct *t, struct list_head *dest) {
  if(dest == &readyqueue){
    t->state = ST_READY;
  }
  else if(dest == &freequeue) t->state = ST_BLOCKED;
  else {
    t->state = ST_RUN;
  }
  list_add_tail(&t->list, dest);
}

void sched_next_rr() {
  struct list_head *element;
  struct task_struct *t;

  if(!list_empty(&readyqueue)){
    element = list_first(&readyqueue);
    list_del(element);
    t = list_head_to_task_struct(element);
  }
  else t = idle_task;

  t->state = ST_RUN;
  t->p_stats.total_trans++;
  t->p_stats.remaining_ticks = get_quantum(t);

  current_quantum = get_quantum(t);

  task_switch((union task_union*) t);

}

void schedule() {
  update_sched_data_rr();
  if(needs_sched_rr()) {
    update_process_state_rr(current(), &freequeue);
    sched_next_rr();
  }
}

unsigned int get_quantum(struct task_struct *t) {
  return t->quantum;
}

void set_quantum(struct task_struct *t, unsigned int new_quantum) {
  t->quantum = new_quantum;
}
