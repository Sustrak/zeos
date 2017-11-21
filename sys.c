/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <system.h>
#include <errno.h>
#include <entry.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define SIZE_BUFFER 32

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	update_user_ticks(&current()->p_stats);
	update_sys_ticks(&current()->p_stats);
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	update_user_ticks(&current()->p_stats);
    update_sys_ticks(&current()->p_stats);
	return current()->PID;
}

int sys_fork(){
  update_user_ticks(&current()->p_stats);

	
  int PID=-1;
	
  if(list_empty(&freequeue)) {
	  update_sys_ticks(&current()->p_stats);
	  return -ENOMEM;
  }
  struct list_head *element = list_first(&freequeue);
  list_del(element);
  union task_union *child_union = list_entry(element, union task_union, task.list);
  union task_union *parent_union = (union task_union*) current();

  page_table_entry *parent_page_table = get_PT(current());

  unsigned int frames[NUM_PAG_DATA];
  unsigned int pag, i;
	
  for(pag = 0; pag < NUM_PAG_DATA; pag++){
    frames[pag] = (unsigned int) alloc_frame();
    if(frames[pag] < 0){
      for(i = 0; i < pag; ++i){
        free_frame(frames[i]);
      }
      list_add_tail(element, &freequeue);
      update_sys_ticks(&current()->p_stats);
      return -EAGAIN;
    }
  }

  copy_data(parent_union, child_union, sizeof(union task_union));
  child_union->task.dir_number = allocate_DIR(&child_union->task);

  page_table_entry *child_page_table = get_PT(&child_union->task);

  for(pag = 0; pag < NUM_PAG_KERNEL+NUM_PAG_CODE; pag++){
    set_ss_pag(child_page_table, pag, get_frame(parent_page_table, pag));
  }

  int free_pag = NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA+1;
  for(pag = 0; pag < NUM_PAG_DATA; pag++){
    set_ss_pag(child_page_table, NUM_PAG_KERNEL+NUM_PAG_CODE+pag, frames[pag]);
    set_ss_pag(parent_page_table, free_pag+pag, frames[pag]);
    copy_data((void *)((NUM_PAG_KERNEL + NUM_PAG_CODE + pag) * PAGE_SIZE), (void *)((free_pag + pag) * PAGE_SIZE), PAGE_SIZE);
    del_ss_pag(parent_page_table, free_pag+pag);
  }

  set_cr3(get_DIR(&parent_union->task));

  PID = nextPID++;
  child_union->task.PID = PID;

  child_union->stack[KERNEL_STACK_SIZE-18] = (unsigned long) &ret_from_fork;
  child_union->stack[KERNEL_STACK_SIZE-19] = 0;
  child_union->task.kernel_esp = (int *) &child_union->stack[KERNEL_STACK_SIZE - 19];

  init_stats(&child_union->task.p_stats);
  child_union->task.state = ST_READY;
  list_add_tail(&child_union->task.list, &readyqueue);
  
  update_sys_ticks(&current()->p_stats);
  return PID;
}

void sys_exit()
{
	
  update_user_ticks(&current()->p_stats);

  struct task_struct *current_task = current();
  --current_task->dir_number;
  if(current_task->dir_number == 0) {
    page_table_entry *current_pt = get_PT(current_task);
    unsigned int pag;
    for (pag = 0; pag < NUM_PAG_DATA; ++pag) {
      free_frame(get_frame(current_pt, PAG_LOG_INIT_DATA + pag));
      del_ss_pag(current_pt, PAG_LOG_INIT_DATA + pag);
    }
  }
  list_add_tail(&current_task->list, &freequeue);
  current_task->PID = -1;
  sched_next_rr();
}

int sys_write(int fd, char *buffer, int size)
{
  /*
   * fd: file descriptor. In this delivery it must always be 1
   * buffer: pointer to the bytes.
   * size: number of bytes.
   */
  update_user_ticks(&current()->p_stats);
  int ret;
  char buff[SIZE_BUFFER];
  int error;

  ret = check_fd(fd, ESCRIPTURA);
  if (ret < 0){
	  update_sys_ticks(&current()->p_stats);
	  return ret;
  }
  if (buffer == NULL) {
	  update_sys_ticks(&current()->p_stats);
	  return -EFAULT;
  }
  if (size < 0) {
	  update_sys_ticks(&current()->p_stats);
	  return -EINVAL;
  }
  while(size > SIZE_BUFFER){
    error = copy_from_user(buffer, buff, SIZE_BUFFER);
    if (error == -1) {
		update_sys_ticks(&current()->p_stats);
		return error;
	}
    ret += sys_write_console(buff, size);
    buffer += SIZE_BUFFER;
    size -= SIZE_BUFFER;
  }
  error = copy_from_user(buffer, buff, SIZE_BUFFER);
  if (error == -1) {
	  update_sys_ticks(&current()->p_stats);
	  return error;
  }
  ret += sys_write_console(buff, size);

  update_sys_ticks(&current()->p_stats);
  return ret;
}

int sys_gettime()
{
  update_user_ticks(&current()->p_stats);
  update_sys_ticks(&current()->p_stats);
  return zeos_clock;
}

int sys_getstats(int pid, struct stats *st){
  update_user_ticks(&current()->p_stats);
  if(!access_ok(VERIFY_WRITE, st, sizeof(struct stats))){
	  update_sys_ticks(&current()->p_stats);
	  return -EFAULT;
  }
  if(pid < 0){
	  update_sys_ticks(&current()->p_stats);
	  return -EINVAL;
  }
  int i;
  for(i = 0; i < NR_TASKS; i++){
    if(task[i].task.PID == pid){
      copy_to_user(&task[i].task.p_stats, st, sizeof(struct stats));
      update_sys_ticks(&current()->p_stats);
      return 0;
    }
  }
  update_sys_ticks(&current()->p_stats);
  return -ESRCH;
}

int sys_clone(void (*function)(void), void *stack){
  update_user_ticks(&current()->p_stats);

  if(!access_ok(0, function, sizeof(void))) return -EACCES;
  if(!access_ok(0, stack, sizeof(void))) return -EACCES;
  if(list_empty(&freequeue)){
    update_sys_ticks(&current()->p_stats);
    return -ENOMEM;
  }
  struct list_head *element = list_first(&freequeue);
  union task_union *child_union = list_entry(element, union task_union, task.list);
  union task_union *father_union = (union task_union*) current();

  copy_data(father_union, child_union, sizeof(union task_union));

  int dir = father_union->task.dir_number;
  ++allocated_dirs[dir];
  child_union->stack[KERNEL_STACK_SIZE - 19] = 0;
  child_union->stack[KERNEL_STACK_SIZE - 18] = (unsigned long)&ret_from_fork;
  child_union->stack[KERNEL_STACK_SIZE - 5] = (unsigned long) function;  //eip
  child_union->stack[KERNEL_STACK_SIZE - 2] = (unsigned long) stack;     //esp
  init_stats(&child_union->task.p_stats);
  child_union->task.PID = nextPID++;
  update_sys_ticks(&current()->p_stats);
  return child_union->task.PID;
}

int sys_sem_init(int n_sem, unsigned int value){
  update_user_ticks(&current()->p_stats);

  if(0 > n_sem > 20) return -EBADF;
  struct semaphore *sem = &semaphores[n_sem];
  if(sem->in_use) return -EBUSY;
  sem->owner = current()->PID;
  sem->counter = value;
  INIT_LIST_HEAD(&sem->blocked);

  update_sys_ticks(&current()->p_stats);
  return 0;
}

int sys_sem_wait(int n_sem){
  update_user_ticks(&current()->p_stats);
  //Destroyed while the process is blocked ???
  if(0 > n_sem > 20) return -EBADF;
  struct semaphore *sem = &semaphores[n_sem];
  if(sem->counter <= 0){
    list_add_tail(&current()->list, &sem->blocked);
    update_user_ticks(&current()->p_stats);
    sched_next_rr();
  }
  else sem->counter--;

  update_sys_ticks(&current()->p_stats);
  return 0;
}

int sys_sem_signal(int n_sem){
  update_user_ticks(&current()->p_stats);
  if(0 > n_sem > 20) return -EBADF;
  struct semaphore *sem = &semaphores[n_sem];
  if(list_empty(&sem->blocked)) sem->counter++;
  else {
    struct list_head *element = list_first(&sem->blocked);
    list_del(&sem->blocked);
    list_add_tail(element, &readyqueue);
  }

  update_sys_ticks(&current()->p_stats);
  return 0;
}

int sys_sem_destroy(int n_sem){
  update_user_ticks(&current()->p_stats);
  if(0 > n_sem > 20) return -EBADF;
  struct semaphore *sem = &semaphores[n_sem];
  if(!sem->in_use) return -EPERM;
  if(sem->owner != current()->PID) return -EPERM;

  struct list_head *it = NULL;
  list_for_each(it, &sem->blocked){
    list_del(&sem->blocked);
    list_add_tail(it, &readyqueue);
  }
  sem->in_use = 0;
  update_sys_ticks(&current()->p_stats);
  if(!it) return -1;
  return 0;

}
