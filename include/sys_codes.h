
#ifndef ZEOS_SYS_CODES_H
#define ZEOS_SYS_CODES_H

//sys_calls codes

#define __NR_exit 1          //exit
#define __NR_fork 2          //fork
#define __NR_write   4       //write
#define __NR_gettime 10      //get_time
#define __NR_clone 19        //clone
#define __NR_getpid 20       //getpid
#define __NR_sem_init 21     //sem_init
#define __NR_sem_wait 22     //sem_init
#define __NR_sem_signal 23   //sem_init
#define __NR_sem_destroy 24  //sem_init
#define __NR_getstats 35     //get_stats

#endif //ZEOS_SYS_CODES_H
