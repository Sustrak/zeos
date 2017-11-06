#include <zeosarg.h>

/*
 * Starts the va_list adding the size of the first argument to its mem address
 * making va_list ready to operate
 */
va_list va_start(va_list args, unsigned int size){
  return args+size;
}

int va_args(va_list *args, unsigned int size){
  *args += size;
  return **args;
}
