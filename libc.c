/*
 * libc.c 
 */

#include <libc.h>
#include <errno.h>
#include <sys_codes.h>

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int write(int fd, char *buffer, int size) {
  int ret;
  __asm__ __volatile__ (
    "int $0x80"
    : "=a" (ret)
    : "a" (__NR_write), "b" (fd), "c" (buffer), "d" (size)
  );
  if(ret < 0) {
    errno = -ret;
    return -1;
  }
  return ret;
}

int gettime()
{
  int ret;
  __asm__ __volatile__ (
    "int $0x80"
    : "=a" (ret)
    : "a" (__NR_gettime)
  );
  if(ret <= 0) return -1;
  return ret;
}

int getpid() {
  int ret;
  __asm__ __volatile__ (
  "int $0x80"
  : "=a" (ret)
  : "a" (__NR_getpid)
  );
  return ret;
}

int fork() {
  int ret;
  __asm__ __volatile__ (
  "int $0x80"
  : "=a" (ret)
  : "a" (__NR_fork)
  );
  return ret;
}

void exit() {
  __asm__ __volatile__ (
  "int $0x80"
  :
  : "a" (__NR_exit)
  );
}

int get_stats(int pid, struct stats *st) {
  int ret;
  __asm__ __volatile__ (
  "int $0x80"
  : "=a" (ret)
  : "a" (__NR_getstats), "b" (pid), "c" (st)
  );
  if(ret < 0) {
    errno = -ret;
    return -1;
  }
  return ret;
}

int clone(void (*function)(void), void *stack) {
  int ret;
  __asm__ __volatile__ (
  "int $0x80;"
  : "=a" (ret)
  : "a" (__NR_clone), "b" (function), "c" (stack)
  );
  if(ret < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}


