
#include <stdlib.h>
#include <libc.h>
#include <types.h>
#include <errno.h>

#define NUM_COLUMNS 80
#define NUM_ROWS    25

void clear_window() {
  int i;
  int j;
  write(1, "\n", 1);
  for (i = 0; i < NUM_ROWS+3; ++i) {
    for (j = 0; j < NUM_COLUMNS; ++j) {
      write(1, "\0", 1);
    }
    write(1, "\n", 1);
  }
}

char *itoc(int i) {
  int size = 0, i2 = i, count = 0, k = 0;
  static char *c;

  if(i==0) { static char c[2] = "0\0"; return c;}

  do {
    i2=i2/10;
    ++size;
  }while(i2>0);

  if(i<0) {size+=2; char v[size]; c = v; c[0] = '-'; count = 1; i = -i; k = 1;}
  else {size+=1; char v[size]; c = v;}
  while(i>0) {
    c[count] = (i%10) + '0';
    i = i/10;
    count++;
  }
  char a;
  for(; k<count/2; k++) {
    a = c[k];
    c[k] = c[count-k-1];
    c[count-k-1] = a;
  }
  c[count] = '\0';
  return c;
}

int atoi(char *str) {
  if(str == NULL){
    errno = EFAULT;
    return -1;
  }
  int res = 0, sign = 1, i = 0;
  if(str[0] == '-'){
    sign = -1;
    i++;
  }
  for(; str[i] != '\0'; i++){
    if(str[i] > '9' || str[i] < '0'){
      errno = EDOM;
      return -1;
    }
    res = res*10 + str[i]-'0';
  }
  return res*sign;
}
