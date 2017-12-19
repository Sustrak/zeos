#include <io.h>
#include <list.h>
#include <sched.h>

// Queue for blocked processes in I/O 
struct list_head blocked;

int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}

/*void block_io_process(struct task_struct *t, int again) {
  if(!again){
    update_process_state_rr(t, &blocked);
    sched_next_rr();
  }
  else {
    t->state = ST_BLOCKED;
    list_add(t, &blocked);
  }

}

void unblock_io_process() {
  struct list_head *element = list_first(&blocked);
  list_del(element);
  struct task_struct *t = list_head_to_task_struct(element);
  update_process_state_rr(t, &readyqueue);
}*/

