/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <system.h>
#include <errno.h>
#include <queue.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define SIZE_BUFFER 32

int chars_to_read;
int offset;

int sys_sem_destroy(int n_sem);
int sys_read_keyboard(char *buf, int count);

int check_fd(int fd, int permissions)
{
  if (fd!=1 && fd!=0) return -EBADF; /*EBADF*/
  if (fd==1 && permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  if (fd==0 && permissions!=LECTURA) return -EACCES; /*EACCES*/
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

int ret_from_fork(){
  return 0;
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

  int frames[NUM_PAG_DATA];
  unsigned int pag, i;

  for(pag = 0; pag < NUM_PAG_DATA; pag++){
    frames[pag] = alloc_frame();
    if(frames[pag] < 0){
      for(i = 0; i < pag; ++i){
        free_frame((unsigned int) frames[i]);
      }
      list_add_tail(element, &freequeue);
      update_sys_ticks(&current()->p_stats);
      return -EAGAIN;
    }
  }

  copy_data(parent_union, child_union, sizeof(union task_union));
  allocate_DIR(&child_union->task);

  page_table_entry *child_page_table = get_PT(&child_union->task);

  for(pag = 0; pag < NUM_PAG_KERNEL+NUM_PAG_CODE; pag++){
    set_ss_pag(child_page_table, pag, get_frame(parent_page_table, pag));
  }
  int free_pag = NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA+1;
  for(pag = 0; pag < NUM_PAG_DATA; pag++){
    set_ss_pag(child_page_table, NUM_PAG_KERNEL+NUM_PAG_CODE+pag, (unsigned int) frames[pag]);
    set_ss_pag(parent_page_table, free_pag+pag, (unsigned int) frames[pag]);
    copy_data((void *)((NUM_PAG_KERNEL + NUM_PAG_CODE + pag) * PAGE_SIZE), (void *)((free_pag + pag) * PAGE_SIZE), PAGE_SIZE);
    del_ss_pag(parent_page_table, free_pag+pag);
  }

  int NUM_PAG_HEAP = parent_union->task.pages_heap;
  int framesH[NUM_PAG_HEAP];
  for(pag = 0; pag < NUM_PAG_HEAP; pag++){
    framesH[pag] = alloc_frame();
    if(framesH[pag] < 0){
      for(i = 0; i < pag; ++i){
        free_frame((unsigned int) framesH[i]);
      }
      list_add_tail(element, &freequeue);
      update_sys_ticks(&current()->p_stats);
      return -EAGAIN;
    }
  }
  int free_pagH = NUM_PAG_KERNEL+NUM_PAG_CODE+2*NUM_PAG_DATA+NUM_PAG_HEAP+2;
  for (pag = 0; pag < NUM_PAG_HEAP; ++pag) {
    set_ss_pag(child_page_table, NUM_PAG_KERNEL+NUM_PAG_CODE+2*NUM_PAG_DATA+NUM_PAG_HEAP+pag,
               (unsigned int) framesH[pag]);
    set_ss_pag(parent_page_table, free_pagH+pag, (unsigned int) framesH[pag]);
    copy_data((void *)((NUM_PAG_KERNEL + NUM_PAG_CODE + 2*NUM_PAG_DATA + pag) * PAGE_SIZE), (void *)((free_pagH + pag) * PAGE_SIZE), PAGE_SIZE);
    del_ss_pag(parent_page_table, free_pagH+pag);
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

  int i;
  for(i = 0; i < NR_SEMAPHORES; i++){
    if(semaphores[i].owner == current_task->PID)
      sys_sem_destroy(i);
  }

  --allocated_dirs[current_task->dir_number];
  if(allocated_dirs[current_task->dir_number] == 0) {
    page_table_entry *current_pt = get_PT(current_task);
    unsigned int pag;
    for (pag = 0; pag < NUM_PAG_DATA; ++pag) {
      free_frame(get_frame(current_pt, PAG_LOG_INIT_DATA + pag));
      del_ss_pag(current_pt, PAG_LOG_INIT_DATA + pag);
    }
    for (pag = 0; pag < current_task->pages_heap; ++pag) {
      free_frame(get_frame(current_pt, PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA + pag));
      del_ss_pag(current_pt, PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA + pag);
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
    ret += sys_write_console(buff, SIZE_BUFFER);
    buffer += SIZE_BUFFER;
    size -= SIZE_BUFFER;
  }
  error = copy_from_user(buffer, buff, size);
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
  list_del(element);
  union task_union *child_union = list_entry(element, union task_union, task.list);
  union task_union *father_union = (union task_union*) current();

  copy_data(father_union, child_union, sizeof(union task_union));

  int dir = father_union->task.dir_number;
  ++allocated_dirs[dir];
  child_union->stack[KERNEL_STACK_SIZE - 19] = 0;
  child_union->stack[KERNEL_STACK_SIZE - 18] = (unsigned long)&ret_from_fork;
  child_union->stack[KERNEL_STACK_SIZE - 5] = (unsigned long) function;  //eip
  child_union->stack[KERNEL_STACK_SIZE - 2] = (unsigned long) stack;     //esp
  child_union->task.kernel_esp = (int *) &child_union->stack[KERNEL_STACK_SIZE - 19];
  init_stats(&child_union->task.p_stats);
  child_union->task.PID = nextPID++;
  list_add_tail(&child_union->task.list, &readyqueue);

  update_sys_ticks(&current()->p_stats);
  return child_union->task.PID;
}

int sys_sem_init(int n_sem, unsigned int value){
  update_user_ticks(&current()->p_stats);

  if(n_sem < 0 || n_sem > 20) return -EINVAL;
  struct semaphore *sem = &semaphores[n_sem];
  if(sem->in_use) return -EBUSY;
  sem->owner = current()->PID;
  sem->counter = value;
  sem->in_use = 1;
  INIT_LIST_HEAD(&sem->blocked);

  update_sys_ticks(&current()->p_stats);
  return 0;
}

int sys_sem_wait(int n_sem){
  update_user_ticks(&current()->p_stats);
  if(n_sem < 0 || n_sem > 20) return -EINVAL;
  struct semaphore *sem = &semaphores[n_sem];
  if(sem->counter <= 0){
    list_add_tail(&current()->list, &sem->blocked);
    update_user_ticks(&current()->p_stats);
    sched_next_rr();
  }
  else sem->counter--;
  if(!sem->in_use) return -EINVAL;
  update_sys_ticks(&current()->p_stats);
  return 0;
}

int sys_sem_signal(int n_sem){
  update_user_ticks(&current()->p_stats);
  if(n_sem < 0 || n_sem > 20) return -EINVAL;
  struct semaphore *sem = &semaphores[n_sem];
  if(!sem->in_use) return -EINVAL;
  if(list_empty(&sem->blocked)) sem->counter++;
  else {
    struct list_head *element = list_first(&sem->blocked);
    list_del(element);
    list_add_tail(element, &readyqueue);
  }

  update_sys_ticks(&current()->p_stats);
  return 0;
}

int sys_sem_destroy(int n_sem){
  update_user_ticks(&current()->p_stats);
  if(n_sem < 0 || n_sem > 20) return -EINVAL;
  struct semaphore *sem = &semaphores[n_sem];
  if(!sem->in_use) return -EINVAL;
  if(sem->owner != current()->PID) return -EPERM;

  sem->in_use = 0;
  struct list_head *it, *it2;

  list_for_each_safe(it, it2, &sem->blocked){
    list_del(it);
    list_add_tail(it, &readyqueue);
  }



  update_sys_ticks(&current()->p_stats);
  return 0;

}

int sys_read (int fd, char *buf, int count) {
  update_user_ticks(&current()->p_stats);

  int ret = check_fd(fd, LECTURA);
  if (ret < 0){
    update_sys_ticks(&current()->p_stats);
    return ret;
  }
  if (buf == NULL) {
    update_sys_ticks(&current()->p_stats);
    return -EFAULT;
  }
  if (count < 0) {
    update_sys_ticks(&current()->p_stats);
    return -EINVAL;
  }
  if(!access_ok(VERIFY_READ, buf, (unsigned long) count)) {
    update_sys_ticks(&current()->p_stats);
    return -EFAULT;
  }
  ret = sys_read_keyboard(buf, count);
  update_sys_ticks(&current()->p_stats);
  return ret;
}

int sys_read_keyboard(char *buf, int count) {
  if(!list_empty(&blocked)) {
    update_process_state_rr(current(), &blocked);
    sched_next_rr();
  }

  chars_to_read = count;
  offset = 0;
  while (chars_to_read > 0) {
    if (queue_count(&char_buffer) >= chars_to_read) {
      char aux[chars_to_read];
      int i;
      for (i = 0; i < chars_to_read; ++i) {
        aux[i] = get_first(&char_buffer);
      }
      int ret = copy_to_user(aux, &buf[offset*QUEUE_SIZE], sizeof(aux));
      return !ret ? count : -EFAULT;
    }
    else if (Q_IS_FULL(char_buffer)) {
      while (queue_count(&char_buffer)) {
        get_first(&char_buffer);
      }
      int ret = copy_to_user(char_buffer.queue, buf, QUEUE_SIZE);
      if(ret < 0) return -EFAULT;
      chars_to_read -= QUEUE_SIZE;
      offset++;
      list_add(&current()->list, &blocked);
      sched_next_rr();
    }
    else {
      list_add(&current()->list, &blocked);
      sched_next_rr();
    }
  }
}

void * sys_sbrk(int increment) {
  int HEAP_START = (NUM_PAG_KERNEL+NUM_PAG_CODE+2*NUM_PAG_DATA+1)*PAGE_SIZE;
  struct task_struct *task = current();
  if (task->heap == NULL){
    int frame = alloc_frame();
    if (frame < 0) return (void *) -1;
    set_ss_pag(get_PT(task), (unsigned int) (HEAP_START / PAGE_SIZE), (unsigned int) frame);
    task->pages_heap = 1;
    task->heap = (int *) HEAP_START;
  }
  if (increment == 0) return task->heap + task->heap_size;
  if (increment > 0) {
    void *ret = task->heap + task->heap_size;
    if (task->heap_size % PAGE_SIZE + increment < PAGE_SIZE) task->heap_size += increment;
    else {
      task->heap_size += increment;
      while (task->pages_heap * PAGE_SIZE < task->heap_size) {
        int frame = alloc_frame();
        if (frame < 0) {
          task->heap_size -= increment;
          while (task->pages_heap * PAGE_SIZE - task->heap_size > PAGE_SIZE) {
            unsigned int frame_to_del = get_frame(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap - 1);
            free_frame(frame_to_del);
            del_ss_pag(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap - 1);
            task->pages_heap--;
          }
          return (void *) -1;
        }
        set_ss_pag(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap, (unsigned int) frame);
        task->pages_heap++;
      }
    }
    return ret;
  }
  else if (task->heap_size + increment < 0) {
    task->heap_size = 0;
    while (task->pages_heap > 0) {
      unsigned int frame_to_del = get_frame(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap - 1);
      free_frame(frame_to_del);
      del_ss_pag(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap - 1);
      task->pages_heap--;
    }
    return task->heap;
  }
  else {
    task->heap_size += increment;
    while (task->pages_heap * PAGE_SIZE - task->heap_size > PAGE_SIZE) {
      unsigned int frame_to_del = get_frame(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap - 1);
      free_frame(frame_to_del);
      del_ss_pag(get_PT(task), HEAP_START / PAGE_SIZE + task->pages_heap - 1);
      task->pages_heap--;
    }
    return task->heap + task->heap_size;
  }
}


